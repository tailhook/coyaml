import textwrap
from collections import defaultdict

from . import load, core

def cstring(val):
    return '"{0}"'.format(repr(val)[1:-1].replace('"', r'\"'))

class ArrayVal(object):
    def __init__(self, value):
        self.value = list(value)

    def format(self, prefix, suffix='{val0.__class__.__name__}_vars'):
        yield 'static {0} {1}_{2}[{3}] = {{'.format(self.value[0].ctype,
            prefix, suffix.format(val0=self.value[0]), len(self.value))
        for i, val in enumerate(self.value):
            if val is None:
                yield '    {{ NULL }}, // {0}. sentinel'.format(i)
                continue
            if self.value[-1] == val:
                yield '    {0}  // {1}. {2}'.format(
                    val.format(prefix), i,
                    '.'.join(val.path) if hasattr(val, 'path') else '')
            else:
                yield '    {0}, // {1}. {2}'.format(
                    val.format(prefix), i,
                    '.'.join(val.path) if hasattr(val, 'path') else '')
            if getattr(val, 'start_mark', None):
                yield '    //{0}'.format(val.start_mark)
        yield '};'

class CStruct(object):
    fields = ()
    bitmask = True
    def __init__(self, val):
        val.ctype = self
        self.start_mark = val.start_mark
        self.path = val.path
        for k, typ in self.fields:
            if hasattr(val, k):
                setattr(self, k, typ(getattr(val, k)))

    @classmethod
    def array(cls, value):
        return ArrayVal(value)

    def format(self, prefix):
        if not self.fields:
            return '{}'
        res = []
        bitmask = 0
        for i, (k, v) in enumerate(self.fields):
            if hasattr(self, k):
                bitmask |= 1 << i
                val = getattr(self, k)
                if v == str:
                    val = '"{0}"'.format(repr(val)[1:-1])
                elif v == CStruct:
                    if val:
                        val = '&{0}_{1}_vars[{2}]'.format(prefix,
                            val.__class__.__name__, val.index)
                    else:
                        val = "NULL"
                elif v == bool:
                    val = str(val).upper()
                res.append('{0}: {1}'.format(k, val))
        if self.bitmask:
            res.append('bitmask: {0}'.format(bitmask))
        return '{{{0}}}'.format(', '.join(res))

class CInt(CStruct):
    ctype = 'coyaml_int_t'
    fields = (
        ('min', int),
        ('max', int),
        )

class CUInt(CInt):
    ctype = 'coyaml_uint_t'

class CFloat(CStruct):
    ctype = 'coyaml_float_t'
    fields = (
        ('min', float),
        ('max', float),
        )

class CString(CStruct):
    ctype = 'coyaml_string_t'

class CBool(CStruct):
    ctype = 'coyaml_bool_t'

class CArray(CStruct):
    ctype = 'coyaml_array_t'

class CMapping(CStruct):
    ctype = 'coyaml_mapping_t'

class CFile(CStruct):
    ctype = 'coyaml_file_t'
    fields = (
        ('check_existence', bool),
        ('check_dir', bool),
        ('check_writable', bool),
        )

class CDir(CStruct):
    ctype = 'coyaml_dir_t'
    fields = (
        ('check_existence', bool),
        ('check_dir', bool),
        ('check_writable', bool),
        )

class CTransition(CStruct):
    ctype = 'coyaml_transition_t'
    bitmask = False
    fields = (
        ('symbol', str),
        ('callback', None),
        ('prop', CStruct),
        )
    def __init__(self, key, typ, *, path):
        self.symbol = key
        self.prop = typ
        self.path = path
        self.start_mark = typ.start_mark

    @property
    def callback(self):
        return '(coyaml_state_fun)&coyaml_' + self.prop.__class__.__name__

class CTag(CStruct):
    ctype = 'coyaml_tag_t'
    bitmask = False
    fields = (
        ('tagname', str),
        ('tagvalue', int),
        )
    def __init__(self, name, value):
        self.name = name
        self.value = value

