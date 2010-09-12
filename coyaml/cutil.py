from string import digits

from . import load
from .util import varname

reserved = {
    'class',
    'default',
    'int',
    'usigned',
    'long',
    'float',
    'double',
    'char',
    'bool',
    }

types = {
    load.Int: 'long',
    load.UInt: 'size_t',
    load.Float: 'double',
    load.Bool: 'int',
    }
_typenames = {
    load.String: 'string',
    }
_typenames.update(types)
string_types = (load.File, load.String, load.Dir)

def string(val):
    return '"{0}"'.format(repr(val)[1:-1].replace('"', r'\"'))

def typename(typ):
    if isinstance(typ, load.Struct):
        return typ.type
    return _typenames[typ.__class__]

def cbool(val):
    return 'TRUE' if val else 'FALSE'

def makevar(val):
    return varname(val).replace('.', '_').replace(' ', '_')
