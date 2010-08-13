
class Usertype(object):
    def __init__(self, name, members, **kw):
        self.name = name
        self.members = {k:v for k, v in members.items()
            if not k.startswith('__')}
        for k, v in kw.items():
            setattr(self, k, v)

class ConfigMeta(object):
    def update(self, dic):
        for k, v in dic.items():
            setattr(self, k, v)

class Config(object):
    def __init__(self, name):
        self.name = name
        self.meta = ConfigMeta()
        self.types = {}

    def fill_meta(self, meta):
        self.meta.update(meta)

    def add_type(self, typ):
        assert not typ.name in self.types
        self.types[typ.name] = typ

    def fill_data(self, data):
        self.data = data

    def print(self):
        import pprint
        pprint.pprint(self.meta)
        pprint.pprint(self.types)
        pprint.pprint(self.data)
