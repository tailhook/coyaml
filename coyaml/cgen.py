import textwrap
from collections import defaultdict
from contextlib import nested

from . import load, core
from .cutil import varname, string, typename
from .cast import *

cmdline_template = """\
Usage:
    {m.program_name} [options]

Description:
{description}

Options:
{options}
"""

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
        self.struct_name = val.struct_name
        self.member_path = val.member_path
        for k, typ in self.fields:
            if hasattr(val, k):
                setattr(self, k, typ(getattr(val, k)))

    @classmethod
    def array(cls, value):
        return ArrayVal(value)

    def format(self, prefix):
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
        if hasattr(self, 'struct_name'):
            res.append('baseoffset: {0}'.format(self.baseoffset))
        if self.bitmask and self.fields:
            res.append('bitmask: {0}'.format(bitmask))
        return '{{{0}}}'.format(', '.join(res))

    @property
    def baseoffset(self):
        return 'offsetof({0}, {1})'.format(self.struct_name, self.member_path)

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
    bitmask = False
    fields = (
        ('element_size', None),
        ('element_prop', CStruct),
        ('element_callback', None),
        ('element_defaults', None),
        )

    @property
    def element_callback(self):
        return '(coyaml_state_fun)&coyaml_' \
            + self.element_prop.__class__.__name__

class CMapping(CStruct):
    ctype = 'coyaml_mapping_t'
    bitmask = False
    fields = (
        ('element_size', None),
        ('key_prop', CStruct),
        ('value_prop', CStruct),
        ('key_callback', None),
        ('value_callback', None),
        ('key_defaults', None),
        ('value_defaults', None),
        )

    @property
    def key_callback(self):
        return '(coyaml_state_fun)&coyaml_' + self.key_prop.__class__.__name__

    @property
    def value_callback(self):
        return '(coyaml_state_fun)&coyaml_' + self.value_prop.__class__.__name__


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
        ('usertype', CStruct),
        )
    def __init__(self, val):
        val.ctype = self
        self.start_mark = val.start_mark
        self.path = val.path
        self.struct_name = val.struct_name
        self.member_path = val.member_path
        self.typename = val.type

class CUsertype(CStruct):
    ctype = 'coyaml_usertype_t'
    fields = (
        ('group', CStruct),
        )

    def __init__(self, data, child, *, path):
        self.data = data
        self.group = child
        self.path = path
        data.ctype = self

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

placeholders = {
    load.Int: '%d',
    load.UInt: '%u',
    load.String: '%s',
    load.File: '%s',
    load.Dir: '%s',
    }

cfgtoast = {
    load.Int: Int,
    load.UInt: Int,
    load.String: String,
    load.File: String,
    load.Dir: String,
    }


class GenCCode(object):

    def __init__(self, cfg):
        self.cfg = cfg
        self.prefix = cfg.name

    def make(self, ast):
        ast(CommentBlock(
            'THIS IS AUTOGENERATED FILE',
            'DO NOT EDIT!!!',
            ))
        ast(StdInclude('coyaml_src.h'))
        ast(StdInclude('stdlib.h'))
        ast(StdInclude('errno.h'))
        ast(StdInclude('strings.h'))
        ast(Include(self.cfg.targetname+'.h'))
        ast(VSpace())
        ast(Var('coyaml_transition_t', self.cfg.name+'_transition_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_usertype_t', self.cfg.name+'_usertype_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_group_t', self.cfg.name+'_group_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_string_t', self.cfg.name+'_string_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_int_t', self.cfg.name+'_int_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_uint_t', self.cfg.name+'_uint_vars',
            static=True, array=(None,)))
        ast(Var('coyaml_custom_t', self.cfg.name+'_custom_vars',
            static=True, array=(None,)))
        self.visit_hier(ast)

        self.make_options(ast)

#~         self.make_defaultfuncs()
#~         self.type_visitor(cfg.data)
#~         self.types['dict'].reverse() # Root item must be first
#~         for k, t in cfg.types.items():
#~             self.type_visitor(t, ('__types__', k))
#~         self.type_enum()
#~         self.make_types()
#~         import pprint; pprint.pprint(dict(printerlines))
#~         self.lines.extend(r'''
#~ int {prefix}_optidx[] = {{{optidx}}};

