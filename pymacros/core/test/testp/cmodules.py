from clink import CLink,CObj,typemap,CConvStruct
import yaml
from copy import copy

try:
    import sage.all as sg
    import sage.rings.integer
    import sage.rings.real_mpfr
    conv2pythonMap={
        sg.Integer: int,
        sage.rings.integer.Integer: int,
        sage.rings.real_mpfr.RealLiteral: float,
        sage.rings.real_mpfr.RealNumber: float
        }

    def conv2python(x):
        if conv2pythonMap.has_key(type(x)):
            return conv2pythonMap[type(x)](x)
        if hasattr(x, '__iter__'):
            items = x.items() if hasattr(x,'items') else x
            y = [conv2python(z) for z in items]
            #if type(x)==dict:
            #    print(y)
            if type(x)==tuple or type(x)==set or type(x)==dict:
                return type(x)(y)
            return y
        if type(x) in typemap or isinstance(x,CObj):
            return x
        #print('CConvStruct from class: '+x.__class__.__name__)
        return CConvStruct(x.__class__.__name__, dict([(f,conv2python(getattr(x,f))) for f in dir(x) if not callable(getattr(x,f)) and not f.startswith('__')]))

    sagemode=True
    print("CLink sage mode")
except ImportError:
    def conv2python(x): return x
    sagemode=False

_cmodule_confs = {}

def _get_confs():
    global _cmodule_confs
    if len(_cmodule_confs):
        return _cmodule_confs
    try:
        with open('cmodules.yaml') as f:
            #_cmodule_confs = dict([tuple([x.strip() for x in line.split(': ')]) for line in f])
            _cmodule_confs = yaml.safe_load(f)
    except Exception as exc:
        _cmodule_confs = {}
        print('cannot read C modules location file `cmodules.yaml`:')
        print(exc)
    return _cmodule_confs

def update_confs():
    global _cmodule_confs
    _cmodule_confs={}
    _get_confs()

_cmodule_port_name = 5000

_loaded_modules = {}

class CModule:
    def __init__(self,name,new_instance,**kwargs):
        cnf = _get_confs()
        if not name in cnf:
            raise Exception('Module `'+name+"` not found")
        if not new_instance and cnf[name] in _loaded_modules and len(_loaded_modules[cnf[name]])>0:
            for m in copy(_loaded_modules[cnf[name]]):
                try:
                    m.getfuncs()
                except:
                    _loaded_modules[cnf[name]].remove(m)
                    continue
                self._link = m
                self._link.register(self, False)
                return
        global _cmodule_port_name
        _cmodule_port_name += 1
        print('path = '+cnf[name])
        res = CLink(_cmodule_port_name, cnf[name],**kwargs)
        if not cnf[name] in _loaded_modules: _loaded_modules[cnf[name]] = []
        _loaded_modules[cnf[name]].append(res)
        self._link = res
        res.register(self, False)
        #return res
    def close(self):
        self._link.close()

def loadCModule(name, new_instance=False,**kwargs):
    res = CModule(name,new_instance,**kwargs)
    if sagemode:
        res._link.addconvertor(conv2python)
    return res
