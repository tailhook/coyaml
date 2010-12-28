
try:
    from waflib import Task, TaskGen
except ImportError:
    raise ImportError("Can use coyaml.waf only from wscript")

def coyaml_decider(context, target):
    if 'coyaml' in context.features:
        return ['.h', '.c']
    return None

def coyaml_gen(task):
    if not task.outputs:
        return
    from . import cgen, hgen, core, load, textast
    name = getattr(task.generator, 'config_name', 'config')
    src = task.inputs[0]
    tgt = task.outputs[0]
    cfg = core.Config(name, tgt.name[:-len(tgt.suffix())])
    with open(src.abspath(), 'rb') as f:
        load.load(f, cfg)
    with open(tgt.abspath(), 'wt', encoding='utf-8') as f:
        with textast.Ast() as ast:
            hgen.GenHCode(cfg).make(ast)
        f.write(str(ast))
    tgt = task.outputs[1]
    cfg = core.Config(name, tgt.name[:-len(tgt.suffix())])
    with open(src.abspath(), 'rb') as f:
        load.load(f, cfg)
    with open(tgt.abspath(), 'wt', encoding='utf-8') as f:
        with textast.Ast() as ast:
            cgen.GenCCode(cfg).make(ast)
        f.write(str(ast))

Task.task_type_from_func(
        name      = 'coyaml', 
        func      = coyaml_gen, 
        ext_in    = '.yaml',
        ext_out   = ['.h', '.c'],
        before    = 'c',
)

@TaskGen.extension('.yaml')
def process_coyaml(self, node):
    if not 'coyaml' in self.features:
        return
    cfile = node.change_ext('.c')
    self.create_task('coyaml', node,
        [node.change_ext('.h'), cfile])
    self.source.append(cfile)
    
@TaskGen.feature('coyaml')
def process_coyaml(self):
    pass

"""
    print("TASKS", self.tasks)
    self.mappings['.h'] = lambda *k, **kw: None
    link = []
    coyaml = []
    for t in self.tasks:
        if t.__class__.__name__ == 'coyaml':
            coyaml.append(t)
        elif t.__class__.__name__ == 'cprogram':
            link.append(t)
            
    for l in link:
        for c in coyaml:
            l.inputs.append(c.outputs[-1])
"""
