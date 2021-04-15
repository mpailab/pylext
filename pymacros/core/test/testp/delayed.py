import threading

class PDelayed(Delayed):
    class State:
        status='not started'
        progress=0
        result=None

    _value = None
    _canceled = False
    _finished = False

    _stop = False
    _cancel = False

    _state = None
    _th = None

    def __init__(self):
        self._state = State()

    def get(self):
        if not self._canceled and self._value == None:
            self._value = self.link.delayedValue(self)
        return self._value
    value = property(get)
    
    def cancel(self):
        if not self._canceled and not self.ready:
            self.link.cancelCalculation(self)
            self._canceled = True
    def stop(self):
        if not self._canceled and not self.ready:
            self.link.stopCalculation(self)
    def wait(self,timeout = None):
        if not self._canceled and not self.ready:
            self._th.join(timeout)
            self._value = self.result
        return self._value
    
    @property
    def canceled():
        return self._canceled
    @property
    def ready(self):
        return self.get() != None
    @property
    def runnung():
        return not self.canceled and not self.ready

    @property
    def state(self):
        return _state

    def printStatus(self):
        st = self.state
        print("Status   : " + str(st.status))
        if str(st.status) != 'finished' and st.progress > 0:
            print("Progress : " + str(int(st.progress * 10000) * 0.01) + "%")
    def __del__(self):
        if self._th.isAlive():
            self.cancel()
            self._th.join()

def dprocess(f):
    @wraps(f)
    def g(*args,**kwargs):
        d = PDelayed()
        def pp():
            for a in f(*args,**kwargs):
                for field in ['progress','state', 'result']:
                    if hasattr(a,field):
                        setattr(d._state,field,getattr(a,field))
                if d._stop: break
        d._th = threading.Thread(target=pp, args=())
        d._th.start()
        return d
