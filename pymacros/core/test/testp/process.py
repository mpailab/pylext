import threading
from functools import wraps

#def lockobj(f):
#    @wraps(f)
#    def g(self,*args,**kwargs):
#        with self.lock:
#            return f(self,*args,**kwargs)
#    return g

class PDelayed(object):
    class State:
        status='not started'
        progress=0
        state = None
        result = None

    _value = None
    _canceled = False
    _finished = False

    _stop = False
    _cancel = False

    _state = None
    _th = None
    _exn = None

    lock = None
    
    def __init__(self):
        self._state = PDelayed.State()
        self.lock = threading.RLock()

    def get(self,raise_exn=True):
        with self.lock:
            if self._exn is not None:
                if raise_exn:
                    raise self._exn
                else: return None
            return self._state.result
    value = property(get)
    
    def cancel(self):
        with self.lock:
            if not self._canceled and not self.ready:
                self._stop = True
                self._cancel = True
                self._canceled = True
        return self
    def stop(self):
        with self.lock:
            if not self._canceled and not self.ready:
                self._stop=True
                self._cancel=False
        return self
    def wait(self,timeout = None):
        if not self._canceled and not self.ready:
            self._th.join(timeout)
            # self._value = self.result
        return self.value
    def update(self):
        return self._state
    @property
    def canceled(self):
        return self._canceled
    @property
    def ready(self):
        with self.lock:
            return self._state.result is not None or self._exn is not None
    @property
    def runnung(self):
        return not self.canceled and not self.ready
    
    @property
    def state(self):
        with self.lock:
            return self._state.state
    @property
    def status(self):
        with self.lock:
            return self._state.status
    @property
    def prorgess(self):
        return self._state.progress

    def printStatus(self):
        with self.lock:
            st = self._state
        print("Status   : " + str(st.status))
        if str(st.status) != 'finished' and st.progress > 0:
            print("Progress : " + str(int(st.progress * 10000) * 0.01) + "%")
    def __del__(self):
        if self._th.isAlive():
            self.cancel()
            if self._th is not threading.current_thread():
                self._th.join()

class SetState:
    def __init__(self,**kwargs):
        self.__dict__.update(kwargs)
    pass
def progress(pr):
    return SetState(progress=pr)
def pstate(st):
    return SetState(state=st,progress=0)
def preturn(r):
    return SetState(result=r)
def pstatus(r):
    return SetState(status=r)

def process(f):
    @wraps(f)
    def g(*args,**kwargs):
        d = PDelayed()
        def pp():
            #try:
                for a in f(*args,**kwargs):
                    #print(d.lock.locked())
                    with d.lock:
                        for field in ['progress','state', 'result','status']:
                            if hasattr(a,field):
                                setattr(d._state,field,getattr(a,field))
                        if hasattr(a,'result'):
                            break
                        if d._stop: 
                            if not d._cancel:
                                if not hasattr(d._state,'result') and hasattr(d._state,'state'):
                                    d._state.result = d._state.state
                                else: d._state.result='Stopped'
                            break
            #except Exception as exn:
                #d._exn = exn
        d._th = threading.Thread(target=pp, args=())
        d._th.start()
        return d
    return g
