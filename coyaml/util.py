from string import digits

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

def varname(value):
    value = value.replace('-', '_')
    if value[0] in digits:
        value = '_'+value
    if value in reserved:
        value = value + '_'
    return value

def cstring(val):
    return '"{0}"'.format(repr(val)[1:-1].replace('"', r'\"'))
