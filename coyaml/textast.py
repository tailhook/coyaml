from io import StringIO
import types
import re
from contextlib import contextmanager, nested

class Node(object):
    __slots__ = ('_futures',)
    pattern = re.compile(r'(\{\{|\}\})|\{(\w+)\}')

    def __init__(self, *args, **kwargs):
        self._futures = []
        for val, (slotname, slottype) in zip(args, self.__slots__.items()):
            setattr(self, slotname, self._convert(val, slottype, slotname))
        for slotname, slottype in self.__slots__.items():
            val = kwargs.pop(slotname, None)
            if val is not None:
                setattr(self, slotname, self._convert(val, slottype, slotname))
        assert not kwargs

    def _convert(self, val, typ, name=None):
        if isinstance(val, _FutureChildren):
            self._futures.append(val)
            val = val.content
        elif isinstance(typ, CList):
            assert isinstance(val, (list, tuple)), val
            assert all(isinstance(i, typ.subtypes) for i in val), val
        elif not isinstance(val, typ):
            if isinstance(typ, tuple):
                val = typ[0](val)
            else:
                val = typ(val)
        elif isinstance(val, Node):
            self._futures.extend(val._futures)
        return val

    def format(self, stream):
        if hasattr(self, 'each_line'):
            for line in self.lines:
                stream.line(self.each_line.format(line))
        if hasattr(self, 'value'):
            stream.write(self.value)
        if hasattr(self, 'line_format'):
            with stream.complexline():
                self._format_line(self.line_format, stream)
        if hasattr(self, 'block_start'):
            with stream.block() as block:
                with block.start():
                    self._format_line(self.block_start, stream)
                for i in self.body:
                    i.format(stream)
                with block.finish():
                    self._format_line(self.block_end, stream)

    def _format_line(self, pattern, stream):
        pieces = self.pattern.split(pattern)
        for (i, val) in enumerate(pieces):
            if i % 3 == 0:
                stream.write(val)
                continue
            elif i % 3 == 1:
                if val:
                    stream.write(val[0])
            elif i % 3 == 2:
                if val:
                    v = getattr(self, val)
                    if isinstance(v, str):
                        stream.write(v)
                    else:
                        v.format(stream)


class VSpace(Node):
    __slots__ = {}
    def format(self, stream):
        stream.write('\n')
        stream.line_start = True

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
            if not isinstance(node, type) \
                or not issubclass(node, Node) or node is Node:
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

lazy = _Lazy() # Hi, I'm a singleton.

class _FutureChildren(object):
    def __init__(self):
        self.content = []

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        pass

    def __call__(self, node):
        self.content.append(node)
        if node._futures:
            if len(node._futures) > 1:
                return nested(*node._futures)
            else:
                return node._futures[0]

    def block(self):
        return _FutureChildren()

class _Block(object):
    def __init__(self, stream, indent, full_line):
        self.stream = stream
        self.indent = indent
        self.full_line = full_line

    @contextmanager
    def start(self):
        if self.full_line:
            self.stream.buffer.write(self.indent)
        yield
        self.stream.buffer.write('\n')
        self.stream.line_start = True

    @contextmanager
    def finish(self):
        self.stream.buffer.write(self.indent)
        yield
        if self.full_line:
            self.stream.buffer.write('\n')
            self.stream.line_start = True

class _Stream(object):
    __slots__ = ('ast','indent_kind', 'indent', 'buffer', 'line_start')

    def __init__(self, ast, indent_kind='    '):
        self.ast = ast
        self.buffer = StringIO()
        self.indent_kind = indent_kind
        self.indent = 0
        self.line_start = True

    def write(self, data):
        self.buffer.write(data)

    def line(self, line):
        assert self.line_start
        if not line.endswith('\n'):
            line += '\n'
        if self.indent:
            line = self.indent_kind * self.indent + line
        self.buffer.write(line)
        self.line_start = True

    def getvalue(self):
        return self.buffer.getvalue()

    @contextmanager
    def complexline(self):
        full_line = self.line_start
        if full_line:
            self.buffer.write(self.indent_kind * self.indent)
            self.line_start = False
        yield
        if full_line:
            self.buffer.write('\n')
            self.line_start = True

    @contextmanager
    def block(self):
        if self.line_start:
            self.buffer.write(self.indent_kind * self.indent)
        block = _Block(self, self.indent_kind*self.indent, self.line_start)
        self.indent += 1
        yield block
        self.indent -= 1

class Ast(object):
    def __init__(self):
        self.body = []

    def __str__(self):
        stream = _Stream(self)
        for i in self.body:
            i.format(stream)
        return stream.getvalue()

    def block(self):
        return _FutureChildren()

    def __call__(self, node):
        self.body.append(node)
        if node._futures:
            if len(node._futures) > 1:
                return nested(*node._futures)
            else:
                return node._futures[0]

def flatten(gen):
    stream = _Stream()
    for val in gen:
        stream.add(val)
    return buf.getvalue()
