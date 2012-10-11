from VecFunction import *
from decorators import *
import numpy as np

@vec
def array_doubler(y, a):
    for i in range(0,1024):
        y[i] = 2.0 * a[i];
            
a = np.array(np.random.rand(1024),dtype=np.float32);
y = np.array(np.random.rand(1024),dtype=np.float32);

for i in range(0,1024):
    y[i] = 0;
    a[i] = i;

print y
for i in range(0,10):
    array_doubler(y,a)


print y



