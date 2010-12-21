import collections

import yaml


class Group(collections.OrderedDict):
    __slots__ = ('start_mark',)

class ConfigLoader(yaml.Loader):
    """Config loader with yaml tags"""

    def construct_yaml_map(self, node):
        data = Group()
        data.start_mark = node.start_mark
        yield data
        value = self.construct_mapping(node)
        data.update(value)

    def construct_mapping(self, node, deep=False):
        if isinstance(node, yaml.MappingNode):
            self.flatten_mapping(node)
        return collections.OrderedDict(self.construct_pairs(node, deep=deep))

yaml.add_constructor('tag:yaml.org,2002:map',
    ConfigLoader.construct_yaml_map, Loader=ConfigLoader)

class YamlyType(yaml.YAMLObject):

    def __init__(self, default=None):
        self.default_ = default
        self.inheritance = None

    @classmethod
    def from_yaml(cls, Loader, node):
        if isinstance(node, yaml.ScalarNode):
            self = cls(default=Loader.construct_scalar(node))
        else:
            self = cls.__new__(cls)
            self.__setstate__(Loader.construct_mapping(node))
        self.start_mark = node.start_mark
        return self

    def __setstate__(self, state):
        self.inheritance = None
        default = state.pop('=', None)
        for k, v in state.items():
            setattr(self, varname(k), v)
        if default is not None:
            self.default_ = default

class Int(YamlyType):
    yaml_tag = '!Int'
    yaml_loader = ConfigLoader

class UInt(YamlyType):
    yaml_tag = '!UInt'
    yaml_loader = ConfigLoader

class Bool(YamlyType):
    yaml_tag = '!Bool'
    yaml_loader = ConfigLoader

class Float(YamlyType):
    yaml_tag = '!Float'
    yaml_loader = ConfigLoader

class String(YamlyType):
    yaml_tag = '!String'
    yaml_loader = ConfigLoader

class File(YamlyType):
    yaml_tag = '!File'
    yaml_loader = ConfigLoader

class Dir(YamlyType):
    yaml_tag = '!Dir'
    yaml_loader = ConfigLoader

class Struct(yaml.YAMLObject):
    yaml_tag = '!Struct'
    yaml_loader = ConfigLoader

    def __init__(self, type):
        self.type = type

    @classmethod
    def from_yaml(cls, Loader, node):
        if isinstance(node, yaml.ScalarNode):
            self = cls(type=Loader.construct_scalar(node))
        else:
            self = cls.__new__(cls)
            self.__setstate__(Loader.construct_mapping(node))
        self.start_mark = node.start_mark
        return self

    def __setstate__(self, state):
        typ = state.pop('=', None)
        for k, v in state.items():
            setattr(self, varname(k), v)
        if typ is not None:
            self.type = typ

class Mapping(YamlyType):
    yaml_tag = '!Mapping'
    yaml_loader = ConfigLoader

class Array(YamlyType):
    yaml_tag = '!Array'
    yaml_loader = ConfigLoader

class Convert(yaml.YAMLObject):
    yaml_tag = '!Convert'
    yaml_loader = ConfigLoader

    def __init__(self, fun):
        self.fun = fun

    @classmethod
    def from_yaml(cls, Loader, node):
        return cls(Loader.construct_scalar(node))

class VoidPtr(YamlyType):
    yaml_tag = '!_VoidPtr'
    yaml_loader = ConfigLoader
    
class CStruct(YamlyType):
    yaml_tag = '!CStruct'
    yaml_loader = ConfigLoader
    
    def __init__(self, type):
        self.structname = type
    
    @classmethod
    def from_yaml(cls, Loader, node):
        if isinstance(node, yaml.ScalarNode):
            self = cls(type=Loader.construct_scalar(node))
        else:
            self = cls(**Loader.construct_mapping(node))
        return self
    
class CType(YamlyType):
    yaml_tag = '!CType'
    yaml_loader = ConfigLoader
    
    def __init__(self, type):
        self.type = type
    
    @classmethod
    def from_yaml(cls, Loader, node):
        if isinstance(node, yaml.ScalarNode):
            self = cls(type=Loader.construct_scalar(node))
        else:
            self = cls(**Loader.construct_mapping(node))
        return self

from .core import Config, Usertype # sorry, circular dependency
from .util import varname

def load(input, config):
    data = yaml.load(input, Loader=ConfigLoader)
    config.fill_meta(data.pop('__meta__', {}))
    for k, v in data.pop('__types__', {}).items():
        typ = Usertype(k, v, start_mark=v.start_mark)
        config.add_type(typ)
    config.fill_data(data)

def main():
    from .cli import simple
    cfg, inp, opt = simple()
    with inp:
        load(inp, cfg)
    if opt.print:
        cfg.print()

if __name__ == '__main__':
    main()
