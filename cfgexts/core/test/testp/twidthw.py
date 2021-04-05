import sys

import networkx as nx
import itertools
from copy import copy
import networkx.drawing.nx_pylab as nxp
from tensorstate import enumerator

def schedule(G,w,weight=1):
    g = {n: set(G[n]) - set([n]) for n in G}
    nv = len(g)
    mw = max(w)
    vs = set()
    vr = set(g.keys())
    vo = []
    mx = 0
    for _ in range(nv):
        minadd = mw*nv+1
        mins = 0
        for v in vr:
            v1 = {x for x in g[v] if x in vr}
            vx = vr - {v} #copy(set(vr))
            gg = G.subgraph(vx) #copy(G)
            #gg.remove_node(v1)
            mm = 0
            ms = 0
            for c in nx.connected_components(gg):
                #w0 = -w(v) if v in c and v in vs else 0
                #s=0
                #for u in c:
                #    if (u in v1 or u in vs):
                #        for x in g[u]:
                #            if x not in vx:
                s = sum(w[v] for v in c if (v in v1 or v in vs))
                vw = w[v]*weight if v in v1 or v in vs else 0
                if s-vw>mm:
                    mm = s-vw
                    ms = s
            if mm<minadd:
                minadd = mm
                ming = gg
                minv = v
                minv1 = v1
                mins = ms
        mx = max(mins,mx)
        G = ming
        vo.append(minv)
        vs |= minv1
        vs.discard(minv)
        vr.discard(minv)
    vo.reverse()
    return (mx,vo)

def s2(u,v): return (u,v) if u<v else (v,u)

def schedule_edges(G,w):
    g = {n: set(G[n]) - set([n]) for n in G}

    nv = len(g)
    mw = max(w)
    vr = {s2(a,b) for (a,b) in G.edges()}
    vo = []
    mx = 0
    while len(vr):
        minadd = mw*nv+1
        for e in vr:
            vx = vr - {e} #copy(set(vr))
            gg = nx.Graph(list(vx)) #copy(G)
            mm = 0
            for c in nx.connected_components(gg):
                s = 0
                for u in c:
                    wt = 0
                    has=False
                    for ww in g[u]:
                        if s2(u,ww) in vx:
                            has = True
                        else:
                            wt = max(wt,w[ww])
                    if has:
                        s+=wt
                mm=max(s,mm)
            if mm<minadd:
                minadd = mm
                ming = gg
                mine = e
        mx = max(minadd, mx)
        G = ming
        vo.append(mine)
        vr.discard(mine)
    vo.reverse()
    return (mx,vo)

def schedule_edges1(G,we):
    cs = [G]
    for e in G.edges():
        e.capacity = min(we[e[0]], we[e[1]])
    while cs!=[]:
        c = cs.pop()
        for e in c.edges():
            cut = nx.minimum_cut(c,)

def neighborhood(G,vv,p):
    ww={x:0 for x in vv}
    ww1=copy(ww)
    en = enumerator()
    vv=set(vv)
    en += vv
    for _ in range(p):
        vv |= set(sum((list(G[x]) for x in vv), []))
        en += vv
        for x in vv:
            if x in ww1: ww1[x]+=1
            else: ww1[x]=1
            if x in ww: ww[x]+=1
            else: ww[x]=0
    ww = {en[x]:y for (x,y) in ww.items()}
    ww1 = {en[x]:y for (x,y) in ww1.items()}
    ed = sorted([s2(en[b],en[e]) for (b,e) in G.subgraph(vv).edges() if ww[en[b]]>0 or ww[en[e]]>0])
    return (nx.Graph(ed),ww,ww1)

if __name__=='__main__':
    g = nx.random_regular_graph(3,100)
    e = list(g.edges())
    for p in range(1,6):
        print('\n=========== p = %d ==========='%p)
        res = neighborhood(g, e[0], p)
        n,w,w1 = res
        print(n.edges())
        #nxp.draw(n)
        print('weights =', w)
        mx,sch = schedule(n, w)
        print('schedule =', sch)
        print('worst =', mx)
        print('----- edges: ------')
        mx,sch = schedule_edges(n, w1)
        print('schedule =', sch)
        print('worst =', mx)
