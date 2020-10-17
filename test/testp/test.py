import time
#%%
from cmodules import *
from simulator import *

test='sim'
#%%
if test=='detool':
    detool = loadCModule('detool') #CLink(path = 'D:/programmes/svn/tools/detool/detool.exe')
    rr = detool.findQSpecQCCSS([[[0,43,31,102,15], [0,64,67,94,112]]],[[[0,33,63,60,15],[0,25,84,96,112]]], 127, 10, numcw=3, onlyX=True)
#%%
if test=='sim':
    sim = Simulator(quiet=True) # loadCModule('QECC simulator') #CLink(path="D:/programmes/svn/qsim/qcode/bin/qcode.exe")
    
    sim.code = {"classname":"ldpc", "matrix":([[[0,43,31,102,15], [0,64,67,94,112]]],[[[0,33,63,60,15],[0,25,84,96,112]]],127)}
    sim.decoder = {"ldpcAccum":1,
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
    channel = { "classname" : "depolarizing" }
    #sim.run={
    #    "nThreads":1, # number of simulation threads
    #    "start":0.1,  # start noise
    #    "step":0.01,  # noise step
    #    "units":"P",    # noise units ('P' for depolarizing and independent, 'factor' for circuitBased and phenomenologcal models)
    #    "dumpW":0,
    #    "dumpInterval":1,
    #    "outputFormat":"table",
    #    "output": "res_test26_1.txt",
    #    "enoughNumErrors":100,
    #    "numWords":10000000000,
    #    "dumpStats":["BER", "WER", "FAR", "numWords", "numErrors", "numMiscorrections", "minDist"]
    #  }
    sim.num_threads=1
    for i in range(1):
        #res = simulate_plot(sim, fmap(SimpleThermalBath, linscale(0.5,0.5)), stop_by_limits(10000000,10), stop_sim_by_wer(1e-5))
        res = simulate_plot(sim, fmap(ThermalBathMC, linscale(0.5,0.5)), stop_by_limits(10000000,10), stop_sim_by_wer(1e-5))
        res.wait()
        #res = sim.simulate(code=code,decoder=decoder,channel=channel,run=run)
        #time.sleep(5)
        
        res.cancel()
        print(res.state)

#%%
