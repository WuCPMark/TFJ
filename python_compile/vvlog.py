from VecFunction import *
from decorators import *
import numpy as np

@vec
def vvlog(y, a, b):
    for i in range(0,1024):
        y[i] = log(a[i])
            
a = np.array(np.random.rand(1024),dtype=np.float32);
b = np.array(np.random.rand(1024),dtype=np.float32);
y = np.array(np.random.rand(1024),dtype=np.float32);

for i in range(0,1024):
    y[i] = 0;

print y
for i in range(0,10):
    vvlog(y,a,b);

print y



