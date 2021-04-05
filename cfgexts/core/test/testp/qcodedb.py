import sqlalchemy as db
from sqlalchemy.orm import sessionmaker, relationship, backref
from sqlalchemy.dialects.postgresql import JSON
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.ext.associationproxy import association_proxy
from sqlalchemy_utils import auto_delete_orphans
from sqlalchemy.orm.collections import attribute_mapped_collection

from unique_mixin import UniqueMixin

from functools import wraps

from cmodules import conv2python
import yaml
import pickle

#sqleng = db.create_engine(place, echo=True)

sqBase = declarative_base()

class SJSON(db.TypeDecorator):
    impl=JSON
    def process_bind_param(self, value, dialect):
        return conv2python(value)

tagassoc = db.Table('tagassoc', sqBase.metadata,
    db.Column('tags_id', db.Integer,db.ForeignKey('tags.id'),primary_key=True),
    db.Column('objects_id', db.Integer,db.ForeignKey('objects.id'),primary_key=True)
)

class Tag(sqBase):
    __tablename__ = 'tags'
    id = db.Column(db.Integer, primary_key=True)
    label = db.Column(db.String, unique=True)
    def __init__(self,label):
        self.label=label

# Auxilliary class for dynamic creation of relationships between DbObjects
class Ref(sqBase):
    __tablename__ = 'refs'
    parent_id = db.Column(db.Integer, db.ForeignKey('objects.id'), primary_key=True)
    ptr_id = db.Column(db.Integer, db.ForeignKey('objects.id'), primary_key=True)
    field = db.Column(db.String)
    parent = relationship('DbObject', 
                          backref=backref('_field_refs',
                                          collection_class=attribute_mapped_collection("field"),
                                          cascade="all, delete-orphan"
                                          ), foreign_keys=[parent_id])
    ptr = relationship('DbObject', foreign_keys=[ptr_id])

class DbObject(sqBase):
    __tablename__ = 'objects'
    id = db.Column(db.Integer, primary_key=True)
    classname = db.Column(db.String)
    typename = db.Column(db.String)
    name = db.Column(db.String)
    _tags = relationship('Tag', secondary=tagassoc, backref = 'objects', lazy='dynamic')
    tags = association_proxy('_tags', 'label')
    props = db.Column(SJSON)
    canonical = db.Column(db.BINARY)
    serialized = db.Column(db.BINARY)
    field_refs = association_proxy('_field_refs','ptr',creator=lambda k,v: Ref(field=k,ptr=v))
    
auto_delete_orphans(DbObject._tags)

class ObjConstruction(sqBase):
    __tablename__ = 'constructions'
    id = db.Column(db.Integer, primary_key=True)
    cmd = db.Column(db.String, unique=True)
    classname = db.Column(db.String)
    name = db.Column(db.String)
    ref = relationship('DbObject', backref = 'cmds', lazy='dynamic')

def addcolumn(cls, pn, name, join=None):
    if name=="":
        name=pn
    if not hasattr(cls,'__keyprops__'):
        cls.__keyprops__ = {}
    cls.__keyprops__[name] = (pn,join) #(col, pn)

def add_method(cls):
    def wrap(func):
        setattr(cls, func.__name__, func)
        return func
    return wrap

def convDbType(type):
    return type

def keyprop(name="", prop=True, join=None):
    def wrap(f):
        addcolumn(f.im_class, f.func_name, name, join)
        return property(f) if prop else f

def dbstorable(typename):
    def wrap(cls):
        cls.__table_name__ = typename
        if not hasattr(cls,"__dbsave__"):
            @add_method(cls)
            def __dbsave__(self): return pickle.dumps(self)
        if not hasattr(cls, "__dbload__"):
            @add_method(cls)
            def __dbload__(self): return pickle.loads(self)
        cls.__contents__ = property(cls.__dbsave__)
        if not hasattr(cls, "__canonical__"):
            cls.__canonical__ = cls.__contents__
        cls.tags = None
        
        cls.addTag = lambda self,tag: (set([tag]) if self.tags is None else self.tags.add(tag))
        return cls
    return wrap

def create_obj(q):
    r = pickle.loads(q.serialized)
    if r.__class__.__name__ != q.classname:
        r = r.unpack()
    return r

def update_dbobj(x,dobj=None):
    if dobj is None:
        dobj=DbObject()
    dobj.typename = x.__table_name__
    dobj.classname = x.__class__.__name__
    dobj.props = dict([(str(k),getattr(x,v)) for (k,v) in x.__keyprops__.items()])
    dobj.canonical = x.__canonical__
    if hasattr(x, 'name'):
        dobj["name"] = x.name
    dobj.tags = [] if x.tags is None else x.tags
    dobj.serialized = x.__contents__
    return dobj
  
class Database:
    def __init__(self, eng = None, **kwargs):
        if eng is None:
            try:
                with open('qcodedb.yaml') as f:
                    _dbconf = yaml.safe_load(f)
                    eng = _dbconf.get('dbplace', 'sqlite:///qcode.db')
            except:
                eng = 'sqlite:///qcode.db'
                print('database engine not specified: use `'+eng+"`")
        self.engine = db.create_engine(eng, **kwargs)
        self.session = sessionmaker(bind=self.engine)
        sqBase.metadata.create_all(self.engine)
        
    def get_tag(self,name):
        s = self.session()
        q = s.query(Tag).filter(Tag.name==name).first()
        if q is None:
            q = Tag(name=name)
            s.add(q)
            s.commit()
        return q
 
    def add(self, obj, update_if_exists=True):
        can = obj.__canonical__
        s = self.session()
        if hasattr(obj,'__command__'):
            if s.query(DbObject).filter(DbObject.cmd == obj.__command__).exists():
                return None # object is already in database
            cmd = obj.__command__
        else: cmd = None
    
        dc = {"cmd": cmd}
        if hasattr(obj, 'name'):
            dc["name"] = obj.name
    
        q = s.query(DbObject).filter(DbObject.canonical == can).one_or_none()
        if q is None:
            q = dc['ref'] = update_dbobj(obj)
            s.add(q)
        else:
            if update_if_exists:
                o1 = create_obj(q)
                if o1.update(obj):
                    s.merge(update_dbobj(o1,q))
            dc['ref'] = q
                
        if cmd is not None:
             s.add(ObjConstruction(**dc))
            
        s.commit()

dbcodes = Database(None,echo=True)