#~ void {prefix}_print_config(FILE *output, {prefix}_main_t *config);
#~ coyaml_cmdline_t {prefix}_cmdline = {{
#~     usage: {usage},
#~     full_description: {full_description},
#~     optstr: {optstr},
#~     optidx: {prefix}_optidx,
#~     options: {prefix}_getopt_options,
#~     coyaml_options: {prefix}_COption_vars,
#~     print_callback: (void (*)(FILE *,void*))&{prefix}_print_config,
#~     filename: {default_filename},
#~     debug: FALSE
#~     }};

#~ void {prefix}_print_config(FILE *output, {prefix}_main_t *config) {{
#~     fprintf(output, "%YAML-1.1\n");
#~     fprintf(output, "# Configuration {prefix}\n");
#~ {printerlines}
#~ }}

#~ bool {prefix}_readfile(char *filename, {prefix}_main_t *target, bool debug) {{
#~     return coyaml_readfile(filename, &{prefix}_CGroup_vars[0], target, debug);
#~ }}

#~ {prefix}_main_t *{prefix}_init({prefix}_main_t *ptr) {{
#~     {prefix}_main_t *res = ptr;
#~     if(!res) {{
#~         res = ({prefix}_main_t *)malloc(sizeof({prefix}_main_t));
#~     }}
#~     if(!res) return NULL;
#~     bzero(res, sizeof({prefix}_main_t));
#~     if(!res) {{
#~         res->head.free_object = TRUE;
#~     }}
#~     obstack_init(&res->head.pieces);
#~     {prefix}_defaults_main(ptr);
#~     return res;
#~ }}
#~ void {prefix}_free({prefix}_main_t *ptr) {{
#~     obstack_free(&ptr->head.pieces, NULL);
#~     if(ptr->head.free_object) {{
#~         free(ptr);
#~     }};
#~ }}
#~ {prefix}_main_t *{prefix}_load({prefix}_main_t *ptr, int argc, char **argv) {{
#~     if(coyaml_cli_prepare(argc, argv, &cfg_cmdline) < 0) {{
#~         if(errno > ECOYAML_MAX || errno < ECOYAML_MIN)
#~             perror(argv[0]);
#~         // else, error is already printed by coyaml
#~         exit((errno == ECOYAML_CLI_HELP) ? 0 : 1);
#~     }}
#~     ptr = {prefix}_init(ptr);
#~     if(!ptr) {{
#~         perror(argv[0]);
#~     }}
#~     if({prefix}_readfile(cfg_cmdline.filename, ptr, cfg_cmdline.debug) < 0) {{
#~         if(errno > ECOYAML_MAX || errno < ECOYAML_MIN)
#~             perror(argv[0]);
#~         // else, error is already printed by coyaml
#~         {prefix}_free(ptr);
#~         exit(1);
#~     }}
#~     if(coyaml_cli_parse(argc, argv, &cfg_cmdline, ptr) < 0) {{
#~         if(errno > ECOYAML_MAX || errno < ECOYAML_MIN)
#~             perror(argv[0]);
#~         // else, error is already printed by coyaml
#~         {prefix}_free(ptr);
#~         exit((errno == ECOYAML_CLI_EXIT) ? 0 : 1);
#~     }}
#~     return ptr;
#~ }}
#~ '''.format(prefix=cfg.name,
#~         usage=cstring("Usage:\n    {0} [options]\n"
#~             .format(cfg.meta.program_name)),
#~         full_description=cstring(self.cmdline_descr),
#~         optstr=cstring(self.cmdline_optstr),
#~         optidx=', '.join(map(str, self.cmdline_optidx)),
#~         default_filename=cstring(cfg.meta.default_config),
#~         printerlines='\n'.join(self.printerlines),
#~         ).splitlines())

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

    def make_options(self, ast):
        optval = 1000
        visited = set()
        targets = []
        for opt in self.cfg.commandline:
            if not hasattr(opt.target, 'options'):
                opt.target.options = defaultdict(list)
                targets.append(opt.target)
            opt.target.options[opt.__class__].append(opt)
            key = id(opt.target), opt.__class__
            if key in visited:
                continue
            opt.index = optval
            optval += 1
        ast(VSpace())
        with ast(VarAssign('struct option',
            self.prefix+'_getopt_ar', Arr(ast.block()), array=(None,))) as cmd:
            cmd(StrValue(name=String('help'), val=Int(500),
                flag='NULL', has_arg='FALSE')),
            cmd(StrValue(name=String('config'), val=Int(501),
                flag='NULL', has_arg='TRUE')),
            cmd(StrValue(name=String('debug-config'), val=Int(502),
                flag='NULL', has_arg='FALSE')),
            cmd(StrValue(name=String('print-config'), val=Int(600),
                flag='NULL', has_arg='FALSE')),
            cmd(StrValue(name=String('check-config'), val=Int(601),
                flag='NULL', has_arg='FALSE')),
            optstr = "hc:PC"
            for opt in self.cfg.commandline:
                has_arg = opt.__class__ == core.Option
                if opt.char:
                    optstr += opt.char
                    if has_arg:
                        optstr += ':'
                if opt.name:
                    cmd(StrValue(name=String(opt.name), val=Int(opt.index),
                        flag='NULL',
                        has_arg='TRUE' if has_arg else 'FALSE'))

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
        descr = cmdline_template.format(m=self.cfg.meta,
            description='\n'.join(textwrap.wrap(self.cfg.meta.description,
                 width=80, initial_indent='    ', subsequent_indent='    ')),
            options='\n'.join(stroptions),
            )
        usage = "Usage: {m.program_name} [options]\n".format(m=self.cfg.meta)
        ast(VarAssign('coyaml_cmdline_t', self.prefix+'_cmdline',
            StrValue(
                optstr=String(optstr),
                usage=String(usage),
                full_description=String(descr),
                options=self.prefix+'_getopt_ar',
                coyaml_options='NULL',
                print_callback=self.prefix+'_print',
            )))

    def visit_hier(self, ast):
        # Visits hierarchy to set appropriate structures and member
        # names for `offsetof()` in `baseoffset`
        self.print_functions = set()
        ast(VSpace())
        with ast(Function('int', self.prefix+'_print', [
            Param('FILE *', 'out'),
            Param('const char *', 'prefix'),
            Param(self.prefix+'_main_t *', 'cfg'),
            ], ast.block())) as past:
            with ast(Function('int', self.prefix+'_defaults', [
                Param(self.prefix+'_main_t *', 'cfg'),
                ], ast.block())) as dast:
                for k, v in self.cfg.data.items():
                    self._visit_hier(v, k, self.prefix+'_main_t', '',
                        Member('cfg', varname(k)),
                        past=past, pfun=past, dast=dast, root=ast)
            free = getattr(past, '_const_free', ())
            for i in free:
                past(Statement(Call('free', [ Ident(i) ])))

    def _visit_hier(self, item, name, struct_name, pws, mem,
        past, pfun, dast, root):
        if isinstance(item, dict):
            past(Statement(Call('fprintf', [ Ident('out'),
                String('%s{0}{1}:\n'.format(pws, name)), Ident('prefix') ])))
            for k, v in item.items():
                self._visit_hier(v, k, struct_name, pws+'  ',
                    Dot(mem, varname(k)),
                    past=past, pfun=pfun, dast=dast, root=root)
        elif item.__class__ in placeholders:
            item.struct_name = struct_name
            item.member_path = mem
            if hasattr(item, 'default_'):
                if isinstance(item, (load.File, load.String, load.Dir)):
                    val = string(item.default_)
                else:
                    val = str(item.default_)
