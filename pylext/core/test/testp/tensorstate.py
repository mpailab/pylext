import numpy as np
import random
import math
from copy import copy
from collections import defaultdict

cmplxtype=np.complex128

class enumerator:
    def __init__(self):
        self._d = {}
        self.inv = []
    def num(self,x):
        if x in self._d:
            return self._d[x]
        else:
            res = len(self.inv)
            self._d[x] = res
            self.inv.append(x)
            return res
        #return self._d.setdefault(x,len(self._d))
    def __getitem__(self,x):
        return self.num(x)
        #return self._d.setdefault(x,len(self._d))
    def __setitem__(self,x,val):
        assert val<len(self.inv)
        assert x in self._d
        self.inv[self._d[x]] = None
        self._d[x] = val
        if self.inv[val] is None:
            self.inv[val] = x
    def __add__(self,x):
        res = enumerator()
        res._d = copy(self._d)
        res.inv = copy(self.inv)
        for a in x:
            res.num(a)
        return res
    def __iter__(self):
        return self._d.__iter__()

class TNElem():
    maxdim = 0
    def __init__(self,ten,indices):
        self._indices = list(indices)
        self._m = {self._indices[i]:i for i in range(len(self._indices))}
        self._ten = np.array(ten, dtype=cmplxtype) if type(ten) is list else ten
        assert np.prod(self._ten.shape)==2**len(self._indices)
        self._ten = self._ten.reshape((2,)*len(self._indices))
        TNElem.maxdim = max(TNElem.maxdim, self.dim)

    @property
    def indices(self):
        return self._indices
    @property
    def dim(self):
        return len(self._indices)
    @property
    def value(self):
        assert self.dim==0
        return float(np.abs(self._ten))
    def rename(self,mm):
        if callable(mm):
            m = {x:mm(x) for x in self._indices}
        else:
            m = {x:x for x in self._indices}
            m.update(mm)
        self._m = {m[k]:v for (k,v) in self._m.items()}
        self._indices = [m[i] for i in self._indices]
        return self
    def __copy__(self):
        return TNElem(self._ten,self._indices)
    def copy(self):
        return TNElem(self._ten,self._indices)
    def conj(self):
        #assert self.dim % 2 == 0
        t = np.conj(self._ten)
        #t.transpose(list(range(self.dim//2,self.dim))+list(range(self.dim//2)))
        return TNElem(t,self._indices).rename(lambda n:n+('conj',))
    def __mul__(self,x):
        return contraction(self,x)
    def __and__(self,x):
        return elem_mult(self,x)
    def __pow__(self,n):
        return TNElem(self._ten**n,self._indices)


def contraction(t1,*tn):
    """Contract several tensors by common indices"""
    for t in tn:
        indices=set(t1._indices).intersection(set(t.indices))
        num1 = [t1._m[i] for i in indices]
        num2 = [t._m[i] for i in indices]
        res = np.tensordot(t1._ten,t._ten, axes=(num1,num2))
        res_ind = [i for i in t1._indices if i not in indices]+[i for i in t._indices if i not in indices]
        t1 = TNElem(res,res_ind)
    return t1

def elem_mult(t1,t):
    """Multiply several tensors by common indices without contraction"""
    en = enumerator()
    en += t1._indices
    en += t._indices
    indices = set(t1._indices)|set(t.indices)
    num1 = [en[i] for i in t1.indices]
    num2 = [en[i] for i in t.indices]
    num = [en[i] for i in indices]
    res = TNElem(np.einsum(t1._ten,num1,t._ten, num2,num),indices)
    return res

def gate(qi,mat):
    return TNElem(np.array(mat,dtype=cmplxtype),[q+('in',) for q in qi]+[q+('out',) for q in qi])

hmatrix = np.array([[1,1],[1,-1]])/2**0.5
def hgate(q):
    return gate(q,hmatrix)

def optimize_contractions(tns,steps=1000,T0=1):
    tns = [set(t) for t in tns]
    vs = sum(tns, []) # Все переменные тензоров
    vst = {v:[t for t in tns if v in t] for v in vs}
    def cost(ord):
        m = 0; t = 0
        
        return m,t

