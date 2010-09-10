#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import coyaml.cgen, coyaml.hgen, coyaml.core, coyaml.load
import os.path

APPNAME='coyamltest'
VERSION='0.1'

top = '.'
out = 'build'

def set_options(opt):
    opt.tool_options('compiler_cc')

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
            'src/parser.c',
            'src/commandline.c',
            trg+'cfg.c',
            ],
        target       = trg,
        includes     = ['include', 'src', bld.bdir + '/default'],
        defines      = [],
        ccflags      = ['-std=c99', '-g'],
        lib          = ['yaml'],
        )


def build(bld):
    singleconfig(bld, 'examples/tinyconfig.yaml', 'tinytest')
    singleconfig(bld, 'examples/comprehensive.yaml', 'comprehensive')
    singleconfig(bld, 'examples/recconfig.yaml', 'recursive')

def makeheader(task):
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
    src = task.inputs[0].srcpath(task.env)
    tgt = task.outputs[0].bldpath(task.env)
    cfg = coyaml.core.Config('cfg', os.path.splitext(os.path.basename(tgt))[0])
    with open(src, 'rb') as f:
        coyaml.load.load(f, cfg)
    with open(tgt, 'wt', encoding='utf-8') as f:
        with coyaml.textast.Ast() as ast:
            coyaml.cgen.GenCCode(cfg).make(ast)
        f.write(str(ast))
