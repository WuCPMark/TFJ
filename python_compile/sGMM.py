from VecFunction import *
from decorators import *
import numpy as np
import time

@vec
def sGMM(In, Mean, Var, GaussianBuffer, Idx, n):
    for i in range(0,n):
        for f in range(0,39):
            for m in range(0,16):
                ii = Idx[i];
                GaussianBuffer[ii][m] = GaussianBuffer[ii][m] + (In[f] - Mean[ii][f][m])*(In[f] - Mean[ii][f][m])*(Var[ii][f][m]);
            
In = np.array(np.random.rand(39), dtype=np.float32);
Mean = np.array(np.random.rand(3006,39,16), dtype=np.float32);
Var = np.array(np.random.rand(3006,39,16), dtype=np.float32);
GaussianBuffer = np.array(np.random.rand(3006,16), dtype=np.float32);
Idx = np.array(np.random.rand(3006), dtype=np.int32);
n = np.int32(800);

for i in range(0,16):
    start = time.time();
    sGMM(In,Mean,Var,GaussianBuffer,Idx,n);
    elapsed = (time.time() - start);
    print 'run = %f' % elapsed



