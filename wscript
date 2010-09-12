#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os.path
import Options, Scripting

APPNAME='coyaml'
VERSION='0.1'

top = '.'
out = 'build'

def set_options(opt):
    import distutils.sysconfig
    opt.tool_options('compiler_cc')
    opt.add_option('--build-shared', action="store_true", dest="build_shared",
        help="Build shared library instead of static")
    opt.add_option('--build-tests', action="store_true", dest="build_tests",
        help="Build test cases")
    opt.add_option('--python-lib', type="string", dest="python_lib",
        help="Path to python site-packages directory",
        default=distutils.sysconfig.get_python_lib())


def configure(conf):
    conf.check_tool('compiler_cc')

def singleconfig(bld, src, trg):
    bld(
        target=trg+'cfg.h',
        rule=makeheader,
        source=src,
        )
    bld(
        target=trg+'cfg.c',
        rule=makecode,
        source=src,
        )
    bld.add_group()
    bld(
        features     = ['cc', 'cprogram'],
        source       = [
            'test/'+trg+'.c',
            trg+'cfg.c',
            ],
        target       = trg,
        includes     = ['include', 'src', bld.bdir + '/default'],
        ccflags      = ['-std=c99'],
        libpath      = [bld.bdir+'/default'],
        lib          = ['coyaml', 'yaml'],
        )


def build(bld):
    bld(
        features     = ['cc', ('cshlib'
            if Options.options.build_shared else 'cstaticlib')],
        source       = [
            'src/parser.c',
            'src/commandline.c',
            ],
        target       = 'coyaml',
        includes     = ['include', 'src', bld.bdir + '/default'],
        defines      = [],
        ccflags      = ['-std=c99'],
        lib          = ['yaml'],
        )
    if Options.options.build_shared:
        bld.install_files('${PREFIX}/lib', [bld.bdir+'/default/libcoyaml.so'])
    else:
        bld.install_files('${PREFIX}/lib', [bld.bdir+'/default/libcoyaml.a'])
    bld.install_files('${PREFIX}/include', [
        'include/coyaml_hdr.h',
        'include/coyaml_src.h',
        ])
    bld.install_files(Options.options.python_lib+'/coyaml', 'coyaml/*.py')
    bld.install_files('${PREFIX}/bin', 'scripts/coyaml', chmod=0o755)
    if Options.options.build_tests:
        singleconfig(bld, 'examples/tinyconfig.yaml', 'tinytest')
        singleconfig(bld, 'examples/comprehensive.yaml', 'comprehensive')
        singleconfig(bld, 'examples/recconfig.yaml', 'recursive')

def test(ctx):
    Scripting.commands += ['build']
    Options.options.build_tests = True

def makeheader(task):
    import coyaml.cgen, coyaml.hgen, coyaml.core, coyaml.load
    src = task.inputs[0].srcpath(task.env)
    tgt = task.outputs[0].bldpath(task.env)
    cfg = coyaml.core.Config('cfg', os.path.splitext(os.path.basename(tgt))[0])
    with open(src, 'rb') as f:
        coyaml.load.load(f, cfg)
    with open(tgt, 'wt', encoding='utf-8') as f:
        with coyaml.textast.Ast() as ast:
            coyaml.hgen.GenHCode(cfg).make(ast)
        f.write(str(ast))

def makecode(task):
    import coyaml.cgen, coyaml.hgen, coyaml.core, coyaml.load
    src = task.inputs[0].srcpath(task.env)
    tgt = task.outputs[0].bldpath(task.env)
    cfg = coyaml.core.Config('cfg', os.path.splitext(os.path.basename(tgt))[0])
    with open(src, 'rb') as f:
        coyaml.load.load(f, cfg)
    with open(tgt, 'wt', encoding='utf-8') as f:
        with coyaml.textast.Ast() as ast:
            coyaml.cgen.GenCCode(cfg).make(ast)
        f.write(str(ast))
