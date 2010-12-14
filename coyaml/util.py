from string import digits
import re

re_int = re.compile("""\s*(\d+)\s*([kMGTPE]i?)?\s*$""")
units = {
    "k": 1000,
    "ki": 1 << 10,
    "M": 1000000,
    "Mi": 1 << 20,
    "G": 1000000000,
    "Gi": 1 << 30,
    "T": 1000000000000,
    "Ti": 1 << 40,
    "P": 1000000000000000,
    "Pi": 1 << 50,
    "E": 1000000000000000000,
    "Ei": 1 << 60,
    }
    
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
    'static',
    }

builtin_conversions = set([
    'coyaml_tagged_scalar',
    ])

def varname(value):
    value = value.replace('-', '_')
    if value[0] in digits:
        value = '_'+value
    if value in reserved:
        value = value + '_'
    return value
    
def parse_int(value):
    if isinstance(value, int):
        return value
    m = re_int.match(value)
    if not m:
        raise TypeError("Can't convert ``{0}'' to int".format(value))
    res = int(m.group(1))
    if m.group(2):
        res *= units[m.group(2)]
    return res
