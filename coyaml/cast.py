import re
from .textast import Node, VSpace, ListConstraint as List, lazy, Ast
from .cutil import string
from collections import OrderedDict

__all__ = [
    'VSpace',
    'CommentBlock',
    'Include', 'StdInclude', 'Define', 'Ifdef', 'Ifndef', 'Endif', 'Macro',
    'Ident',
    'TypeDef', 'Typename', 'Struct', 'AnonStruct', 'Void',
    'Enum', 'EnumItem', 'EnumVal',
    'Param', 'Var', 'FVar', 'VarAssign', 'Assign',
    'Arr', 'ArrArr', 'StrValue',
    'Member', 'Dot', 'Subscript', 'Ref', 'Deref',
    'Expression', 'Statement',
    'For', 'If', 'Return',
    'Func', 'Function', 'Call',
    'Int', 'Float', 'String', 'Coerce',
    'Add', 'Mul', 'Div', 'Sub', 'Not', 'Ternary',
    'Gt', 'Lt', 'Ge', 'Le', 'Eq', 'Neq', 'And', 'Or',
    'NULL',
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
        ('name', (Ident, Dot)),
        ])
    line_format = '{source}->{name}'

class Subscript(Node):
    __slots__ = OrderedDict([
        ('source', lazy.Expression),
        ('expr', lazy.Expression),
        ])
    line_format = '{source}[{expr}]'

class Add(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left}+{right}'

class Sub(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left}-{right}'

class Mul(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left}*{right}'

class Div(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left}/{right}'

class Eq(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} == {right}'

class Neq(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} != {right}'

class Lt(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} < {right}'

class Gt(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} > {right}'

class Le(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} <= {right}'

class Ge(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} >= {right}'

class And(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} && {right}'

class Or(Node):
    __slots__ = OrderedDict([
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '{left} || {right}'

class Ternary(Node):
    __slots__ = OrderedDict([
        ('cond', lazy.Expression),
        ('left', lazy.Expression),
        ('right', lazy.Expression),
        ])
    line_format = '({cond}) ? {left} : {right}'

class Not(Node):
    __slots__ = OrderedDict([
        ('expr', lazy.Expression),
        ])
    line_format = '!{expr}'

class Deref(Node):
    __slots__ = OrderedDict([
        ('source', lazy.Expression),
        ])
    line_format = '*{source}'

class Ref(Node):
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
        (?:char|short|int|long|bool|float|double|void|\w+_t|\w+_fun|\w+_enum|FILE)
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

_type = (Typename, Void, lazy.Struct, lazy.AnonStruct, lazy.Enum)

class Coerce(Node):
    __slots__ = OrderedDict([
        ('type', _type),
        ('expr', lazy.Expression),
        ])
    line_format = '({type}){expr}'

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

class Define(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ])
    top = True
    line_format = '#define {name}'

class Macro(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ('args', List(Ident)),
        ('subst', str),
        ])
    top = True
    line_format = '#define {name}({args}) {subst}'

class Ifdef(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ])
    top = True
    line_format = '#ifdef {name}'

class Ifndef(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ])
    top = True
    line_format = '#ifndef {name}'

class Endif(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ])
    top = True
    line_format = '#endif //{name}'

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
        ('expr', (Ident, Int, Float, String, Dot, Member, Ref, Deref, Subscript,
            Add, Sub, Mul, Div, Not, Coerce,
            Lt, Gt, Le, Ge, Eq, Neq, And, Or, Ternary,
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

class Return(Var):
    __slots__ = OrderedDict([
        ('expr', lazy.Expression),
        ])
    top = True
    line_format = 'return {expr};'

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
    block_start = 'struct {name} {{'
    block_end = '}}'

class AnonStruct(Node):
    __slots__ = OrderedDict([
        ('body', List(Var)),
        ])
    block_start = 'struct {{'
    block_end = '}}'

class EnumItem(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ])
    line_format = '{name},'

class EnumVal(Node):
    __slots__ = OrderedDict([
        ('name', Ident),
        ('val', (Int, lazy.Expression)),
        ])
    line_format = '{name} = {val},'

class Enum(Node):
    __slots__ = OrderedDict([
        ('body', List(EnumItem, EnumVal)),
        ])
    block_start = 'enum {{'
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
    block_start = 'if({cond}) {{'
    block_end = '}}'

lazy.fix(globals())

NULL = Ident('NULL')

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
