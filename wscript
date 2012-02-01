#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from waflib.Build import BuildContext
from waflib import Utils, Options
import os.path
import subprocess

APPNAME='coyaml'
if os.path.exists('.git'):
    VERSION=subprocess.getoutput('git describe').lstrip('v').replace('-', '_')
else:
    VERSION='0.3.14'

top = '.'
out = 'build'

def options(opt):
    import distutils.sysconfig
    opt.load('compiler_c python')
    opt.add_option('--build-shared', action="store_true", dest="build_shared",
        help="Build shared library instead of static")

def configure(conf):
    conf.load('compiler_c python')
    conf.check_python_version((3,0,0))
    conf.env.BUILD_SHARED = Options.options.build_shared

def build(bld):
    pass
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
            'src/eval.c',
            ],
        target       = 'coyaml',
        includes     = ['include', 'src'],
        defines      = ['COYAML_VERSION="%s"' % VERSION],
        cflags       = ['-std=c99', '-Wall'],
        lib          = ['yaml'],
        )
    if bld.env.BUILD_SHARED:
        bld.install_files('${PREFIX}/lib', 'libcoyaml.so')
    else:
        bld.install_files('${PREFIX}/lib', 'libcoyaml.a')
    bld.install_files('${PREFIX}/include', [
        'include/coyaml_hdr.h',
        'include/coyaml_src.h',
        ])
    bld(features='py',
        source=bld.path.ant_glob('coyaml/*.py'),
        install_path='${PYTHONDIR}/coyaml')
    bld.install_files('${PREFIX}/bin', 'scripts/coyaml', chmod=0o755)

def build_tests(bld):
    import coyaml.waf
    build(bld)
    bld.add_group()
    bld(
        features     = ['c', 'cprogram', 'coyaml'],
        source       = [
            'test/tinytest.c',
            'test/tinyconfig.yaml',
            ],
        target       = 'tinytest',
        includes     = ['include', 'test'],
        libpath      = ['.'],
        cflags       = ['-std=c99', '-Wall'],
        lib          = ['coyaml', 'yaml'],
        )
    bld(
        features     = ['c', 'cprogram', 'coyaml'],
        source       = [
            'test/vartest.c',
            'test/vars.yaml',
            ],
        target       = 'vartest',
        includes     = ['include', 'test'],
        libpath      = ['.'],
        cflags       = ['-std=c99', '-Wall'],
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
        libpath      = ['.'],
        cflags       = ['-std=c99', '-Wall'],
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
        libpath      = ['.'],
        cflags       = ['-std=c99', '-Wall'],
        lib          = ['coyaml', 'yaml'],
        config_name  = 'cfg',
        )
    bld.add_group()
    diff = 'diff -u ${SRC[0].abspath()} ${SRC[1]}'
    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -v -C -P > ${TGT[0]}',
        source=['tinytest', 'examples/tinyexample.yaml'],
        target='tinyexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/tinyexample.out', 'tinyexample.out'],
        always=True)

    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -C -P > ${TGT[0]}',
        source=['vartest', 'examples/varexample.yaml'],
        target='varexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/varexample.out', 'varexample.out'],
        always=True)

    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} --config-var clivar=CLI -C -P > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compexample.out.ws',
        always=True)
    bld(rule="sed -r 's/\s+$//g' ${SRC[0]} > ${TGT[0]}",
        source='compexample.out.ws',
        target='compexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/compexample.out', 'compexample.out'],
        always=True)

    bld(rule='COMPR_LOGLEVEL=7 ./${SRC[0]} -c ${SRC[1].abspath()} --config-var clivar=CLI -C -PP > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compexample.out.ws1',
        always=True)
    bld(rule="sed -r 's/\s+$//g' ${SRC[0]} > ${TGT[0]}",
        source='compexample.out.ws1',
        target='compexample.out1',
        always=True)
    bld(rule=diff,
        source=['examples/compexample_comments.out', 'compexample.out1'],
        always=True)

    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -C -P > ${TGT[0]}',
        source=['recursive', 'examples/recexample.yaml'],
        target='recexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/recexample.out', 'recexample.out'],
        always=True)
    bld(rule='COMPR_CFG=${SRC[1].abspath()} ./${SRC[0]} -Dclivar=CLI > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compr.out',
        always=True)
    bld(rule=diff,
        source=['examples/compr.out', 'compr.out'],
        always=True)

class test(BuildContext):
    cmd = 'test'
    fun = 'build_tests'
    variant = 'test'

