try:
    import Task, TaskGen
except ImportError:
    raise ImportError("Can use coyaml.waf only from wscript")

def coyaml_gen(task):
    from . import cgen, hgen, core, load, textast
    src = task.inputs[0]
    tgt = task.outputs[0]
    cfg = core.Config(task.config_name, tgt.file_base())
    with open(src.abspath(), 'rb') as f:
        load.load(f, cfg)
    with open(tgt.abspath(task.env), 'wt', encoding='utf-8') as f:
        with textast.Ast() as ast:
            hgen.GenHCode(cfg).make(ast)
        f.write(str(ast))
    tgt = task.outputs[1]
    cfg = core.Config(task.config_name, tgt.file_base())
    with open(src.abspath(), 'rb') as f:
        load.load(f, cfg)
    with open(tgt.abspath(task.env), 'wt', encoding='utf-8') as f:
        with textast.Ast() as ast:
            cgen.GenCCode(cfg).make(ast)
        f.write(str(ast))

Task.task_type_from_func('coyaml_gen', color='BLUE',
    func=coyaml_gen, ext_in='.yaml', ext_out=['.c', '.h'])

@TaskGen.feature('coyaml')
@TaskGen.before('apply_core')
def process_coyaml(self):
    filename = getattr(self, 'config', '')
    if not filename:
        raise ValueError('Cannot process coyaml without "config" attribute.')
    node = self.path.find_resource(filename)
    if not node:
        raise ValueError('Cannot find {0} in {1}'
            .format(filename, self.path.abspath()))
    c_node = node.change_ext('.c')
    h_node = node.change_ext('.h')
    task = self.create_task('coyaml_gen', [node], [h_node, c_node])
    task.config_name = getattr(self, 'config_name', 'config')
    self.allnodes.append(c_node)
