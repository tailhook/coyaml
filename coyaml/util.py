from contextlib import contextmanager
from string import digits
import re
import sys

re_int = re.compile("""\s*(-?\s*\d+)\s*([kMGTPE]i?)?\s*$""")
re_float = re.compile("""\s*(-?\s*\d+(?:\.\d+)?)\s*([kMGTPE]i?)?\s*$""")
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


def parse_float(value):
    if isinstance(value, (float, int)):
        return value
    m = re_float.match(value)
    if not m:
        raise TypeError("Can't convert ``{0}'' to float".format(value))
    res = float(m.group(1))
    if m.group(2):
        res *= units[m.group(2)]
    return res


@contextmanager
def nested(*managers):
    """nested decorator stolen from old python for python 3.2 compatibility"""
    exits = []
    vars = []
    exc = (None, None, None)
    try:
        for mgr in managers:
            exit = mgr.__exit__
            enter = mgr.__enter__
            vars.append(enter())
            exits.append(exit)
        yield vars
    except:
        exc = sys.exc_info()
    finally:
        while exits:
            exit = exits.pop()
            try:
                if exit(*exc):
                    exc = (None, None, None)
            except:
                exc = sys.exc_info()
        if exc != (None, None, None):
            # Don't rely on sys.exc_info() still containing
            # the right information. Another exception may
            # have been raised and caught by an exit method
            raise exc[0]
