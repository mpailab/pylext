import sqlalchemy as db
from sqlalchemy.orm import sessionmaker, relationship, backref,scoped_session
from sqlalchemy.ext.declarative import declarative_base
import threading
import time

from functools import wraps
import pickle

sqBase = declarative_base()

class SBINARY(db.TypeDecorator):
    impl=db.BINARY
    def process_bind_param(self, value, dialect):
        if type(value)==str:
            return value
        return pickle.dumps(value)
    def process_result_value(self,value, dialect):
        return pickle.loads(value)
    
class Item(sqBase):
    __tablename__ = 'items'
    #id = db.Column(db.Integer, primary_key=True)
    key = db.Column(db.BINARY, primary_key=True)
    value = db.Column(db.BINARY)

def conv_to_key(k):
    if type(k)==str:
        return k
    if type(k)==dict:
        k = [(x,conv_to_key(y)) for (x,y) in k.items()]
        k.sort()
        return tuple(k)
    if type(k)==set:
        k=list(k)
        k.sort()
        return tuple(k)
    if hasattr(k,'__iter__'):
        return tuple([conv_to_key(x) for x in k])
    return k

class Cache:
    def __init__(self,dbname, **kwargs):
        self.engine = db.create_engine('sqlite:///'+dbname+'.dbcache', echo=False,**kwargs)
        self.session = sessionmaker(bind=self.engine)()
        self.last_commit_time = time.time()
        sqBase.metadata.create_all(self.engine)
        self.lock = threading.RLock()
        self._stop = False
        self._commited = True
        def comm_cycle(s):
            i=0
            while not s._stop:
                time.sleep(0.1)
                i+=1 
                if i>=100 and not s._commited:
                    s.commit()
                    i=0
                    
        self._commit_thread = threading.Thread(target=comm_cycle, args=(self,))
        #self._commit_thread.start()
    def __del__(self):
        #self._stop = True
        #self._commit_thread.join()
        self.commit()
    def commit(self, dt = 0):
        t = time.time()
        if t-self.last_commit_time >= dt:
            self.last_commit_time = t
            self.session.commit()
            self._commited = True
        else: self._commited = False
    def clear(self):
        #s = self.session()
        #with self.lock:
        self.session.query(Item).all().delete()
        self.commit()
    def __getitem__(self,arg):
        #with self.lock:
            #s = self.session()
            #key = pickle.dumps(arg)
        if not self._commited:
            self.commit(10)
        q = self.session.query(Item).filter(Item.key==pickle.dumps(arg)).one_or_none()
        if q is None: return None
        return pickle.loads(q.value)
    def __setitem__(self,k,v):
        #with self.lock:
        self.session.merge(Item(key=pickle.dumps(k),value=pickle.dumps(v)))
        self.commit(10)
            #self.session.commit()
    def update(self, f, *args, **kwargs):
        ign = kwargs.pop('_ignoring')
        update = kwargs.pop('update',False)
        kwa1 = kwargs if ign is None else dict((k,v) for (k,v) in kwargs.items() if k not in ign)
        key = conv_to_key(args if kwa1=={} else args+(kwa1,)) #tuple(args)+(kwargs,)
        #print('search key',key)
        v = None if update else self[key] 
        if v is None:
            v = f(*args, **kwargs)
            self[key] = v
        return v
    def printkeys(self):
        with self.lock:
            for q in self.session.query(Item).all():
                print(str(pickle.loads(q.key))+' --> '+str(pickle.loads(q.value)))
class Caches:
    def __init__(self):
        self.c = {}
    def __getitem__(self,nm):
        if nm not in self.c:
            self.c[nm]=Cache(nm)
        return self.c[nm]

_caches=Caches()
def dbcached(nm, ignoring=None):
    if type(ignoring) is str:
        ignoring = {ignoring}
    def wrap(f):
        @wraps(f)
        def g(*args,**kwargs):
            cache = kwargs.pop('use_cache',True)
            update = kwargs.pop('update',False)
            if not cache:
                return f(*args,**kwargs)
            return _caches[nm].update(f,*args,update=update,_ignoring=ignoring,**kwargs)
        return g
    return wrap

def update_cache(nm, d):
    c = _caches[nm]
    for (k,v) in d.items():
        c[k]=v
    c.commit()
