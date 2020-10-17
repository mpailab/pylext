from tensorstate import TNElem, contraction
import numpy as np
from math import sin,cos
#from qaoa import gentree, adjust_maxcut_angles
from time import time
#import dlib
import scipy.optimize
import random

def diag(l):
    return np.array([[l[i] if i==j else 0 for i in range(len(l))] for j in range(len(l))])

def Hh(q,i):
    return TNElem(np.array([[1,1],[1,-1]])/2**0.5,[(q,i),(q,i+1)])

def zero(q):
    return TNElem([1,0],[(q,0)])

def Uc(q1,i1,q2,i2,gamma):
    tb=[0,1,1,0]
    return TNElem(diag([cos(gamma*x)-1.0j*sin(gamma*x) for x in tb]), [(q1,i1),(q2,i2),(q1,i1+1),(q2,i2+1)])

def Uc0(q1,i1,q2,i2,gamma):
    tb=[0,1,1,0]
    return TNElem([cos(gamma*x)-1.0j*sin(gamma*x) for x in tb], [(q1,i1),(q2,i2)])

def Me(q1,i1,q2,i2):
    return TNElem(diag([0,1,1,0]),[(q1,i1), (q2,i2), (q1,i1,'conj'), (q2,i2,'conj')])

def Me0(q1,i1,q2,i2):
    return TNElem([0,1,1,0],[(q1,i1), (q2,i2)])

def Ub(q,i,beta):
    return TNElem([[cos(beta),-1.0j*sin(beta)],[-1.0j*sin(beta),cos(beta)]], [(q,i),(q,i+1)])

def debugpr(*args,**kwargs):
    #print(*args,**kwargs)
    pass

def calc_maxcut_tree_expectation(d,beta,gamma):
    p=len(beta)
    assert p==len(gamma)
    h0 = Hh(0,0)*zero(0)
    uc01 = Uc(0,1,1,0,gamma[0])
    g = h0*uc01
    g *= g.conj().rename({(0,2,'conj'):(0,2)})
    debugpr('i = 0: dim = ',g.dim,';\tindices = ',g.indices)
    for i in range(1,p+1):
        h = Hh(i,0)*zero(i)
        gr = h*h.conj()
        debugpr('processing level',i)
        for j in range(1,d):
            gr *= g.rename(lambda x: (x[0],x[1]+1)+x[2:])
            debugpr('\tjoin subtree ',j,': dim = ',gr.dim,';\tindices = ',gr.indices)
        #print('i =',i,': gr.dim = ',gr.dim,';\tindices = ',gr.indices)
        g = gr.copy()
        if i==p:break
        uc = [Uc(i,d+j*(d+1),i+1,(d if i==p else 0)+j*(d+1),gamma[j])*Ub(i,(j+1)*(d+1),beta[j]) for j in range(i)]
        for j in range(i-1):
            g *= uc[j]; g*= uc[j].conj()
            debugpr('\tadd Uc[',j,']: dim = ',g.dim,';\tindices = ',g.indices)
        jn = i*(d+1)+2
        uci = Uc(i,jn-1,i+1,i*(d+1),gamma[i])*Ub(i,jn,beta[i])
        debugpr('\tuc[i-1] indices = ',uc[i-1].indices)
        debugpr('\tuc[i] indices = ',uci.indices)
        u1 = (uc[i-1]*uci*uci.conj().rename({(i,jn+1,'conj'): (i,jn+1)})*uc[i-1].conj())
        g *= u1
        debugpr('\tadd Uc last link: dim = ',u1.dim,';\tindices = ',u1.indices)
        debugpr('\tadd Uc last: dim = ',g.dim,';\tindices = ',g.indices)

    for j in range(p-1):
        ucj = Uc(i,d+j*(d+1),i+1,d+j*(d+1),gamma[j])*Ub(i,(j+1)*(d+1),beta[j])*Ub(i+1,(j+1)*(d+1),beta[j])
        g *= ucj; g*= ucj.conj()
        debugpr('\tadd Uc[',j,']: dim = ',g.dim,';\tindices = ',g.indices)

    gr.rename(lambda x: (p+1,)+x[1:])
    pos = (d+1)*p
    u = Uc(p,pos-1,p+1,pos-1,gamma[p-1])
    gm = Me(p,pos+1,p+1,pos+1)*Ub(p,pos,beta[-1])*Ub(p+1,pos,beta[-1])*Ub(p,pos,beta[-1]).conj()*Ub(p+1,pos,beta[-1]).conj()
    debugpr('gm0.dim =',gm.dim,';\tgm0.indices =',gm.indices)
    debugpr('uc[p-1].dim =',u.dim,';\tuc[p-1].indices =',u.indices)
    gm *= u*u.conj()
    debugpr('gm.dim =',gm.dim,';\tgm.indices =',gm.indices)
    g *= gm
    debugpr('g*gm dim = ',g.dim,';\tindices = ',g.indices)
    g *= gr
    debugpr('final: dim = ',g.dim,';\tindices = ',g.indices)
    return g.value

