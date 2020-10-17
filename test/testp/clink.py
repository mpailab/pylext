#import numpy
import struct
import socket
import copy
import io
import os
import time
import threading

tpError = -1
tpNone = 0
tpInt = 1
tpDouble = 2
tpString = 3
tpVector = 4
tpList = 5
tpClass = 6
tpMap = 7
tpFunction = 8
tpPType = 9
tpSet = 10
tpInst = 11
tpTuple = 12
tpCObj = 13
tpDelayed = 14

typemap = {
    type(None):tpNone,
    bool: tpInt,
    int : tpInt,
    type(2**63): tpInt,
    float:tpDouble,
    str : tpString,
    bytearray:tpString,
    list:tpList,
    tuple:tpTuple,
    'classobj':tpClass,
    dict:tpMap,
    set:tpSet,
    'function':tpFunction,
    Exception:tpError
    }

def pythonType(x):
    if type(x) in typemap:
        return type(x)
    elif hasattr(x,'__item__'):
        return list
    else: return type(x)

def conv2python(x):
    if type(x) in typemap:
        return x
    elif hasattr(x,'__item__'):
        return list(x)
    return x

class CObj(object):
    id = -1
    def __init__(self, link, id=-1):
        # print('create cobj, id = '+str(id))
        self.id = id
        self.link = link
        #self.link._regcobj(self)
        if id >= 0:
            self.link.incCObjRef(self)
    def __del__(self):
        if self.id >= 0 and self.link.opened:
            #print('delete cobj, id = '+str(self.id))
            # self.link._delcobj(self)
            self.link.decCObjRef(self)
        self.id = -1
    def __copy__(self):
        return self
        # return CObj(self.link, id)
    def __deepcopy__(self, memo):
        return self
        #return CObj(self.link, id)
    def __repr(self):
        return 'CObj(' + self.link.name + ', id = ' + str(self.id) + ')'

class CStruct:
    def __repr__(self):
        return ''.join([f + ' = '+ str(getattr(self,f))+'\n' for f in dir(self) if not callable(getattr(self,f)) and not f.startswith('__')])

class CConvStruct:
    def __init__(self, cls, dict):
        self.__dict__ = dict
        self._src_cls_ = cls

class Delayed(CObj):
    _value = None
    _canceled = False
    #def __init__(self, id=-1):
    #    super(CObj,self).__init__(id)
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
    def wait(self,timeout=None):
        if not self._canceled and not self.ready:
            self._value = self.link.waitCalculation(self,timeout=timeout)
        return self._value
    
    @property
    def canceled(self):
        return self._canceled
    @property
    def ready(self):
        return self.get() != None
    @property
    def runnung(self):
        return not self.canceled and not self.ready

    def update(self):
        if self._value is None and not self.canceled:
            self._state=self.link.delayedState(self)
        return self._state
    @property
    def state(self):
        return self.update().state
    @property
    def status(self):
        return self.update().status
    @property
    def progress(self):
        return self.update().progress

    def printStatus(self):
        st = self.update()
        print("Status   : " + str(st.status))
        if str(st.status) != 'finished' and st.progress > 0:
            print("Progress : " + str(int(st.progress * 10000) * 0.01) + "%")
    def __del__(self):
        if not self.ready:
            self.cancel()
        CObj.__del__(self)


def iunpack(fmt,data): return struct.unpack(fmt,data)[0]

def str2ba(s):
    if type(s) == bytearray: return s
    return bytearray(s,encoding = 'utf-8')
def ba2str(b):
    if type(b) == str: return b
    return b.decode('utf-8')