#~                 self.defaultfuncs[struct_name].append((mem, val))
            past(Statement(Call('fprintf', [ Ident('out'),
                String('%s{0}{1}: {2}\n'.format(pws, name,
                placeholders[item.__class__])), Ident('prefix'), mem ])))
            if hasattr(item, 'default_'):
                dast(Statement(Assign(mem,
                    cfgtoast[item.__class__](item.default_))))
        elif isinstance(item, load.Struct):
            item.struct_name = struct_name
            item.member_path = mem
            sname = self.prefix+'_'+item.type+'_t'
            fname = self.prefix+'_print_'+item.type
            if fname not in self.print_functions:
                self.print_functions.add(fname)
                typname = self.prefix+'_'+item.type+'_t'
                root(VSpace())
                with root(Function('int', fname, [
                    Param('FILE *', 'out'),
                    Param('const char *', 'prefix'),
                    Param(typname+' *', 'cfg'),
                    ], root.block())) as cur:
                    defname = self.prefix+'_defaults_'+item.type
                    with root(Function('int', defname, [
                        Param(typname+' *', 'cfg'),
                        ], root.block())) as cdef:
                        for k, v in self.cfg.types[item.type].members.items():
                            self._visit_hier(v, k, typname,
                                '', Member('cfg', varname(k)),
                                past=cur, pfun=cur, dast=cdef, root=root)
                    free = getattr(cur, '_const_free', ())
                    for i in free:
                        cur(Statement(Call('free', [ Ident(i) ])))
            if name is not None:
                past(Statement(Call('fprintf', [ Ident('out'),
                    String('%s{0}{1}:\n'.format(pws, name)),
                    Ident('prefix') ])))
            pxname = 'prefix{0}'.format(len(pws)+2)
            if not hasattr(pfun, '_const_'+pxname):
                setattr(pfun, '_const_'+pxname, True)
                if not hasattr(pfun, '_const_free'):
                    pfun._const_free = []
                pfun._const_free.append(pxname)
                pfun.insert_first(VarAssign('char *', Ident(pxname),
                    Call('asprintf', [ String('%s' + pws + '  '),
                    Ident('prefix') ])))
            past(Statement(Call(self.prefix+'_print_'+item.type, [ Ident('out'),
                Ident(pxname), Deref(mem)])))
            dast(Statement(Call(self.prefix+'_defaults_'+item.type, [
                Deref(mem) ])))
        elif isinstance(item, load.Mapping):
            item.struct_name = struct_name
            item.member_path = mem
            past(Statement(Call('fprintf', [ Ident('out'),
                String('%s{0}{1}:\n'.format(pws, name)),
                Ident('prefix') ])))
            with past(For(
                FVar(self.prefix+'_m_'+typename(item.key_element)
                    +'_'+typename(item.value_element)+'_t *', 'item', mem),
                Ident('item'), Assign(Ident('item'),
                Dot(Member(Ident('item'), 'head'), 'next')),
                past.block())) as loop:
                if not isinstance(item.key_element, load.Struct)\
                    and not isinstance(item.value_element, load.Struct):
                    loop(Statement(Call('fprintf', [ Ident('out'),
                        String('%s{0}  {1}: {2}\n'.format(pws,
                        placeholders[item.key_element.__class__],
                        placeholders[item.value_element.__class__])),
                        Ident('prefix'),
                        Member(Ident('item'), 'key'),
                        Member(Ident('item'), 'value'),
                        ])))
                elif not isinstance(item.key_element, load.Struct):
                    loop(Statement(Call('fprintf', [ Ident('out'),
                        String('%s{0}  {1}:\n'.format(pws,
                            placeholders[item.key_element.__class__])),
                        Ident('prefix'), Member(Ident('item'), 'key'),
                        ])))
                    self._visit_hier(item.value_element, None, '{0}_m_{1}_{2}_t'
                        .format(self.prefix, typename(item.key_element),
                        typename(item.value_element)), pws + '    ',
                        Member(Ident('item'), 'value'),
                        past=loop, pfun=pfun, dast=dast, root=root)
                else:
                    raise NotImplementedError(item.key_element)
