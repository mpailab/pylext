from projectq import MainEngine
#from projectq.backends import *
from projectq.ops import QubitOperator,H,MatrixGate,All,Measure
from scipy.optimize import minimize
from math import sin,cos,pi
from networkx import random_regular_graph
import networkx
import networkx.algorithms.approximation.treewidth as twidth
from functools import wraps
import numpy as np
import yaml
import random
import dwave_networkx as dnx
from tensorstate import *
from dbcache import dbcached

# clause : ([var list], truth table | function)

def diag(l):
    return [[l[i] if i==j else 0 for i in range(len(l))] for j in range(len(l))]
def bits(x,n):
    return tuple([x>>i & 1 for i in range(n)])

def balance(C):
    cnt = dict([(i,0) for c in C for i in c[0]])
    unused = set(range(len(C)))
    cc = []
    for i in range(len(C)):
        mn = (i+1,0)
        xmin = 0
        for x in unused:
            mx = (max([cnt[j] for j in C[x][0]]), sum(cnt[j] for j in C[x][0])/len(C[x][0]))
            if mx<mn :
                mn = mx
                xmin = x
        c = C[xmin]
        #c[1] = ([c[1](bits(i,len(c[0]))) for i in range(2**len(c[0]))] if callable(c[1]) else c[1])
        for j in c[0]:
            cnt[j] = mn[0]+1
        unused.remove(xmin)
        cc.append(c)
    #print('depth = ',max(cnt))
    return cc

def frombits(b):
    return sum(b[i]<<i for i in range(len(b)))

def expectation(f, eng, qureg):
    if f.__class__ is QubitOperator:
        return eng.backend.get_expectation_value(f,qureg)
    n = len(qureg)
    res=0
    for i in range(2**n):
        b = [(i>>j) & 1 for j in range(n)]
        p = eng.backend.get_probability(b, qureg)
        res += f(b)*p
    return res

def get_operator(line):
    tb = line[1] if len(line)>1 else None
    n = len(line[0])
    op = None
    k = line[-1]
    has2 = len(line)>1
    if hasattr(tb,'__len__'):
        if len(line[1])!=2**n:
            print('constraint with', n, 'variables : second argument must be lookup table of length',2**n)
            raise Exception('constraint with '+str(n)+' variables : second argument must be lookup table of length '+str(2**n))
        tb = [int(x) for x in tb]
        #return (line[1], None)
    elif callable(tb):
        tb = [tb(*frombits(i)) for i in range(2**n)]
    else:
        op = line[0]
        tb = [sum(i>>j for j in range(n))%2 for i in range(2**n)]
        k = line[1] if has2 else 1
    if op is None:
        k = line[2] if len(line)==3 else 1
        c = tb[0]
        cc = [tb[2**i] for i in range(n)]
        if tb == [(c+sum((i>>j)&cc[j] for j in range(n)))%2 for i in range(2**n)]:
            if c: k=-k
            op = [line[0][i] for i in range(n) if cc[i]]
    if op is not None:
        op = -0.5*QubitOperator(''.join('Z'+str(i)+' ' for i in op))
    return (tb, op, k, line[0])

