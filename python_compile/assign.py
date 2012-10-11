from VecFunction import *
from decorators import *
import numpy as np

@vec
def assign(y,x):
    for i in range(0,1024):
        y[i] = x[0];

y = np.array(np.random.rand(1024),dtype=np.int32);
x = np.array(np.random.rand(1),dtype=np.int32);
assign(y,x);

