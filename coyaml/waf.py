
try:
    from waflib import Task, TaskGen
except ImportError:
    raise ImportError("Can use coyaml.waf only from wscript")

def coyaml_decider(context, target):
    if 'coyaml' in context.features:
        return ['.h', '.c']
    return []

def coyaml_gen(task):
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
        
TaskGen.declare_chain(
        name      = 'coyaml', 
        rule      = coyaml_gen, 
        ext_in    = '.yaml',
        reentrant = False,
        decider   = coyaml_decider,
        before    = 'c',
)

@TaskGen.feature('coyaml')
@TaskGen.after('apply_link')
def process_coyaml(self):
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