def truthtableall(args,tb,n):
    args==list(args)
    args.sort()
    a = np.array(tb*((2**n)//len(tb)),np.int32)
    idx = list(range(n))
    for i in range(n-len(args), n):
        t=idx[i]
        idx[i]=idx[n-args[n-i-1]-1]
        idx[n-args[n-i-1]-1]=t
    return a.reshape((2,)*n).transpose(*idx).reshape(2**n)

def create_qaoa_param_circuit(eng, C):
    s = set([q for c in C for q in c[0]]) # set of qubits
    n = max(s)+1
    if n!=len(s):
        print("Warning: not all numbers are used")
    cc = balance(C)
    ccc = [get_operator(x) for x in cc]
    def h0(x):
        return sum((y[frombits([x[i] for i in idx])]-0.5)*k for (y,_,k,idx) in ccc)
    def h_all():
        s = sum(truthtableall(idx,tb,n)*k for (tb,_,k,idx) in ccc)
        return int(max(s))
    hall = h_all()
    h = QubitOperator()
    for (_, op, k, _) in ccc:
        if op is None:
            h = h0
            break
        h += k*op

    def adjust(beta,gamma,measure=False,prdistrib=False):
        if len(beta)!=len(gamma):
            raise Exception("beta and gamma length must be the same")
        p = len(beta)
        q = eng.allocate_qureg(max(s)+1)

        def UC(g):
            #if pauli_h:
            #     TimeEvolution(g, h)| q
            #else:
                for (tb,_,k,idx) in ccc:
                    mat = diag([cos(k*g*x)-1.0j*sin(k*g*x) for x in tb])
                    MatrixGate(mat) | [q[x] for x in idx]
        def UB(b):
            All(MatrixGate([[cos(b),-1.0j*sin(b)],[-1.0j*sin(b),cos(b)]])) | q

        All(H)| q
        for i in range(p):
            UC(gamma[i])
            UB(beta[i])

        eng.flush()
        if prdistrib:
            for i in range(2**n):
                b = bits(i,n)
                print(b,":\t",eng.backend.get_probability(bits(i,n),q))
        if measure:
            All(Measure)| q
            eng.flush()

            res = [int(q[i]) for i in range(n)]
            return (res, h0(res))

        res = expectation(h, eng, q)
        rr = hall

        All(Measure)| q
        return (res+len(cc)*0.5,rr)

    return adjust

# find circuit parts on each level of QAOA to evaluate expectation i-th hamiltonian term
def get_circuit_parts(i,C,p):
    s = set(C[i][0])
    res=[[]]*p
    for i in range(p):
        res[p-i-1] = [c for c in C if any(x in s for x in c[0])]
        #print(res)
        s = {y for c in res[p-i-1] for y in c[0]}
    return (res,s)

def calc_treewidth(C,p,alledges=False,prtree=False, calcd=False,twfun=twidth.treewidth_decomp):
    s = set([q for c in C for q in c[0]]) # set of qubits
    cc = [get_circuit_parts(i,C,p)[0] for i in range(len(C))]
    twmax = 0
    twmax1=0
    worst = None
    for i in range(len(C) if alledges else 1):
        #print('consider edge '+str(C[i]))
        tn = []
        en = enumerator()
        ln = {k:0 for k in s}
        cci = [[tuple(x[0]) for x in ccx] for ccx in cc[i]]+[[tuple(C[i][0])]]
        layers = [set(cci[j])-set(cci[j+1]) for j in range(p)]+[cci[-1]]
        for j in range(p+1):
            ee = set()
            for k in range(j,p+1):
                for (b,e) in layers[k]:
                    if k>j:
                        ee.add(b); ee.add(e)
                    tn.append((en[ln[b],b],en[ln[e],e]))
                    #bi0=en[ln[b],b]; ln[b]+=1; bi1=en[ln[b],b] #ln[b]+=1
                    #ei0=en[ln[e],e]; ln[e]+=1; ei1=en[ln[e],e] #ln[e]+=1
                    #bi2=en[ln[b],b]; ei2=en[ln[e],e]
                    #tn+=[(bi0,bi1),(ei0,ei1),(bi0,ei0)] #,(bi1,bi2),(ei1,ei2)]
            for v in ee:
                b = en[ln[v],v]; ln[v]+=1; e=en[ln[v],v]
                tn.append((b,e))
        #print('tn = ',tn)
        #(tw,g) = 
        
        if prtree:
            print('tree = ',g.edges())
        if calcd:
            #(b,e)=cci[-1][0]
            #tn+=[(en[ln[b],b],en[ln[e],e])]
            for j in range(p-1,-1,-1):
                ee=set()
                for k in range(p,j-1,-1):
                    for (b,e) in layers[k]:
                        if k>j:
                            ee.add(b); ee.add(e)
                            tn.append((en[ln[b]+1,b],en[ln[e]+1,e]))
                        else:
                            tn.append((en[ln[b],b],en[ln[e],e]))
                #for j in range(p-1,-1,-1):
                #    for k in range(p,j-1,-1):
                #        for (b,e) in layers[k]:
                #            bi0=en[ln[b],b]; ln[b]+=1; bi1=en[ln[b],b]; ln[b]+=1
                #            ei0=en[ln[e],e]; ln[e]+=1; ei1=en[ln[e],e]; ln[e]+=1
                #            bi2=en[ln[b],b]; ei2=en[ln[e],e]
                #            tn+=[(bi0,bi1),(bi0,ei0),(bi0,ei1),(bi1,ei0),(bi1,ei1),(ei0,ei1),(bi1,bi2),(ei1,ei2)]
                for v in ee:
                    b = en[ln[v],v]; ln[v]+=1; e=en[ln[v],v]
                    tn.append((b,e))

            tw = twfun(networkx.Graph(tn))
            if type(tw) is tuple:
                (tw,g)=tw
            print('double tree width result = ',tw)
        else:
            tw = twfun(networkx.Graph(tn))
            if type(tw) is tuple:
                (tw,g)=tw
            print('tree width result = ',tw)
        if tw>twmax:
            worst = tn
        twmax=max(tw,twmax)
    print('\nMax treewidth = ',twmax)
    if calcd:
        print('Max double treewidth = ',twmax1)
    return (twmax,worst) if calcd else (twmax,worst)

def create_qaoa_param_circuit_energy(eng, C, p):
    s = set([q for c in C for q in c[0]]) # set of qubits
    cc = [get_circuit_parts(i,C,p)[0] for i in range(len(C))]
    #print('cc = ',cc)
    ccc = [[[get_operator(c) for c in layer] for layer in elem] for elem in cc]

    def expect_i(Ci, cs, beta, gamma):
        assert len(beta)==len(gamma)
        p = len(beta)
        mm = [0]*(max(s)+1)
        ss = {x for c in cs[0] for x in c[-1]}
        #print('cs =',cs)
        #print('ss =',ss)
        N=0
        for i in ss:
           mm[i] = N
           N+=1
        if N>30:
            raise Exception('Too many qubits : '+str(N))
        Ci[0]=[mm[i] for i in Ci[0]]
        #print('C[i] =',Ci)
        (y,op,k,idx) = get_operator(Ci)
        if op is None:
            def h(x): return (y[frombits([x[mm[i]] for i in idx])]-0.5)*k
        else: h = op

        q = eng.allocate_qureg(N)

        def UC(g,c):
            for (tb,_,k,idx) in c:
                mat = diag([cos(k*g*x)-1.0j*sin(k*g*x) for x in tb])
                MatrixGate(mat) | [q[mm[x]] for x in idx]
        def UB(b,c):
            for i in {y for x in c for y in x[-1]}:
                mat = [[cos(b),-1.0j*sin(b)],[-1.0j*sin(b),cos(b)]]
                MatrixGate(mat) | q[mm[i]]

        All(H)| q
        for i in range(p):
            UC(gamma[i],cs[i])
            UB(beta[i],cs[i])

        eng.flush()
        #print('h =',h)
        res = expectation(h, eng, q)
        All(Measure)| q
        return res+0.5

    def expect(beta,gamma):
        res = 0
        for i in range(len(C)):
            r = expect_i(C[i],ccc[i],beta,gamma)
            #print('r[',i,']=',r)
            res += r
        return res
    return expect

def create_qaoa_param_circuit_energy_tn(C, p, opt='greedy') -> ():
    s  = set([q for c in C for q in c[0]]) # set of qubits
    cc = [get_circuit_parts(i,C,p)[0] for i in range(len(C))]
    #print('cc = ',cc)
    ccc = [[[get_operator(c) for c in layer] for layer in elem] for elem in cc]
    def expect_i(Ci, cs, beta, gamma):
        assert len(beta)==len(gamma)
        p = len(beta)
        mm = [0]*(max(s)+1)
        ss = {x for c in cs[0] for x in c[-1]}
        #print('cs =',cs)
        #print('ss =',ss)
        N=0
        for i in ss:
           mm[i] = N
           N+=1
        #if N>30:
        #    raise Exception('Too many qubits : '+str(N))
        Ci[0]=[mm[i] for i in Ci[0]]
        #print('C[i] =',Ci)
        (y,_,_,idx) = get_operator(Ci)
        h = y
        #if op is None:
        #    def h(x): return (y[frombits([x[mm[i]] for i in idx])]-0.5)*k
        #else: h = op

        tn = CircState()

        def UC(g,c):
            for (tb,_,k,idx) in c:
                d = [cos(k*g*x)-1.0j*sin(k*g*x) for x in tb]
                tn.diag(d, *[mm[x] for x in idx])
                #mat = diag([cos(k*g*x)-1.0j*sin(k*g*x) for x in tb])
                #MatrixGate(mat) | [q[mm[x]] for x in idx]
        def UB(b,c):
            for i in {y for x in c for y in x[-1]}:
                tn.gate([[cos(b),-1.0j*sin(b)],[-1.0j*sin(b),cos(b)]], mm[i])
                #MatrixGate(mat) | q[mm[i]]

        for i in range(N):
            tn.gate(hmatrix, i)
        #All(H) | q

        for i in range(p):
            UC(gamma[i],cs[i])
            UB(beta[i],cs[i])

        #print('h =',h)
        res = tn.expectation(h, *idx, opt=opt) # expectation(h, eng, q)
        #All(Measure)| q
        return res

    def expect(beta,gamma):
        res = 0
        for i in range(len(C)):
            r = expect_i(C[i],ccc[i],beta,gamma)
            #print('r[',i,']=',r)
            res += r
        return res
    return expect


def readyaml(fn):
    try:
        with open(fn,'r') as f:
            r = yaml.unsafe_load(f)
            return r if type(r)==dict else {}
    except:
        return {}

def saveyaml(x,fn):
    with open(fn,'w') as f:
        yaml.dump(x,f)

_best = None
def best():
    global _best
    if _best is None:
        _best = readyaml('best_maxcut.yaml')
    return _best

def save_best():
    if _best is not None:
        saveyaml(_best,'best_maxcut.yaml')

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

_cache = {}
def cachedyaml(fn):
    def wrap(f):
        @wraps(f)
        def g(*args,**kwargs):
            cache = kwargs.pop('cache',True)
            upd = kwargs.pop('update',False)
            if not cache:
                return f(*args,**kwargs)
            if fn not in _cache:
                _cache[fn] = readyaml(fn+'.yaml')
            c = _cache[fn]
            key = conv_to_key(args if kwargs=={} else args+(kwargs,))
            if upd or key not in c:
                c.update(readyaml(fn+'.yaml'))
                c[key] = f(*args,**kwargs)
                saveyaml(c,fn+'.yaml')
            return c[key]
        return g
    return wrap

def adjust_angles(f, p, method='Nelder-Mead', pr=False, **kwargs):
    # bounds=[(0, 3.1416) for i in range(p)]+[(0, 2*3.1416) for i in range(p)],
    retry = kwargs.pop('retry',1)
    def fst(x): return x[0] if hasattr(x,'__getitem__') else x
    if type(f) is list:
        def func(x):
            return -fst(sum(fi(list(x[:p]),list(x[p:]),False)) for fi in f)
    else:
       def func(x):
           return -fst(f(list(x[:p]),list(x[p:]),False))
    init = [[pi*(i+1)/(p*p) for i in range(2*p)]]
    for i in range(1,retry):
        init.append([random.uniform(0,2*pi) for j in range(2*p)])
    res= {}
    for i in init:
        r = minimize(func, i, method=method, **kwargs)
        if res == {} or res['fun'] > r['fun']:
            res = r
            if pr:
                print(res)
    return ([float(x) for x in res['x'][:p]],[float(x) for x in res['x'][p:]])

def neq(i,j): return [[i,j]]

def maxcut(gr): return [neq(*edge) for edge in gr]

@dbcached('adjusted_maxcut_angles',ignoring='use_e')
def adjust_maxcut_angles(p, grset, **kwargs):
     eng = MainEngine()
     circs = [create_qaoa_param_circuit(eng, maxcut(gr)) for gr in grset]
     def callback(xk,r=None):
         if r is None:
             print('x =', xk)
         else:
             print('x =', xk, '\tres = ', r['fun'])
         return False
     return adjust_angles(circs, p, pr=True, callback = callback, **kwargs)

def get_opt_fun(p,grset):
    eng = MainEngine()
    circs = [create_qaoa_param_circuit(eng, maxcut(gr)) for gr in grset]
    def rt(x): return x[0]/x[1]
    def f(beta,gamma):
        return sum(rt(fi(beta,gamma,False)) for fi in circs)/len(grset)
    return f

@dbcached('best_maxcut',ignoring='use_e')
def run_maxcut(p, gr, prdistrib=False,**kwargs):
    eng = MainEngine()
    use_e=kwargs.pop('use_e',False)
    if use_e:
        circ = create_qaoa_param_circuit_energy(eng, maxcut(gr), p)
    else:
        circ = create_qaoa_param_circuit(eng, maxcut(gr))
    beta,gamma = adjust_angles(circ, p, **kwargs)
    if use_e:
        res = circ(beta, gamma)
        exact = None
    res,exact=circ(beta, gamma, measure=False, prdistrib=prdistrib)
    if prdistrib:
        print('beta  =',beta)
        print('gamma =',gamma)
        print('QAOA expectation =',res)
        if exact is not None:
            print('Exact solution   =',exact)
    return (beta,gamma,res,exact)

def run_qaoa(p,train,test,cmp=True, save_table=None, **kwargs):
    use_e = kwargs.get('use_e',False)
    print('training...')
    beta,gamma=adjust_maxcut_angles(p,train,**kwargs)
    print('beta =',beta,'\ngamma=',gamma)
    test_qaoa(p,beta,gamma,test,cmp,save_table,**kwargs)

def test_qaoa(p,beta,gamma,test,cmp=True, use_e=False, save_table=None, **kwargs):
    print('testing...')
    grnum = -1
    if save_table is not None:
        with open(save_table,'w'):
            pass
    for group in test:
        average_ratio=0
        worst_ratio=1
        worst_ratio0=1
        average_ratio0=0
        worst_loss=0
        average_loss=0

        grnum+=1
        if len(group)==0:  continue
        print('\n=============================================================')
        if hasattr(group,__name__):
            nm = group.__name__
        else:
            nv = max(max(x) for x in (group[0][0] if use_e or type(group[0]) is tuple else group[0]))
            if all(max(max(x) for x in (g[0] if use_e or type(g) is tuple else g))==nv for g in group):
                nm = str(nv+1)
            else:
                nm=str(grnum)
        print('          Testing group ',nm,'\n')
        ng=0
        for g in group:
            ng+=1
            eng = MainEngine()
            if use_e:
                circ = create_qaoa_param_circuit_energy(eng, maxcut(g[0]), p)
                res = circ(beta,gamma)
                exact = g[1]
            else:
                circ = create_qaoa_param_circuit(eng, maxcut(g if type(g) is not tuple else g[0]))
                res,exact=circ(beta,gamma,False)
            worst_ratio=min(worst_ratio,res/exact)
            average_ratio += res/exact
            if cmp:
                try:
                    res0 = run_maxcut(p,g,**kwargs)[2]
                except:
                    res0 = run_maxcut(p,g,update=True,**kwargs)[2]
                average_ratio0 += res0/exact
                worst_ratio0 = min(worst_ratio0,res0/exact)
                average_loss += (res0-res)/exact
                worst_loss = max(worst_loss, (res0-res)/exact)
                print('%3d. Test:'%ng,res,'\tTest_one:',res0,'\texact:',exact,'\tratio:',res/exact,'\tloss:',(res0-res)/exact)
            else:
                print('%3d. Test:'%ng,res,'\texact:',exact,'\tratio:',res/exact)
        average_ratio/=len(group)
        print('Worst ratio:       ', worst_ratio)
        print('Average ratio:     ', average_ratio)

        if save_table is not None:
            with open(save_table,'a') as ff:
                ff.write('%10s %8.3f %8.3f'%(nm, worst_ratio, average_ratio))

        if cmp:
            average_loss/=len(group)
            average_ratio0/=len(group)
            print('Worst own ratio:   ',worst_ratio0)
            print('Average own ratio: ',average_ratio0)
            print('Worst loss:        ',worst_loss)
            print('Average loss:      ',average_loss)
            if save_table is not None:
                with open(save_table,'a') as ff:
                    ff.write(' %8.3f %8.3f %8.3f %8.3f'%(worst_ratio0, average_ratio0,worst_loss,average_loss))
        if save_table is not None:
            with open(save_table,'a') as ff:
                ff.write('\n')

def testmaxcut(p,gr,**kwargs):
    print(gr)
    beta,gamma,res,exact = run_maxcut(p,gr,**kwargs)
    print('QAOA:',res, '\texact: ',exact,'\tratio:',res/exact)
    return res,exact

def callback(x):
    print(x)
    return False

def run_maxcut_random(fn, p, train, d, count, maxnv, **kwargs):
    print('training...')
    beta,gamma=adjust_maxcut_angles(p,train,**kwargs)
    print('beta =',beta,'\ngamma=',gamma)
    print('testing...')
    with open(fn,'w') as f:
        for nv in range(d+1,maxnv):
            if nv*d%2:continue
            average_ratio=0
            worst_ratio=1
            print("\nTesting n = ", nv)
            s=set()
            for _ in range(count):
                gr0 = random_regular_graph(d,nv)
                g=[(e1,e2) if e1<e2 else (e2,e1) for (e1,e2) in gr0.edges()]
                g.sort()
                if not tuple(g) in s:
                    eng = MainEngine()
                    circ = create_qaoa_param_circuit(eng, maxcut(g))
                    res,exact=circ(beta,gamma,False)
                    if res>exact:
                        print(g)
                        res,exact=circ(beta,gamma,False)

                    worst_ratio=min(worst_ratio,res/exact)
                    average_ratio += res/exact
                    s.add(tuple(g))
                    print('Test:',res,'\texact:',exact,'\tratio:',res/exact)
            average_ratio/=len(s)
            print('Worst ratio:       ', worst_ratio)
            print('Average ratio:     ', average_ratio)
            f.write('%5d %8.4f %8.4f\n'%(nv,worst_ratio,average_ratio))
            f.flush()

def scan2d(gr,xrange,yrange,nx,ny,fn):
    ff = get_opt_fun(1, gr)
    with open(fn+'.txt','w') as f:
        for i in range(nx):
            for j in range(ny):
                r = ff([i*xrange/nx],[j*yrange/ny])
                print((i*xrange/nx,j*yrange/ny),'-->',r)
                f.write('%8.4f '%r)
            f.write('\n')

def scan4d(gr,xrange,yrange,nx,ny,fn):
    ff = get_opt_fun(1, gr)
    with open(fn+'.txt','w') as f:
        for b1 in range(nx):
            for b2 in range(nx):
                beta = [b1*xrange/nx,b2*xrange/nx]
                for g1 in range(ny):
                    for g2 in range(ny):
                        gamma = [g1*yrange/ny,g2*yrange/ny]
                        r = ff(beta,gamma)
                        print((beta,gamma),'-->',r)
                        f.write('%8.4f '%r)
                    f.write('\n')

def gentree(d,p,doprint=True):
    nv0=0
    nv1=2
    n=2
    ed=[(0,1)]
    for _ in range(p):
        for j in range(nv0,nv1):
            for _ in range(d-1):
                ed.append((j,n))
                n+=1
        nv0=nv1; nv1=n
    if doprint:
        print('edges = ',ed)
    return ed

def readtable(fn:str) -> list:
    res = []
    def elem(x:str):
        try: return eval(x)
        except: return x
    with open(fn,'r') as f:
        for line in f:
            l = [elem(x) for x in line.split()]
            if len(l)>0:
                res.append(l)
    return res

def load_angles(f:str) -> dict:
    ang = readtable(f)
    res = {}
    for [d,p,val,*args] in ang:
        res[d,p] = (val,args[:p],args[p:])
    return res

def save_cnf(ed, filename):
    vert = set(sum((list(x) for x in ed),[]))
    v =  max(vert) + 1
    e = len(ed)
    cnf = "p cnf {} {}\n".format(v, e)
    for (b,e) in ed:
        cnf += "{} {} 0\n".format(b+1,e+1)

    with open(filename, 'w') as fp:
        fp.write(cnf)
from reg_graphs_3 import gr as gr3
if __name__ == "__main__":
    #testmaxcut(2,[(0, 5), (0, 8), (0, 11), (1, 6), (1, 9), (1, 13), (2, 7), (2, 13), (2, 15), (3, 9), (3, 10), (3, 14), (4, 10), (4, 11), (4, 12), (5, 8), (5, 11), (6, 12), (6, 13), (7, 14), (7, 15), (8, 12), (9, 15), (10, 14)],
    #    callback = callback,
    #    update=True,# method='Powell',
    #    options={'disp':True})
    #gr=[(0, 5), (0, 8), (0, 11), (1, 6), (1, 9), (1, 13), (2, 7), (2, 13), (2, 15), (3, 9), (3, 10), (3, 14), (4, 10), (4, 11),
    #    (4, 12), (5, 8), (5, 11), (6, 12), (6, 13), (7, 14), (7, 15), (8, 12), (9, 15), (10, 14)]
    #eng = MainEngine(CommandPrinter())
    #circ = create_qaoa_param_circuit(eng, maxcut(gr))
    #circ([1,1],[1,1],True)
    #update_cache('best_maxcut',readyaml('best_maxcut.yaml'))
    #update_cache('adjusted_maxcut_angles',readyaml('adjusted_maxcut_angles.yaml'))
    #import reg_graphs_3
    #p=2
    #beta,gamma=adjust_maxcut_angles(p,reg_graphs_3.gr[10])
    #for i in range(100):
    #    g = random_regular_graph(3,1000).edges()
    #    #g=[(0,1),(1,2),(2,3),(0,3)]
    #    #(beta,gamma,res,exact) = run_maxcut(p, g)
    #    eng = MainEngine()
    #    circ = create_qaoa_param_circuit_energy(eng, maxcut(g), p)
    #    res1 = circ(beta,gamma)
    #    print('res = ',res1)

    #_caches['adjusted_maxcut_angles'].printkeys()
    #import reg_graphs_3
    #import tbmaxcut
    #beta=[2.5866326234959307, 4.42014107803203]; gamma=[3.629728195929617, 0.8982821639811691]
    #test_qaoa(2,beta,gamma,tbmaxcut.grres,cmp=False,use_e=True,save_table='simnew100_p2.txt')
    beta=[4.13101981469674, 3.3970593855650613, 4.896034917679501]; gamma=[5.83234634254089, 2.0991556650387073, 4.732933216312497]
    beta,gamma=([-0.9619, -2.6820, -1.8064],[2.7197, 5.4848, 2.2046])
    #test_qaoa(3,beta,gamma,tbmaxcut.grres,cmp=False,use_e=True,save_table='simnew100_p3.txt')
    #beta,gamma=([2.1713551452853417, 2.0114833009871433, 0.3096838477649316, 6.123407623684938], [2.7347182075635477, 5.504637863356622, 5.317438209137507, 2.021956828546331])
    #test_qaoa(4,beta,gamma,tbmaxcut.grres[:2],cmp=False,use_e=False,save_table='simnew20_p4.txt')
    #beta,gamma=([2.17554677078105, 2.049155187108996, 0.2399125715004379, 6.108584918250921, -0.1193882770826509], [2.7538554361439735, 5.539747632341281, 5.308211127992548, 1.6380280429611327, 0.9712543564973646])
    #test_qaoa(5,beta,gamma,tbmaxcut.grres[:2],cmp=False,use_e=False,save_table='simnew20_p5.txt')
    #run_qaoa(1,reg_graphs_3.gr[10], tbmaxcut.grres, cmp=False, use_e=True,save_table='sim100_p1.txt')
    #run_qaoa(2,reg_graphs_3.gr[10], tbmaxcut.grres, cmp=False, use_e=True,save_table='sim100_p2.txt')
    #run_qaoa(3,reg_graphs_3.gr[10], tbmaxcut.grres[9:10], retry=10, cmp=False, use_e=True, save_table='sim100_p3-1.txt')
    #run_qaoa(5, reg_graphs_3.gr[8], [[(gentree(3,5),100)]], #[[(random_regular_graph(3,1000).edges(),1500)]], #tbmaxcut.grres[9:10],
    #    cmp=False, use_e=True, save_table='sim100_p5-1.txt')
    #calc_treewidth(maxcut(gentree(3,5,doprint=False)),5)

    #angles = load_angles('all_opt1.txt')
    gr  = maxcut(random_regular_graph(3,70).edges())
    res = list()

    d = 3
    calcd = 0
    #for p in range(3,7):
        #print('=============== p = ',p,' ===================')
        #for g in gr3:
        #
        #val, beta, gamma = angles[d,p]
        #test_qaoa(p, beta, gamma, gr3, cmp=False, use_e=False, save_table='sim_qaoa_r3_p'+str(p)+'.txt')
    tf = twidth.treewidth_min_fill_in #,dnx.min_width_heuristic,dnx.max_cardinality_heuristic,dnx.minor_min_width]:
    for p in range(7):
        #print(tf.__name__,':')
        res.append((tf.__name__, p) + calc_treewidth(gr, p, alledges=True, calcd=calcd, twfun = tf) + calc_treewidth(gr,p,alledges=True, calcd=calcd,twfun = dnx.minor_min_width))
        (fn,p,r,g,l,_) = res[-1]
        print('f = ', fn,',\tp =',p,':',r,'; lower = ',l)
        save_cnf(g,'cnf_'+str(p)+'-2.txt')
        #(tw,g) = calc_treewidth(maxcut(gentree(3,5,doprint=False)), p, alledges=False, calcd=calcd)
        #(twm,g) = calc_treewidth(maxcut(gentree(3,5,doprint=False)), p, alledges=False, calcd=calcd,twfun = dnx.minor_min_width)
        #circ = create_qaoa_param_circuit_energy_tn(gr, p, opt='greedy')
        #print('worst graph =', res[-1][-1])
        #print('run expectation with \n    beta = ',beta,'\n    gamma =',gamma)
        #r = circ(beta,gamma)
        #print('result(',d,',',p,') = ',r)
        #print('graph6 = ',networkx.readwrite.to_graph6_bytes(networkx.Graph(res[-1][-1]))[10:].decode('utf-8'))
        #print("result(",p,") = ",tw,', lower = ',twm)
        #save_cnf(g,'tree_p'+str(p)+'.txt')

    for (fn,p,r,g,l,_) in res: #,l,_) in res:
        print('f = ', fn,',\tp =',p,':',r,'; lower = ',l)
        save_cnf(g,'cnf_'+str(p)+'.txt')
    #scan4d(reg_graphs_3.gr[8],pi/2,pi,20,20,'pltp2_gr8-20pt')
    #run_maxcut_random('res_mc_p5_d3.txt', 5, reg_graphs_3.gr[8],3,100,30)#, retry=10)#, update=True)

    #gr = [x for x in reg_graphs_3.gr]
    #run_qaoa(2, gr[10], gr, True)#, method='Powell')

if 0:
    for d in [3,4,5]:
        for nv in range(d+1, d+10):
            if nv*d%2:continue
            print("\nTesting n = ",nv)
            s=set()
            g=[]
            for i in range(100):
                gr0 = random_regular_graph(d,nv)
                gr=gr0.edges()
                isom=False
                for x in g:
                    if networkx.is_isomorphic(x,gr0):
                        isom=True
                if not isom and not tuple(gr) in s:
                    s.add(tuple(gr))
                    g.append(gr0)
                    testmaxcut(gr,1)
                    if len(g)>20: break

#gr = [(0,1),(1,2),(2,3),(3,0)]
#circ = create_qaoa_param_circuit(eng, maxcut(gr))
#beta,gamma = adjust_angles(circ, 2)
##beta=gamma=[0.5,0.5]
#print('beta =', beta)
#print('gamma = ',gamma)
#print('res =', circ(beta,gamma,measure=False))
#
#for i in range(10):
#    print(circ(beta,gamma))