class TensorNetwork:
    def __init__(self):
        self.indices = []
        self._en = enumerator()
        self.tensors = []
    def add(self, t, ind):
        ii = [self._en[i] for i in ind]
        ten = np.array(t, dtype=cmplxtype)
        dim = int(math.log2(np.prod(ten.shape)))
        ten = ten.reshape((2,)*dim)
        assert dim == len(ind)
        self.tensors.append((np.array(ten, dtype=cmplxtype),ii))
    def contract(self,res_ind=[],opt='greedy'):
        ri = [self._en[i] for i in res_ind]
        args = [i for t in self.tensors for i in t]
        path = np.einsum_path(*args,ri,optimize=opt)
        #print(path[1])
        return np.einsum(*args, ri, optimize=path[0])
    def copy(self):
        res = TensorNetwork()
        res.tensors = self.tensors
        res._en = copy(self._en)
        res.indices = copy(self.indices)
    def conj(self):
        res = TensorNetwork()
        for (t,i) in self.tensors:
            res.add(np.conj(t),[(self._en.inv[ii],'conj') for ii in i])
        return res
    def addtn(self,tn):
        for (t,i) in tn.tensors:
            self.add(t,[tn._en.inv[ii] for ii in i])
    def identify(self,i,j): # rename j -> i
        i1 = self._en[i]
        j1 = self._en[j]
        self._en[j] = i1
        for k in range(len(self.tensors)):
            self.tensors[k] = (self.tensors[k][0], [x if x!=j1 else i1 for x in self.tensors[k][1]])

class CircState:
    def __init__(self):
        #print('============= START CIRCUIT =============')
        self._en = enumerator()
        self._d = defaultdict(lambda:0)
        self._tn = TensorNetwork()
        self._gates = []
        self._nq = 0
    def qnum(self,i) -> int:
        r = self._en[i]
        if r>=self._nq:
            self._nq=r+1
            self._tn.add([1,0],[(r,0)])
        return r
    def gate(self,mat,*qnum):
        #print('gate ',qnum)
        if np.prod(np.array(mat).shape)==2**len(qnum):
            return self.diag(mat,*qnum)
        q = [self.qnum(i) for i in qnum]
        qi = [(qj, self._d[qj]) for qj in q]
        for qj in q:
            self._d[qj]+=1
        qi += [(qj, self._d[qj]) for qj in q]
        self._tn.add(mat,qi)
        return self
    def diag(self,vec,*qnum):
        #print('diag ',qnum)
        q = [self.qnum(i) for i in qnum]
        qi = [(qj, self._d[qj]) for qj in q]
        self._tn.add(vec,qi)
        return self
    def expectation(self, mat,*qnum,opt='greedy'):
        tn = self._tn.conj()
        tn.addtn(self._tn)
        vec = np.prod(np.array(mat).shape)==2**len(qnum)
        if not vec: assert np.prod(np.array(mat).shape)==4**len(qnum)

        q = [self.qnum(x) for x in (self._en if vec else (set(self._en) - set(qnum)))]
        qn = [self.qnum(x) for x in qnum]
        for qi in q:
            tn.identify((qi,self._d[qi]),((qi,self._d[qi]),'conj'))
        if vec: tn.add(mat, [(qi,self._d[qi]) for qi in qn])
        else:   tn.add(mat, [(qi,self._d[qi]) for qi in qn] + [((qi,self._d[qi]),'conj') for qi in qn])
        r = float(np.real(tn.contract(opt=opt)))
        #print('Measured : ',r)
        #print('============ MEASURE CIRCUIT ============')
        return r

gH = np.array([[1,1],[1,-1]])/2**0.5
gCNOT = np.array([[1,0,0,0],[0,1,0,0],[0,0,0,1],[0,0,1,0]])
if __name__ == '__main__':
    st = CircState()
    st.gate(gH,0)
    st.gate(gCNOT,0,1)
    print(st.expectation([1,0],0))