class S:
    def __init__(self):
        self.data = bytearray()
    def packsd(self,d):
        self << len(d)
        for (k,v) in d:
            (self << k).pack(v)
        return self
    def pack(self,d,prtp=True):
        t = typemap.get(pythonType(d),tpInst)
        if prtp:
            if d.__class__ == CObj:
                self.data += struct.pack('<i',tpCObj)
            elif d.__class__ == Delayed:
                self.data += struct.pack('<i',tpDelayed)
            else: self.data += struct.pack('<i',t)
        d = conv2python(d)
        if d == None: pass
        elif t == tpInt:
            self.data += struct.pack('<q',d)
        elif type(d) == bool:
            self.data += struct.pack('<q',int(d))
        elif type(d) == float:
            self.data += struct.pack('<d',d)
        elif type(d) == bytearray:
            self << len(d)
            self.data += d
        elif type(d) == str:
            ba = str2ba(d)
            self << len(ba)
            self.data += ba
        elif type(d) == dict:
            self << len(d)
            for (k,v) in d.items():
                self.pack(k).pack(v)
        #elif type(d)==function:
        #    raise Exception("Not implemented")
        elif type(d) == Exception:
            self << d.message
        elif type(d) == type:
            self << str(d)
        elif type(d) == type(S):
            self << d.__name__
            self.packsd([(f,getattr(d,f)) for f in dir(d) if not callable(getattr(d,f)) and not f.startswith('__')])
        elif issubclass(d.__class__, CObj):
            self << d.id
        elif d.__class__ is CConvStruct:
            self << d._src_cls_
            self.packsd([(f,getattr(d,f)) for f in dir(d) if not callable(getattr(d,f)) and not f.startswith('__') and f!='_src_cls_'])
        elif type(d) == tuple or type(d) == list or type(d) == set:
            self << len(d)
            for x in d:
                self.pack(x)
        else:
            #print('serialize class: '+d.__class__.__name__)
            self << d.__class__.__name__
            self.packsd([(f,getattr(d,f)) for f in dir(d) if not callable(getattr(d,f)) and not f.startswith('__')])
        return self
    def __lshift__(self,d):
        return self.pack(d,False)

class D:
    def __init__(self,d,clink):
        self.link = clink
        self.s = io.BytesIO(bytearray(d))
        self.pos = 0
    def read(self,n):
        self.pos+=n
        rr = self.s.read(n)
        #print('pos = '+str(self.pos)+', '+rr)
        return rr
    def unpacksd(self):
        l = self.unpack(tpInt)
        return [(self.unpack(tpString),self.unpack()) for i in range(l)]
    def unpack(self,prtp=None):
        if prtp == None:
            prtp = iunpack('<i',self.read(4))
        if prtp == tpNone:
            return None
        elif prtp == tpInt:
            return  iunpack('<q',self.read(8))
        elif prtp == tpDouble:
            return  iunpack('<d',self.read(8))
        elif prtp == tpString:
            l = iunpack('<q',self.read(8))
            return ba2str(self.read(l))
        elif prtp == tpList or prtp == tpVector:
            l = iunpack('<q',self.read(8))
            return [self.unpack() for i in range(l)]
        elif prtp == tpTuple:
            l = iunpack('<q',self.read(8))
            return tuple([self.unpack() for i in range(l)])
        elif prtp == tpSet:
            l = iunpack('<q',self.read(8))
            return set([self.unpack() for i in range(l)])
        elif prtp == tpMap:
            l = iunpack('<q',self.read(8))
            return dict([(self.unpack(),self.unpack()) for i in range(l)])
        elif prtp == tpFunction:
            id = iunpack('<q',self.read(8))
            def f(*args): return callCFunc(id,args)
            return f
        elif prtp == tpError:
            return Exception(self.unpack(tpString))
        elif prtp == tpCObj:
            return CObj(self.link,self.unpack(tpInt))
        elif prtp == tpDelayed:
            return Delayed(self.link, self.unpack(tpInt))
        elif prtp == tpInst:
            nm = self.unpack(tpString)
            if not nm in globals():
                raise Exception('No class with name `' + nm + '`')
            kl = globals()[nm]
            obj = kl()
            for (k,v) in self.unpacksd():
                obj.__dict__[k] = v
            return obj
        raise Exception('Cannot import object : unknown type ' + str(prtp))
#    def __lshift__(self,d):
#        return pack(self,d,False)

def makeCommand(cmd, *args):
    s = S()
    s << str(cmd) << args
    return s.data

def obj2msg(x):
    s = S()
    s.pack(x)
    return s.data

def readObj(link,s):
    #if link.debug:
    #    print("decode `"+s+"`")
    d = D(s,link)
    return d.unpack()

def formMsg(ba):
    return struct.pack('<i',len(ba)) + ba

def send(s,msg):
    s.send(formMsg(msg))
def recv0(s,d):
    rr = b''
    while len(rr) < d:
        x = s.recv(d - len(rr))
        if len(x)==0:
            raise Exception("cannot receive message from TCP server, connection lost")
        rr += x
    return rr

