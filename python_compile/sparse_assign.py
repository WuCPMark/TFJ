from VecFunction import *
from decorators import *
import numpy as np

@vec
def sparse_assign(y, idx):
    for i in range(0,1024):
        y[idx[i]] = 1
        
y = np.array(np.random.rand(1024),dtype=np.float32);
idx = np.array(np.random.rand(1024), dtype=np.int32);
sparse_assign(y,idx);

