from VecFunction import *
from decorators import *
import numpy as np

@vec
def axpy(alpha, y, a, b):
    for i in range(0,1024):
        y[i] = alpha[0]*a[i]+b[i]

@vec
def dot(y,a,b):
    for i in range(0,1024):
        y[0] = y[0] + a[i]*b[i];
            
a = np.array(np.random.rand(1024),dtype=np.float32);
b = np.array(np.random.rand(1024),dtype=np.float32);
y = np.array(np.random.rand(1024),dtype=np.float32);
alpha = np.array(np.random.rand(1),dtype=np.float32);

for i in range(0,1024):
    y[i] = 0;

print y
for i in range(0,10):
    axpy(alpha, y,a,b);

dot(y,a,b);

print y



