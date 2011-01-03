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
    load.UInt: 'unsigned long',
    load.Float: 'double',
    load.Bool: 'int',
    load.VoidPtr: 'void *',
    }
_typenames = types.copy()
_typenames.update({
    load.String: 'string',
    load.File: 'File',
    load.Dir: 'Dir',
    load.VoidPtr: 'void',
    })
_typenames.update(types)
string_types = (load.File, load.String, load.Dir)

def string(val):
    return '"{0}"'.format(repr(val)[1:-1].replace('"', r'\"'))

def typename(typ):
    if isinstance(typ, load.Struct):
        return typ.type
    elif isinstance(typ, load.CType):
        return typ.type
    elif isinstance(typ, load.CStruct):
        return 'struct ' + typ.structname
    return _typenames[typ.__class__]

def cbool(val):
    return 'TRUE' if val else 'FALSE'

def makevar(val):
    return varname(val).replace('.', '_').replace(' ', '_')
