import clink
import time
from math import *
import cmodules
from process import *

class UnknownUnitsException(Exception):
    def __init__(self,units):
        Exception.__init__(self,'Unknown noise units `'+str(units)+'`')

class NoiseModel(object):
    @property
    def defaultUnits(self): # universal unit is error probability
        return 'P'
    def setNoise(self,units,val):
        raise "Not implemented"
    def get_conf(self):
        return self
    def get_value(self):
        raise Exception(str(self.__class__)+".get_value() not implemented")
    #value=property(get_value)

class Channel(NoiseModel):
    pass

class SimpleChannel(Channel):
    def __init__(self):
        self.px=0
        self.py=0
        self.pz=0
        self._ref=None
        self._name='simple'
    def setXYZNoise(self,px,py,pz):
        self.px=px
        self.py=py
        self.pz=pz

class IThermalBath(NoiseModel):
    def __init__(self):
        self.T=0
        self.usebeta = False
    def defaultUnits(self): 
        return 'beta' if self.usebeta else 'T'
    def setT(self,T):
        self.T,self.beta=(T,1./T)
        self.usebeta=False
    def setBeta(self,beta):
        self.beta,self.T=(beta,1./beta)
        self.usebeta=True
    @property
    def noise(self):
        return self.beta if self.usebeta else self.T
    @noise.setter
    def noise(self,val):
        self.T,self.beta = (1./T,T) if self.usebeta else (T,1./T)
    def setNoise(self,units,val):
        if units=='T':
            self.setT(units)
        elif units=='beta':
            self.setBeta(units)
        else: raise Exception('Unknown noise units `'+str(units)+'`')
    @property
    def value(self):
        return self.noise

class SimpleThermalBath(IThermalBath):
    classname='simpletb'
    def __init__(self, beta, usebeta = True):
        IThermalBath.__init__(self)
        if usebeta:self.setBeta(beta)
        else: self.setT(beta)

class ThermalBath(IThermalBath):
    classname='thermalbathmc'
    def __init__(self, beta, usebeta = True):
        IThermalBath.__init__(self)
        if usebeta:self.setBeta(beta)
        else: self.setT(beta)

class Depolarizing(SimpleChannel):
    classname='depolarizing'
    def __init__(self, p=0.):
        self.noise = p
    @property
    def noise(self):
        return self._p
    @noise.setter
    def noise(self,val):
        self._p = val
        self.px = self.py = self.pz = val/3.
    def setNoise(units,val):
        if units.lower() == 'p':
            self.noise = val
        else: raise Exception('Unknown noise units `'+str(units)+'`')
    @property
    def value(self):
        return self.noise


class IndependentXZ(SimpleChannel):
    classname = 'xz'
    def __init__(self,p=0.):
        self.noise = p

    @property
    def noise(self):
        return self.px

    @noise.setter
    def noise(self,val):
        self.px = self.pz = val
        self.py = val*val

    def setNoise(self,units,val):
        if units.lower() == 'p':
            self.noise = val
            #self.px = self.pz = val
            #self.py = val*val
        else: raise Exception('Unknown noise units `'+str(units)+'`')
    @property
    def value(self):
        return self.noise

class CircuitNoise(NoiseModel):
    def setNoise(units,p):
        if units.lower()=='p' or units.lower()=='factor':
            self.p = p
        else: raise Exception('Unknown noise units `'+str(units)+'`')

class PhenomModel(CircuitNoise):
    classname='phenomNoise'

class CircuitBasedModel(CircuitNoise):
    classname='circuitBased'

class Simulator(object):
    def __init__(self, new_instance = False,**kwargs):
        self._ref = cmodules.loadCModule("QECC simulator", new_instance, **kwargs)
        #self._params = kwargs
        self._code    = None
        self._decoder = None
        self.seed = 0
        self.run = { "nThreads":1}
        self._sim = None #self._ref.createSim()
    @property
    def num_threads(self):return self.run["nThreads"]
    @num_threads.setter
    def num_threads(self,n): 
        if self.run["nThreads"]!=n:
            self.run["nThreads"]=n
            self._sim = None

    @property
    def code(self):return self._code
    @code.setter
    def code(self,c): 
        self._code=c
        #print('set new code')
        self._sim = None
    
    @property
    def decoder(self):return self._decoder
    @decoder.setter
    def decoder(self,d): 
        self._decoder=d
        self._sim = None

    @property
    def csim(self):
        if self._sim is None: self._sim = self._ref.createSim(code=self.code, decoder=self.decoder,run={"nThreads":self.num_threads})
        return self._sim

    def simulate(self, noise_model, seed = 0):
        if self.code is None: raise Exception("Simulator error: code not set")
        if self.decoder is None: raise Exception("Simulator error: decoder not set")
        if isinstance(noise_model, SimpleChannel):
            return self._ref.simulatePTSimple(self.csim, self._ref.createChannel(noise_model.get_conf()),seed)
        elif isinstance(noise_model, CircuitNoise): 
            return self._ref.simulateC(self.csim, self._ref.createMsModel(noise_model.get_conf()),seed)
        elif isinstance(noise_model, IThermalBath):
            return self._ref.simulatePTSelfCorrection(self.csim, self._ref.createThermalBath(noise_model.get_conf()), seed)
        raise Exception("Unknown noise model class `"+noise_model.__class__+"`")
    def __del__(self):
        # self._ref.close()
        pass

def stop_by_limits(max_nw, max_ne):
    return (lambda st: max(float(st.nW)/float(max_nw), float(st.NE)/float(max_ne)))

def stop_sim_by_wer(wer):
    return (lambda st: st.WER <= wer)

def linscale(start,delta):
    while True:
        yield start
        start+=delta

def lin2zero(start,delta):
    while start>0:
        yield start
        start-=delta

def fmap(f, gen):
    for x in gen: yield f(x)

def logscale(start,factor=10,points=[1,2,3,4,5,6,8]):
    ss = factor**floor(log(start,factor))
    for npt in range(len(points)):
        if points[npt]>=start/ss:
            break
    while True:
        yield ss*points[npt]
        if npt == 0:
            npt=len(points)-1
            ss/=factor
        else: npt-=1

# sim is Simulator class that has method simulate (on some noise)
@process
def simulate_plot(sim, noise, pt_stop_cond, sim_stop_cond, callback = None):
    res = []
    for n in noise:
        ns=n.value
        yield pstatus("Simulate point "+str(ns))
        rr = sim.simulate(n)
        res.append((ns,None))
        while not rr.ready and not rr.canceled:
            time.sleep(1)
            st = rr.state
            if st is None or st==[]: 
                continue
            #st = st
            pgs = pt_stop_cond(st)
            #print(progress)
            if pgs>=1:
                rr.stop()
                break
            if callback is not None:
                callback(res)
            res[-1] = (ns,st)
            yield pstate(res)
            yield progress(pgs)

        st = rr.state
        res[-1]=(ns,st)
        yield pstate(res)
        yield progress(1.)

        if sim_stop_cond(st):
            break

    yield preturn(res)