def recv(s):
    d = recv0(s,4)
    d = int(iunpack('<i',d))
    return recv0(s,d)
    #print('msg length = '+str(d))
    #rr = b''
    #while len(rr) < d:
    #    rr += s.recv(d - len(rr))
    #return rr

class CLink:
    HOST = '127.0.0.1' # The localhost
    PORT = 1234    # The same port as used by the server
    opened = False
    debug = False
    lock = None

    def printFuncInfo(self):
        for (nm,na,hasconf,dscr) in self.callfunc('getfuncs'):
            if hasconf:
                print(nm + '(' + str(na - 1) + ' args + conf) : ' + dscr)
            else:
                print(nm + '(' + str(na) + ' args) : ' + dscr)

    def register(self, obj, pr):
        def bind(f,*args):
            def g(*a,**kw): return f(*(args + a), **kw)
            return g
        for (nm,na,hasconf,dscr) in self.callfunc('getfuncs'):
            if hasconf:
                def f(info, s,*args,**kwargs):
                    (nm,na,hasconf) = info
                    if len(args) == na - 1:
                        return s.callfunc(nm,*(args + (kwargs,)))
                    elif len(args) == na:
                        if len(kwargs):
                            a = copy.copy(args)
                            a[-1].update(kwargs)
                            return s.callfunc(nm,*a)
                        return s.callfunc(nm,*args)
                    else: raise(Exception('wrong number of arguments : ' + str(len(args) + 1) + ' given, ' + str(na) + ' expected'))
                dscr1 = nm + '(' + str(na - 1) + ' args + conf) : ' + dscr
            else:
                def f(info, s,*args):
                    (nm,na,hasconf) = info
                    return s.callfunc(nm,*args)
                dscr1 = nm + '(' + str(na) + ' args) : ' + dscr
            if pr and dscr != '':
                print(dscr1)
            bf = bind(f,(nm, na, hasconf),self)
            setattr(bf, '__doc__', dscr1)
            setattr(obj,nm,bf)

    def connect(self, pr):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.opened = True
        self.s.connect((self.HOST, self.PORT))
        self.register(self,pr)

    def addconvertor(self, conv):
        self.convertors.append(conv)
        return self

    def startde(self,**kwargs):
        s = ' '.join(x+'='+str(y) for (x,y) in kwargs.items())
        if os.name == 'posix':
            b = os.system(self.depath + ' runtcp=1 port=' + str(self.PORT) + ' '+s+' &')
        else: b = os.system('start ' + self.depath + ' runtcp=1 port=' + str(self.PORT)+' '+s)
        if b:
            raise Exception("Cannot start "+self.name)

    def __init__(self, port=1234, path='D:/programmes/svn/tools/detool/detool.exe', dbg=False, printfuncs=False,**kwargs):
        self.debug = dbg
        self.name = os.path.basename(path)
        self.depath = path
        self.PORT = port
        self.convertors = []
        self._cobjs = set()
        self.lock = threading.RLock()

        try:
            print('Try connect to existing instance')
            self.connect(printfuncs)
        except:
            print('Start new instance of ' + self.name)
            self.startde(**kwargs)
            b = 50
            while b:
                try:
                    self.connect(printfuncs)
                    break
                except Exception as exc:
                    b -= 1
                    if not b: raise exc
                    time.sleep(0.1)
            print('Connected')
    def _regcobj(self,x):
        self._cobjs.add(x)
    def _delcobj(self,x):
        self._cobjs.remove(x)
    def callfunc(self, cmd, *args):
        for conv in self.convertors:
            args = [conv(x) for x in args]
        if self.debug:
            print('Send command to server : ' + cmd + str(tuple(args)))
        msg = makeCommand(cmd,*args)
        with self.lock:
            send(self.s,msg)
            obj = readObj(self, recv(self.s))
        if obj.__class__ == Exception:
            raise obj
        if self.debug:
            print('Result = ' + str(obj))
        return obj
    def close(self):
        if self.opened:
            self.s.close()
        self.opened=False
        print('Client closed.')
    def __del__(self):
        for c in self._cobjs:
            del c
        self.close()

#clink = CLink(1234)
#clink.debug=True
#def cfunc(name,*args):
#    return clink.callfunc(name,*args)

