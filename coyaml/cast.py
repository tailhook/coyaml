import re
from .textast import Node, VSpace, ListConstraint as List, lazy, Ast
from .cutil import string
from collections import OrderedDict

__ast__ = [
    'Ident',
    'Typename',
    'CommentBlock',
    'StdInclude',
    'Var',
    'Param',
    'Struct',
    'TypeDef',
    'Arr',
    'ArrArr',
    'Expression',
    'For',
    'If',
    ]

class Ident(Node):
    __slots__ = OrderedDict([
        ('value', str),
        ])
    re_ident = re.compile('^[a-zA-Z_][0-9a-zA-Z_]*$')
    def __init__(self, value):
        super(Ident, self).__init__(value)
        assert isinstance(value, str), value
        assert self.re_ident.match(value), value

class Dot(Node):
    __slots__ = OrderedDict([
        ('source', lazy.Expression),
        ('name', Ident),
        ])
    line_format = '{source}.{name}'

class Member(Node):
    __slots__ = OrderedDict([
        ('source', lazy.Expression),
        ('name', Ident),
        ])
    line_format = '{source}->{name}'

class Deref(Node):
    __slots__ = OrderedDict([
        ('source', lazy.Expression),
        ])
    line_format = '&{source}'

class Typename(Node):
    __slots__ = OrderedDict([
        ('value', str),
        ])
    re_typename = re.compile('''^
        (?:(?:const|static)\s+)?
        (?:(?:unsigned|signed)\s+)?
        (?:char|short|int|long|bool|float|double|\w+_t|FILE)
        \s*(?:\*\s*)* |
        struct\s+\w+
        \s*(?:\*\s*)*
        $''', re.X)
    def __init__(self, value):
        super(Typename, self).__init__(value)
        assert self.re_typename.match(self.value), self.value

class Void(Node):
    __slots__ = {}
    line_format = 'void'

_type = (Typename, Void, lazy.Struct, lazy.AnonStruct)

class CommentBlock(Node):
    __slots__ = OrderedDict([
        ('lines', List(str)),
        ])
    top = True
    each_line = '/* {0:s} */'

    def __init__(self, *lines):
        super(CommentBlock, self).__init__(lines)

class StdInclude(Node):
    __slots__ = OrderedDict([
        ('filename', str),
        ])
    top = True
    line_format = '#include <{filename}>'

class Include(Node):
    __slots__ = OrderedDict([
        ('filename', str),
        ])
    top = True
    line_format = '#include "{filename}"'

class Var(Node):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ('array', tuple),
        ('static', bool),
        ])
    top = True
    line_format = '{static }{type} {name}{array};'

    def fmt_static(self):
        return 'static' if getattr(self, 'static', None) else ''

    def fmt_array(self):
        return ''.join('[]' if a is None else '[{0:d}]'.format(a)
            for a in getattr(self, 'array', ()))

class Int(Node):
    __slots__ = OrderedDict([
        ('value', int),
        ])

class Float(Node):
    __slots__ = OrderedDict([
        ('value', float),
        ])

class String(Node):
    __slots__ = OrderedDict([
        ('value', str),
        ])
    def format(self, stream):
        stream.write(string(self.value))

class Expression(Node):
    __slots__ = OrderedDict([
        ('expr', (Ident, Int, Float, String, Dot, Member,
            lazy.Call, lazy.StrValue, lazy.Assign)),
        ])
    line_format = '{expr}'

class StrValue(Node):
    __slots__ = OrderedDict([
        ('items', object),
        ])
    def __init__(self, **kw):
        super(StrValue, self).__init__()
        self.items = dict((k, (v if isinstance(v, Expression)
            else Expression(v))) for k, v in kw.items())
        assert all(isinstance(v, Expression) for v in self.items.values()), \
            self.items

    def format(self, stream):
        it = iter(self.items.items())
        try:
            k, v = next(it)
        except StopIteration:
            return
        stream.write('{' + k + ': ')
        v.format(stream)
        for k, v in it:
            stream.write(', ' + k + ': ')
            v.format(stream)
        stream.write('}')

class Arr(Node):
    __slots__ = OrderedDict([
        ('items', List(Expression, StrValue)),
        ])
    line_format = '{{{items}}}'

class ArrArr(Node):
    __slots__ = OrderedDict([
        ('items', List(Arr, lazy.ArrArr)),
        ])
    line_format = '{{{items}}}'

class VarAssign(Var):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ('expr', (Expression, Arr)),
        ('array', tuple),
        ('static', bool),
        ])
    top = True
    line_format = '{static }{type} {name}{array} = {expr};'

class FVar(Var):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ('expr', (Expression, Arr)),
        ])
    top = True
    line_format = '{type} {name} = {expr}'

class Assign(Var):
    __slots__ = OrderedDict([
        ('lvalue', Expression),
        ('rvalue', Expression),
        ])
    top = True
    line_format = '{lvalue} = {rvalue}'

class Param(Node):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ])
    line_format = '{type}{ name}'

class Struct(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ('body', List(Var)),
        ])
    top = True
    block_start = 'struct {name} {{'
    block_end = '}}'

class AnonStruct(Node):
    __slots__ = OrderedDict([
        ('body', List(Var)),
        ])
    top = True
    block_start = 'struct {{'
    block_end = '}}'

class TypeDef(Node):
    __slots__ = OrderedDict([
        ('definition', _type),
        ('name', Ident),
        ])
    top = True
    line_format = 'typedef {definition} {name};'

class Func(Node):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ('params', List(Var)),
        ])
    top = True
    line_format = '{type} {name}({params});'

class Call(Node):
    __slots__ = OrderedDict([
        ('expr', Expression),
        ('params', List(Expression)),
        ])
    line_format = '{expr}({params})'

class Statement(Node):
    __slots__ = OrderedDict([
        ('expr', Expression),
        ])
    line_format = '{expr};'

class Function(Node):
    __slots__ = OrderedDict([
        ('type', _type),
        ('name', Ident),
        ('params', List(Var)),
        ('body', List(Node)),
        ])
    top = True
    block_start = '{type} {name}({params}) {{'
    block_end = '}}'

class For(Node):
    __slots__ = OrderedDict([
        ('initial', FVar),
        ('cond', Expression),
        ('incr', Expression),
        ('body', List(Node)),
        ])
    top = True
    block_start = 'for({initial}; {cond}; {incr}) {{'
    block_end = '}}'

class If(Node):
    __slots__ = OrderedDict([
        ('cond', Expression),
        ('body', List(Node)),
        ])
    top = True
    block_start = 'If({cond}) {{'
    block_end = '}}'

lazy.fix(globals())

if __name__ == '__main__':
    def sample(ast):
        ast(CommentBlock(
            "THIS IS AUTOGENERATED FILE",
            "DO NOT EDIT!!!",
            ))
        ast(StdInclude('coyaml_hdr.h'))
        ast(VSpace())
        with ast(TypeDef(Struct('test_s', ast.block()),'test_t')) as ch:
            ch(Var(Typename('int'), 'intval'))
            ch(Var(Typename('unsigned long'), 'longval'))
            ch(Var(Typename('const char *'), 'stringval'))
    print(str(Ast(sample)))