#~             item.struct_name = struct_name
#~             item.member_path = mem
#~             self._visit_hier(item.key_element, '{0}_m_{1}_{2}_t'.format(
#~                 self.cfg.name, ctypename(item.key_element),
#~                     ctypename(item.value_element)), 'key')
#~             self._visit_hier(item.value_element, '{0}_m_{1}_{2}_t'.format(
#~                 self.cfg.name, ctypename(item.key_element),
#~                     ctypename(item.value_element)), 'value')
#~             self._printerline('{0}{1}:\n'.format(pws, name))
#~             self.printerlines.append(
#~                 '    for({prefix}_m_{0}_{1}_t *item={2}; item; item = item->head.next) {{'
#~                 .format(ctypename(item.key_element),
#~                 ctypename(item.value_element), mem, prefix=self.cfg.name))
#~             if(not isinstance(item.key_element, load.Struct)
#~                 and not isinstance(item.value_element, load.Struct)):
#~                 self._printerline(ws + '  {0}: {1}\n'.format(
#~                     placeholders[item.key_element.__class__],
#~                     placeholders[item.value_element.__class__]),
#~                     'item->key', 'item->value', ws='        ')
#~             else:
#~                 raise NotImplementedError(item.key_element, item.value_element)
#~             self.printerlines.append('    }')
        elif isinstance(item, load.Array):
            item.struct_name = struct_name
            item.member_path = mem
            past(Statement(Call('fprintf', [ Ident('out'),
                String('%s{0}{1}:\n'.format(pws, name)),
                Ident('prefix') ])))
            loop = For(
                FVar(self.prefix+'_a_'+typename(item.element)+'_t *',
                    'item', mem),
                Ident('item'), Assign(Ident('item'),
                Dot(Member(Ident('item'), 'head'), 'next')),
                past.block())
            with nested(past(loop), dast(loop)) as (ploop, dloop):
                if not isinstance(item.element, load.Struct):
                    ploop(Statement(Call('fprintf', [ Ident('out'),
                        String('%s{0}  - {1}\n'.format(pws,
                        placeholders[item.element.__class__])),
                        Ident('prefix'),
                        Member(Ident('item'), 'value'),
                        ])))
                else:
                    ploop(Statement(Call('fprintf', [ Ident('out'),
                        String('%s{0}  -\n'.format(pws)),
                        Ident('prefix'),
                        ])))
                    self._visit_hier(item.element, None, '{0}_a_{1}_t'.format(
                        self.prefix, typename(item.element)), pws + '  ',
                        Member(Ident('item'), 'value'),
                        past=ploop, pfun=pfun, dast=dloop, root=root)