#for (nm,na,hasconf,dscr) in cfunc('getfuncs'):
#    if hasconf:
#        exec('def '+nm+"(*args,**kwargs): \n"
#             " if len(args)=="+str(na-1)+":\n"
#             " return clink.callfunc('"+nm+"',*(args+(kwargs,)))\n"
#             " elif len(args)=="+str(na)+":\n"
#             " if len(kwargs):\n"
#             " a = copy.copy(args)\n"
#             " a[-1].update(kwargs)\n"
#             " return clink.callfunc('"+nm+"',*a)\n"
#             " return clink.callfunc('"+nm+"',*args)\n"
#             " else: raise(Exception('wrong number of arguments :
#             '+str(len(args)+1)+' given, "+str(na)+" expected'))")

#        if dscr!='':
#            print(nm+'('+str(na-1)+' args + conf) : '+dscr)
#    else:
#        exec('def '+nm+"(*args): return clink.callfunc('"+nm+"',*args)")
#        if dscr!='':
#            print(nm+'('+str(na)+' args) : '+dscr)
if __name__ == "__main__":
    print("==================================")
    #print(getfuncs())
    def findcwCSS(mx,mz,z,dmax,**kwargs):
        rr = findQSpecQCCSSP(mx,mz,z,dmax,**kwargs)
        nn = 0
        s = rr.state
        print(s.status)
        while not rr.ready:
            ss = rr.state
            if ss.status != s.status:
                print(ss.status)
                s.status = ss.status
            nn+=1
        print("nn = " + str(nn))
        (d,tb,cwx,cwz) = rr.get() #findQSpecQCCSSP(mx,mz,z,dmax,**kwargs)
        if len(cwx) == 0 and len(cwz) == 0:
            print("No codewords found")
            print("Minimum distance >= " + str(d))
        else: print("Minimum distance = " + str(d))
        #for p in sorted(tb.items()):
        if len(cwx):
            print("X codewords examples:")
            for c in cwx:
                print(str(c))
        if len(cwz):
            print("Z codewords examples:")
            for c in cwz:
                print(str(c))
    #findcwCSS([[[0,43,31,102,15],
    #[0,64,67,94,112]]],[[[0,33,63,60,15],[0,25,84,96,112]]],127,10,numcw=3,onlyX=True)
    #rr = findQSpecQCCSSP([[[0,43,31,102,15],
    #[0,64,67,94,112]]],[[[0,33,63,60,15],[0,25,84,96,112]]],127,12,numcw=3,onlyX=True)
    code = {"classname":"ldpc", "matrix":([[[0,43,31,102,15], [0,64,67,94,112]]],[[[0,33,63,60,15],[0,25,84,96,112]]],127)}
    decoder = {"ldpcAccum":1,
        "classname":"nb4sse16",
        "nbdec":0,
        "enh":3,
        "augdelta":0.1,
        "enhall":1,
        "edelta":0.6,
        "osd": {"order":0, "sep":0, "syn":0, "osdy":0, "fast":1, "delta":1},
        "minPos":"all",
        "fastBP":2,
        "C2VmsgBitWidth":5, #9
        "V2CmsgBitWidth":7, #13
        "alpha":0.625,
        "fractionBits":2,
        "hardInputVal":10,   #30
        #nretry:5,
        "augInit":0,
        "nIters":32 }
    channel = {"classname":"depolarizing"}
    run = {
        "nThreads":1, # number of simulation threads
      "start":0.1,  # start noise
      "step":0.01,  # noise step
      "units":"P",    # noise units ('P' for depolarizing and independent, 'factor' for
                      # circuitBased and phenomenologcal models)
      "dumpW":0,
      "dumpInterval":1,
      "outputFormat":"table",
      "output": "res_test26_1.txt",
      "enoughNumErrors":100,
      "numWords":10000000000,
      "dumpStats":["BER", "WER", "FAR", "numWords", "numErrors", "numMiscorrections", "minDist"]
      }

    res = simulate(code=code,decoder=decoder,channel=channel,run=run)
    print(res)

    print("==================================")

#obj = {1:2,'abcd':3,(1,2):4} #[[1,'abcd'],[2,5,6],3,4]
#cmd="a"
#args=[10,3]
#print('Send to server : '+cmd+str(tuple(args))) #+str(obj))
#msg = makeCommand(cmd,*args) # obj2msg(obj)
#
#send(s,msg)
#
#data = recv(s)
#obj1=readObj(data)
#print 'Received back: '+str(obj1)
#
#s.close()
#print 'Client closed.'