class CGroup(CStruct):
    ctype = 'coyaml_group_t'
    fields = (
        ('transitions', CStruct),
        )
    def __init__(self, dic, first_tran, *, path):
        self.path = path
        self.start_mark = dic.start_mark
        self.first_tran = first_tran

    @property
    def transitions(self):
        return self.first_tran

class CCustom(CStruct):
    ctype = 'coyaml_custom_t'
    fields = (
        ('transitions', CStruct),
        )

    @property
    def transitions(self):
        return self.first_tran

class CUsertype(CStruct):
    ctype = 'coyaml_usertype_t'
    fields = (
        ('transitions', CStruct),
        )

    def __init__(self, data, child, *, path):
        self.data = data
        self.child = child
        self.path = path

class CGetopt(CStruct):
    ctype = 'struct option'
    bitmask = False
    fields = (
        ('name', str),
        ('has_arg', bool),
        ('flag', None),
        ('val', int),
        )
    flag = 'NULL'
    def __init__(self, opt, val=None):
        if isinstance(opt, str):
            self.name = opt
            self.has_arg = False
        else:
            self.name = opt.name
            self.has_arg = opt.has_argument
        if val is None:
            self.val = 1000+opt.index
        else:
            self.val = val

class COption(CStruct):

    ctype = 'coyaml_option_t'
    bitmask = False
    fields = (
        ('callback', None),
        ('prop', CStruct),
        )
    def __init__(self, callback, prop):
        self.callback = callback
        self.prop = prop


ctypes = {
    'Int': CInt,
    'Uint': CUInt,
    'Float': CFloat,
    'String': CString,
    'Bool': CBool,
    'Array': CArray,
    'Mapping': CMapping,
    'File': CFile,
    'Dir': CDir,
    'Struct': CCustom,
    'Usertype': CUsertype,
    'dict': CGroup,
    'transition': CTransition,
    }