def calc_maxcut_tree_expectation_1(d,beta,gamma):
    p=len(beta)
    assert p==len(gamma)
    h0 = Hh(0,0)*zero(0)
    uc01 = Uc0(0,1,1,1,gamma[0])
    g = h0 & uc01
    g *= g.conj().rename({(0,1,'conj'):(0,1)})
    debugpr('\n======= d = %d, p = %d ========'%(d,p))
    debugpr('i = 0: dim = ',g.dim,';\tindices = ',g.indices)
    for i in range(1,p+1):
        g **= (d-1)
        h = Hh(i,0)*zero(i)
        gr = h*h.conj()
        g &= h
        g &= h.conj()
        for j in range(i-1):
            g &= Ub(i,j+1,beta[j])
            g &= Ub(i,j+1,beta[j]).conj()
        if i<p:
            g &= Ub(i,i,beta[i-1])&Ub(i,i,beta[i-1]).conj().rename({(i,i+1,'conj'):(i,i+1)})
        gr = g.copy()
        for j in range(i+1 if i<p else p-1):
            if j<i:
                g *= Uc0(i,j+1,i+1,j+1,gamma[j])
                g *= Uc0(i,j+1,i+1,j+1,gamma[j]).conj()
            else:
                g &= Uc0(i,j+1,i+1,j+1,gamma[j])
                g *= Uc0(i,j+1,i+1,j+1,gamma[j]).conj().rename({(i,i+1,'conj'):(i,i+1)})
            debugpr('\tadd Uc[',j,']: dim = ',g.dim,';\tindices = ',g.indices)

    gr.rename(lambda x: (x[0]+1,)+x[1:])
    pos = p+1
    uc = Uc0(p,p,p+1,p,gamma[-1])
    gm = Me0(p,pos,p+1,pos) & Ub(p,pos-1,beta[-1]) & Ub(p+1,pos-1,beta[-1])
    gm *= (Ub(p,pos-1,beta[-1]).conj()*Ub(p+1,pos-1,beta[-1]).conj()).rename(lambda x: (x[0],x[1]) if x[1]==pos else x)
    gm &= uc; gm &= uc.conj()

    debugpr('g dim = ',g.dim,';\tindices = ',g.indices)
    debugpr('gm.dim =',gm.dim,';\tgm.indices =',gm.indices)
    g *= gm
    debugpr('g*gm dim = ',g.dim,';\tindices = ',g.indices)
    debugpr('gr dim = ',gr.dim,';\tindices = ',gr.indices)
    g *= gr
    debugpr('final: dim = ',g.dim,';\tindices = ',g.indices)
    return g.value


bg = [
    ([],[]),
    ([1.1781],[2.5261]),
    ([0.6541,1.3665],[2.4488,0.1450]),
    ([0.9619, 2.6820, 1.8064],[2.7197, 5.4848, 2.2046]),
    ([5.6836, 1.1365, 5.9864, 4.8714],[0.4088, 0.7806, 0.9880, 4.2985]),
    ([1.6617, 0.5446, 1.0590, 4.0065, 6.0988],[3.8370, 2.0107, 5.2067, 0.2183, 1.7976])
]

class timer:
    def __init__(self,name='',count=1):
        self._name = name
        self.count = count
    def __enter__(self):
        self._t = time()
    def __exit__(self, type, value, traceback):
        print(self._name+' time:', (time()-self._t)/self.count)
        return False

def find_tree_angles(p,d,max_calls=1000,method='COBYLA',x0=None):
    print('\nOptimize p =',p,',  d =',d)
    iters=0
    def f(*args):
        nonlocal iters
        iters += 1
        return calc_maxcut_tree_expectation_1(d,args[:p],args[p:])
    t=time()
    if method=='dlib':
        res = dlib.find_max_global(f,[0]*(2*p),[6.3]*(2*p),max_calls)
    else:
        best = 0
        while iters<max_calls:
            rr = scipy.optimize.minimize(lambda x:-f(*x), [random.uniform(0,6.283) for i in range(2*p)] if iters>0 or x0 is None else x0,method=method,options={'maxiter':max_calls-iters})
            if -rr['fun']>best:
                best=-rr['fun']
                print('new value =',best)
                res=(list(rr['x']),best)

    t=time()-t
    print('   ncalls     =',iters)
    print('   total time =',t)
    print('   call time  =',t/iters)
    print('   value      =', res)
    return res

if __name__ == "__main__":
    if 1:
      dres = {}
      for p in range(5,11):
        for d in range(3,8):
            rr  = find_tree_angles(p,d,1000*p,method='COBYLA',x0=None if (d-1,p) not in dres else dres[(d-1,p)][0])
            rr1 = find_tree_angles(p,d,1000000,method='COBYLA',x0=None if (d,p-1) not in dres else dres[(d,p-1)][0][:p-1]+[0]+dres[(d,p-1)][0][p-1:]+[0])
            rr = rr if rr[1]>rr1[1] else rr1
            #rr = find_tree_angles(p,d,10000,method=None,x0=None if (d,p-1) not in dres else dres[(d,p-1)][0][:p-1]+[0]+dres[(d,p-1)][0][p-1:]+[0])
            dres[(d,p)]=rr
            with open('optimized.txt','w') as f:
                for ((dd,pp),y) in sorted(list(dres.items())):
                    f.write('d = '+str(dd)+', p = '+str(pp)+' : '+str(y[1])+', '+str(y[0])+'\n')
if 0:
    #from reg_graphs_3 import gr
    p=4
    d=3
    #dlib.find_max_global()
    #beta,gamma = adjust_maxcut_angles(p,gr[10])
    for p in range(1,13):
        for d in range(3,8):
            n = 1000 if p==1 else (500 if p<=3 else (100 if p<8 else (10 if p<10 else 1)))
            with timer('QAOA for d=%d, p=%d'%(d,p),n):
                for i in range(n):
                    #r = calc_maxcut_tree_expectation(d,range(p),range(p,0,-1))
                    r1 = calc_maxcut_tree_expectation_1(d,range(p),range(p,0,-1))
                #print('r-r1 =',abs(r-r1))
                #print('res =',r)
        print('maxdim =',TNElem.maxdim)
