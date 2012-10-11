from VecFunction import *
from decorators import *
import numpy as np

@vec
def sparse_vvadd(y, a, b, idx):
    for i in range(0,1024):
        y[idx[i]] = a[idx[i]]+b[idx[i]]
            
a = np.array(np.random.rand(1024),dtype=np.float32);
b = np.array(np.random.rand(1024),dtype=np.float32);
y = np.array(np.random.rand(1024),dtype=np.float32);
idx = np.array(np.random.rand(1024), dtype=np.int32);
sparse_vvadd(y,a,b,idx);