#~             self.printerlines.append(
#~                 '    for({prefix}_a_{0}_t *item={1}; item; item = item->head.next) {{'
#~                 .format(ctypename(item.element), mem, prefix=self.cfg.name))
#~             if not isinstance(item.element, load.Struct):
#~                 self._printerline(ws + '- {0}\n'.format(
#~                     placeholders[item.element.__class__]),
#~                     'item->value', ws='        ')
#~             else:
#~                 self._printerline(ws + '-\n', ws='        ')
#~                 self._make_printer(item.element, None,
#~                     ws+'  ', 'item->value');
        else:
            raise NotImplementedError(item)

    def type_visitor(self, data, path=()):
        data.path = path
        if isinstance(data, load.Usertype):
            mem = load.Group(data.members)
            mem.start_mark = data.start_mark
            child = self.type_visitor(mem, path=path)
            res = CUsertype(data, child, path=path)
            self.types['Usertype'].append(res)
        elif isinstance(data, load.Mapping):
            res = CMapping(data)
            res.element_size = 'sizeof({0}_m_{1}_{2}_t)'.format(
                self.cfg.name, ctypename(data.key_element),
                    ctypename(data.value_element))
            res.key_prop = self.type_visitor(data.key_element, path+('key',))
            if isinstance(res.key_prop, CCustom):
                res.key_defaults = '(coyaml_defaults_fun)&{0}_defaults_{1}'\
                    .format(self.cfg.name, ctypename(data.key_element))
            res.value_prop = self.type_visitor(data.value_element,
                path+('value',))
            if isinstance(res.value_prop, CCustom):
                res.value_defaults = '(coyaml_defaults_fun)&{0}_defaults_{1}'\
                    .format(self.cfg.name, ctypename(data.value_element))
            self.types['Mapping'].append(res)
        elif isinstance(data, load.Array):
            res = CArray(data)
            res.element_size = 'sizeof({0}_a_{1}_t)'.format(
                self.cfg.name, ctypename(data.element))
            res.element_prop = self.type_visitor(data.element, path+('value',))
            if isinstance(res.element_prop, CCustom):
                res.element_defaults = '(coyaml_defaults_fun)&{0}_defaults_{1}'\
                    .format(self.cfg.name, ctypename(data.element))
            self.types['Array'].append(res)
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
        if 'Struct' in self.types:
            for t in self.types['Struct']:
                t.usertype = self.cfg.types[t.typename].ctype

    def make_defaultfuncs(self):
        self.lines.append('')
        for k, lst in self.defaultfuncs.items():
            self.lines.append('void {prefix}_defaults_{1}'
                '({0} *target);'.format(k, k[len(self.cfg.name)+1:-2],
                prefix=self.cfg.name))
        self.lines.append('')
        for k, lst in self.defaultfuncs.items():
            if '_a_' in k or '_m_' in k:
                continue
            self.lines.append('void {prefix}_defaults_{1}'
                '({0} *target) {{'.format(k, k[len(self.cfg.name)+1:-2],
                prefix=self.cfg.name))
            uniq = set()
            for mem, val in lst:
                if mem in uniq:
                    continue
                uniq.add(mem)
                if val in self.defaultfuncs:
                    self.lines.append('    {prefix}_defaults_{0}(&target->{1});'
                        .format(val[len(self.cfg.name)+1:-2], mem,
                            prefix=self.cfg.name))
                else:
                    self.lines.append('    target->{0} = {1};'.format(mem, val))
            self.lines.append('}')


def main():
    from .cli import simple
    from .load import load
    cfg, inp, opt = simple()
    with inp:
        load(inp, cfg)
    generator = GenCCode(cfg)
    ast = Ast()
    generator.make(ast)
    print(str(ast))

if __name__ == '__main__':
    from .cgen import main
    main()
