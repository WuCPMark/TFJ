from VecFunction import *
from decorators import *
import numpy as np

@vec
def vvadd_int(y, a, b):
    for i in range(0,1024):
        y[i] = a[i]+b[i]
            
a = np.array(np.random.rand(1024),dtype=np.int32);
b = np.array(np.random.rand(1024),dtype=np.int32);
y = np.array(np.random.rand(1024),dtype=np.int32);

vvadd_int(y,a,b);

