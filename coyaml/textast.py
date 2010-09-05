from io import StringIO
import types
from contextlib import contextmanager

class Node(object):
    __slots__ = {'text_body': str}

    def __init__(self, *args, **kwargs):
        for val, (slotname, slottype) in zip(args, self.__slots__.items()):
            if isinstance(slottype, CList):
                assert isinstance(val, (list, tuple))
                assert all(isinstance(i, slottype.subtypes) for i in val)
            elif not isinstance(val, slottype):
                val = slottype(val)
            setattr(self, slotname, val)
        for slotname, slottype in self.__slots__.items():
            val = kwargs.pop(slotname, None)
            if val is not None:
                if isinstance(slottype, CList):
                    assert isinstance(val, (list, tuple))
                    assert all(isinstance(i, slottype.subtypes) for i in val)
                elif not isinstance(val, slottype):
                    val = slottype(val)
                setattr(self, slotname, val)
        assert not kwargs

class VSpace(Node):
    __slots__ = {}
    def __flatten__(self):
        yield ''

class CList(object):
    __slots__ = ('subtypes')
    def __init__(self, *subtypes):
        self.subtypes = subtypes

class _LazyRef(object):
    __slots__ = ('name',)
    def __init__(self, name):
        self.name = name

class _Lazy(object):

    def __init__(self):
        self.items = set()

    def __getattr__(self, name):
        self.items.add(name)
        return _LazyRef(name)

    def fix(self, dic):
        for node in dic.values():
            if not isinstance(node, type) or not issubclass(node, Node):
                continue
            for k, v in node.__slots__.items():
                if isinstance(v, tuple):
                    v1 = tuple(
                        dic[item.name] if isinstance(item, _LazyRef) else item
                        for item in v)
                    if v1 != v:
                        node.__slots__[k] = v1
                else:
                    if isinstance(v, _LazyRef):
                        node.__slots__[k] = dic[v.name]

lazy = _Lazy()

class _Stream(object):

    def __init__(self, indent_kind='    '):
        self.buf = StringIO()
        self.indent = 0
        self.indent_kind = indent_kind

class Ast(object):
    def __init__(self, callable, indent_kind='    '):
        self.callable = callable
        self.indent = 0
        self.indent_kind = indent_kind

    def __str__(self):
        self.buf = StringIO()
        try:
            val = self.callable(self)
            if isinstance(val, types.GeneratorType):
                for i in val:
                    pass
            return self.buf.getvalue()
        finally:
            del self.buf

    def line(self, line):
        if not line.endswith('\n'):
            line += '\n'
        if self.indent:
            line = self.indent_kind * self.indent + line
        self.buf.write(line)

    def _buffer(self, val):
        o = self.buf
        self.buf = StringIO()
        try:
            self(val)
            return self.buf.getvalue()
        finally:
            self.buf = o

    def _fill(self, node):
        res = {}
        for k in node.__slots__:
            if k == 'body':
                continue
            val = getattr(node, k)
            if isinstance(val, Node):
                if hasattr(val, 'text_body'):
                    val = val.text_body
                else:
                    val = self._buffer(val)
            res[k] = val
        return res

    @contextmanager
    def _wrapper(self, node):
        fill = self._fill(node)
        self.line(node.block_start.format(**fill))
        self.indent += 1
        yield self
        self.indent -= 1
        self.line(node.block_end.format(**fill))

    @contextmanager
    def body(self, node):
        fill = self._fill(node)
        oldbuf = self.buf
        self.buf = StringIO()
        self.line(node.block_start.format(**fill))
        self.indent += 1
        yield node
        self.indent -= 1
        self.line(node.block_end.format(**fill))
        node.text_body = self.buf.getvalue().strip()
        self.buf = oldbuf

    def __call__(self, node):
        if hasattr(node, 'each_line'):
            for line in node.lines:
                self.line(node.each_line.format(line, self._fill(node)))
        if hasattr(node, 'line_format'):
            self.line(node.line_format.format(**self._fill(node)))
        if hasattr(node, 'value'):
            self.buf.write(node.value)
        return self._wrapper(node)

def flatten(gen):
    stream = _Stream()
    for val in gen:
        stream.add(val)
    return buf.getvalue()
