from .core import Config

def simple():
    from optparse import OptionParser
    op = OptionParser(usage="\n    %prog\n    %prog -c config.yaml")
    op.add_option('-c', '--config', metavar="FILENAME",
        help="Configuration file to parse",
        dest="configfile", default=None, type="string")
    op.add_option('-n', '--name', metavar="NAME",
        help="Name of configuration (default `config`), usefull if you have"
            "several configuration in single binary",
        dest="name", default="config", type="string")
    op.add_option('-f', '--filename', metavar="NAME",
        help="Filename to read",
        dest="filename", default="config", type="string")
    op.add_option('-p', '--print',
        help="Print parsed configuration file",
        dest="print", default=False, action="store_true")
    options, args = op.parse_args()
    if args:
        op.error("No arguments expected")
    cfg = Config(options.name, options.filename)
    if options.configfile:
        inp = open(options.configfile, 'rt', encoding='utf-8')
    else:
        import sys
        inp = sys.stdin
    return cfg, inp, options