class GenCCode(object):

    def __init__(self, cfg):
        self.cfg = cfg
        self.lines = [
            '/* THIS IS AUTOGENERATED FILE */',
            '/* DO NOT EDIT!!! */',
            '#include <coyaml_src.h>',
            '#include "{0}.h"'.format(cfg.targetname),
            '',
            'static coyaml_transition_t '+self.cfg.name+'_CTransition_vars[];',
            ]
        self.types = defaultdict(list)
        self.type_visitor(cfg.data)
        self.types['dict'].reverse() # Root item must be first
        for k, t in cfg.types.items():
            self.type_visitor(t, ('__types__', k))
        self.type_enum()
        self.make_types()
        self.make_options()
        self.lines.extend(r'''
int {prefix}_optidx[] = {{{optidx}}};

void {prefix}_print_config(FILE *output, {prefix}_main_t *config);
coyaml_cmdline_t {prefix}_cmdline = {{
    usage: {usage},
    full_description: {full_description},
    optstr: {optstr},
    optidx: {prefix}_optidx,
    options: {prefix}_getopt_options,
    coyaml_options: {prefix}_COption_vars,
    print_callback: (void (*)(FILE *,void*))&{prefix}_print_config,
    filename: {default_filename},
    debug: FALSE
    }};

void {prefix}_print_config(FILE *output, {prefix}_main_t *config) {{
    fprintf(output, "%YAML-1.1\n");
    fprintf(output, "# Configuration {prefix}\n");
}}

bool {prefix}_readfile(char *filename, {prefix}_main_t *target, bool debug) {{
    return coyaml_readfile(filename, &{prefix}_CGroup_vars[0], target, debug);
}}
'''.format(prefix=cfg.name,
        usage=cstring("Usage:\n    {0} [options]\n"
            .format(cfg.meta.program_name)),
        full_description=cstring(self.cmdline_descr),
        optstr=cstring(self.cmdline_optstr),
        optidx=', '.join(map(str, self.cmdline_optidx)),
        default_filename=cstring(cfg.meta.default_config),
        ).splitlines())

    def make_types(self):
        types = self.types.copy()
        tran = types.pop('transition', [])
        for tname, items in types.items():
            self.lines.append('')
            self.lines.extend(ctypes[tname].array(items)
                .format(prefix=self.cfg.name))
        self.lines.append('')
        self.lines.extend(CTransition.array(tran)
            .format(prefix=self.cfg.name))

    def make_options(self):
        visited = set()
        options = []
        targets = []
        for opt in self.cfg.commandline:
            if not hasattr(opt.target, 'options'):
                opt.target.options = defaultdict(list)
                targets.append(opt.target)
            opt.target.options[opt.__class__].append(opt)
            key = id(opt.target), opt.__class__
            if key in visited:
                continue
            opt.index = len(options)
            options.append(COption(
                '(coyaml_option_fun)&coyaml_'
                    +opt.target.ctype.__class__.__name__+'_o',
                opt.target.ctype))
        self.lines.extend(COption.array(options).format(self.cfg.name))
        self.lines.append('')
        options = [
            CGetopt('help', 500),
            CGetopt('config', 501),
            CGetopt('debug-config', 502),
            CGetopt('print-config', 600),
            CGetopt('check-config', 601),
            ]
        options.extend(CGetopt(opt)
            for opt in self.cfg.commandline if not opt.short)
        self.lines.extend(CGetopt.array(options)
            .format(prefix=self.cfg.name, suffix='getopt_options'))
        stroptions = []
        for target in targets:
            for typ in (core.Option, core.IncrOption, core.DecrOption):
                opt = ', '.join(o.param for o in target.options[typ])
                if not opt:
                    continue
                if typ != core.Option and target.options[core.Option]:
                    if typ == core.IncrOption:
                        description = 'Increment aformentioned value'
                    elif typ == core.DecrOption:
                        description = 'Decrement aformentioned value'
                else:
                    description = target.description
                if len(opt) < 17:
                    opt = '  {:17s} '.format(opt)
                    stroptions.extend(textwrap.wrap(description,
                        width=80, initial_indent=opt, subsequent_indent=' '*20))
                else:
                    stroptions.append('  '+opt)
                    stroptions.extend(textwrap.wrap(description,
                        width=80, initial_indent=opt, subsequent_indent=' '*20))
        self.cmdline_descr = """\
Usage:
    {m.program_name} [options]

Description:
{description}

Options:
{options}
""".format(m=self.cfg.meta,
            description='\n'.join(textwrap.wrap(self.cfg.meta.description,
                 width=80, initial_indent='    ', subsequent_indent='    ')),
            options='\n'.join(stroptions),
            )
        self.cmdline_optstr = ("c:hP" # filename, help, print
            + ''.join(c.char + (':' if c.has_argument else '')
                for c in self.cfg.commandline if c.short))
        self.cmdline_optidx = [501, 0, 500, 600]
        for c in self.cfg.commandline:
            if c.short:
                self.cmdline_optidx.append(1000+c.index)
                if c.has_argument:
                    self.cmdline_optidx.append(0)

    def type_visitor(self, data, path=()):
        data.path = path
        if isinstance(data, load.Usertype):
            mem = load.Group(data.members)
            mem.start_mark = data.start_mark
            child = self.type_visitor(mem, path=path)
            res = CUsertype(data, child, path=path)
            self.types['Usertype'].append(res)
        elif isinstance(data, dict):
            children = []
            for k, v in data.items():
                chpath = path + (k,)
                typ = self.type_visitor(v, chpath)
                children.append(CTransition(k, typ, path=chpath))
            if children:
                children.append(None)
                self.types['transition'].extend(children)
            res = CGroup(data, children and children[0] or None, path=path)
            self.types['dict'].append(res)
        else:
            tname = data.__class__.__name__
            res = ctypes[tname](data)
            self.types[tname].append(res)
        return res

    def type_enum(self):
        for lst in self.types.values():
            for i, t in enumerate(lst):
                if t is None:
                    continue
                t.index = i

    def print(self):
        for line in self.lines:
            print(line)

    def write_into(self, file):
        for line in self.lines:
            file.write(line + '\n')

def main():
    from .cli import simple
    from .load import load
    cfg, inp, opt = simple()
    with inp:
        load(inp, cfg)
    GenCCode(cfg).print()

if __name__ == '__main__':
    from .cgen import main
    main()
