#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import Options, Scripting
import glob

APPNAME='coyaml'
VERSION='0.3.2'

top = '.'
out = 'build'

def options(opt):
    import distutils.sysconfig
    opt.load('compiler_c python')
    opt.add_option('--build-shared', action="store_true", dest="build_shared",
        help="Build shared library instead of static")
    opt.add_option('--build-tests', action="store_true", dest="build_tests",
        help="Build test cases")
    opt.add_option('--run-tests', action="store_true", dest="run_tests",
        help="Run test cases")
    #opt.add_option('--python-lib', type="string", dest="python_lib",
    #    help="Path to python site-packages directory",
    #    default=distutils.sysconfig.get_python_lib())


def configure(conf):
    import coyaml.waf
    conf.load('compiler_c python')
    conf.check_python_version((3,0,0))
    conf.env.BUILD_TESTS = Options.options.build_tests
    conf.env.BUILD_SHARED = Options.options.build_shared

def build(bld):
    bld(
        features     = ['c', ('cshlib'
            if bld.env.BUILD_SHARED else 'cstlib')],
        source       = [
            'src/parser.c',
            'src/commandline.c',
            'src/helpers.c',
            'src/vars.c',
            'src/types.c',
            'src/emitter.c',
            'src/copy.c',
            ],
        target       = 'coyaml',
        includes     = ['include', 'src'],
        defines      = ['COYAML_VERSION="%s"' % VERSION],
        cflags      = ['-std=c99'],
        lib          = ['yaml'],
        )
    if bld.env.BUILD_SHARED:
        bld.install_files('${PREFIX}/lib', ['libcoyaml.so'])
    else:
        bld.install_files('${PREFIX}/lib', ['libcoyaml.a'])
    bld.install_files('${PREFIX}/include', [
        'include/coyaml_hdr.h',
        'include/coyaml_src.h',
        ])
    bld(features='py',
        source=glob.glob('coyaml/*.py'),
        install_path='${PYTHONDIR}/coyaml')
    bld.install_files('${PREFIX}/bin', 'scripts/coyaml', chmod=0o755)
    if bld.env.BUILD_TESTS:
        import coyaml.waf
        bld.add_group()
        bld(
            features     = ['c', 'cprogram', 'coyaml'],
            source       = [
                'test/tinytest.c',
                'test/tinyconfig.yaml',
                ],
            target       = 'tinytest',
            includes     = ['include', 'test'],
            cflags      = ['-std=c99'],
            #libpath      = [bld.bdir+'/default'],
            lib          = ['coyaml', 'yaml'],
            )
        bld(
            features     = ['c', 'cprogram', 'coyaml'],
            source       = [
                'test/compr.c',
                'test/comprehensive.yaml',
                ],
            target       = 'compr',
            includes     = ['include', 'test'],
            cflags      = ['-std=c99'],
            #libpath      = [bld.bdir+'/default'],
            lib          = ['coyaml', 'yaml'],
            config_name  = 'cfg',
            )
        bld(
            features     = ['c', 'cprogram', 'coyaml'],
            source       = [
                'test/recursive.c',
                'test/recconfig.yaml',
                ],
            target       = 'recursive',
            includes     = ['include', 'test'],
            cflags      = ['-std=c99'],
            #libpath      = [bld.bdir+'/default'],
            lib          = ['coyaml', 'yaml'],
            config_name  = 'cfg',
            )
        if Options.options.run_tests:
            rule = ('./default/{0} -c ../examples/{1}.yaml -C -P > /tmp/{0};'
                'diff -u /tmp/{0} ../examples/{1}.out')
            bld(rule=rule.format('tinytest', 'tinyexample'),
                always=True)
            bld(rule='./default/compr -c ../examples/compexample.yaml'
                ' --config-var clivar=CLI -C -P > /tmp/compr1;'
                ' diff -u /tmp/compr1 ../examples/compexample.out',
                always=True)
            bld(rule='./default/compr -c ../examples/compexample.yaml'
                ' -Dclivar=CLI > /tmp/compr2;'
                ' diff -u /tmp/compr2 ../examples/compr.out',
                always=True)
            bld(rule=rule.format('recursive', 'recexample'),
                always=True)

def test(ctx):
    Scripting.commands += ['build']
    Options.options.build_tests = True
    Options.options.run_tests = True