def dist(ctx):
    ctx.excl = ['.waf*', '*.tar.bz2', '*.zip', 'build',
        '.git*', '.lock*', '**/*.pyc']
    ctx.algo = 'tar.bz2'

def make_pkgbuild(task):
    import hashlib
    task.outputs[0].write(Utils.subst_vars(task.inputs[0].read(), {
        'VERSION': VERSION,
        'DIST_MD5': hashlib.md5(task.inputs[1].read('rb')).hexdigest(),
        }))

class makepkg(BuildContext):
    cmd = 'archpkg'
    fun = 'archpkg'
    variant = 'archpkg'

def archpkg(bld):
    distfile = APPNAME + '-' + VERSION + '.tar.bz2'
    bld(rule=make_pkgbuild,
        source=['PKGBUILD.tpl', distfile, 'wscript'],
        target='PKGBUILD')
    bld(rule='cp ${SRC} ${TGT}', source=distfile, target='.')
    bld.add_group()
    bld(rule='makepkg -f', source=distfile)
    bld.add_group()
    bld(rule='makepkg -f --source', source=distfile)

def bumpver(ctx):
    ctx.exec_command(r"sed -ri.bak 's/(X-Version[^0-9]*)[0-9.]+/\1"+VERSION+"/'"
        " examples/compr.out examples/compexample.out")

def encode_multipart_formdata(fields, files):
    """
    fields is a sequence of (name, value) elements for regular form fields.
    files is a sequence of (name, filename, value) elements for data
    to be uploaded as files
    Return (content_type, body) ready for httplib.HTTP instance
    """
    BOUNDARY = b'----------ThIs_Is_tHe_bouNdaRY'
    CRLF = b'\r\n'
    L = []
    for (key, value) in fields:
        L.append(b'--' + BOUNDARY)
        L.append(('Content-Disposition: form-data; name="%s"' % key)
            .encode('utf-8'))
        L.append(b'')
        L.append(value.encode('utf-8'))
    for (key, filename, value, mime) in files:
        assert key == 'file'
        L.append(b'--' + BOUNDARY)
        L.append(b'Content-Type: ' + mime.encode('ascii'))
        L.append(('Content-Disposition: form-data; name="%s"; filename="%s"'
            % (key, filename)).encode('utf-8'))
        L.append(b'')
        L.append(value)
    L.append(b'--' + BOUNDARY + b'--')
    L.append(b'')
    body = CRLF.join(L)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY.decode('ascii')
    return content_type, body

def upload(ctx):
    "quick and dirty command to upload files to github"
    import hashlib
    import urllib.parse
    from http.client import HTTPSConnection, HTTPConnection
    import json
    distfile = APPNAME + '-' + VERSION + '.tar.bz2'
    with open(distfile, 'rb') as f:
        distdata = f.read()
    md5 = hashlib.md5(distdata).hexdigest()
    remotes = subprocess.getoutput('git remote -v')
    for r in remotes.splitlines():
        url = r.split()[1]
        if url.startswith('git@github.com:'):
            gh_repo = url[len('git@github.com:'):-len('.git')]
            break
    else:
        raise RuntimeError("repository not found")
    gh_token = subprocess.getoutput('git config github.token').strip()
    gh_login = subprocess.getoutput('git config github.user').strip()
    cli = HTTPSConnection('github.com')
    cli.request('POST', '/'+gh_repo+'/downloads',
        headers={'Host': 'github.com',
                 'Content-Type': 'application/x-www-form-urlencoded'},
        body=urllib.parse.urlencode({
            "file_name": distfile,
            "file_size": len(distdata),
            "description": APPNAME.title() + ' source v'
                + VERSION + " md5: " + md5,
            "login": gh_login,
            "token": gh_token,
        }).encode('utf-8'))
    resp = cli.getresponse()
    data = resp.read().decode('utf-8')
    data = json.loads(data)
    s3data = (
        ("key", data['path']),
        ("acl", data['acl']),
        ("success_action_status", "201"),
        ("Filename", distfile),
        ("AWSAccessKeyId", data['accesskeyid']),
        ("policy", data['policy']),
        ("signature", data['signature']),
        ("Content-Type", data['mime_type']),
        )
    ctype, body = encode_multipart_formdata(s3data, [
        ('file', distfile, distdata, data['mime_type']),
        ])
    cli.close()
    cli = HTTPSConnection('github.s3.amazonaws.com')
    cli.request('POST', '/',
                body=body,
                headers={'Content-Type': ctype,
                         'Host': 'github.s3.amazonaws.com'})
    resp = cli.getresponse()
    print(resp.read())
