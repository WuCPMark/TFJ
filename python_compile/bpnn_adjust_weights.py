from VecFunction import *
from decorators import *
import numpy as np

@vec
def bpnn_adjust_weights(delta, ndelta, ly, nlyp1, w, oldw):
    for j in range(0, ndelta):
        for k in range(0, nlyp1):    
            w[k][j+1] = w[k][j+1] + ((0.3 * delta[j+1] * ly[k]) + (0.3 * oldw[k][j+1]));
            oldw[k][j+1] = ((0.3 * delta[j+1] * ly[k]) + (0.3 * oldw[k][j+1]));

def py_adjust(delta, ndelta, ly, nlyp1, w, oldw):
    for j in range(0, ndelta):
        for k in range(0, nlyp1):    
            w[k][j+1] = w[k][j+1] + ((0.3 * delta[j+1] * ly[k]) + (0.3 * oldw[k][j+1]));
            oldw[k][j+1] = ((0.3 * delta[j+1] * ly[k]) + (0.3 * oldw[k][j+1]));

delta = np.array(np.random.rand(1024), dtype=np.float32);
ndelta = np.int32(512);
ly = np.array(np.random.rand(1024), dtype=np.float32);
nlyp1 = np.int32(511);

w0 = 128.0 * np.array(np.random.rand(1024,1024), dtype=np.float32);
oldw0 = 231.0 *np.array(np.random.rand(1024,1024), dtype=np.float32);

w1 = w0
oldw1 = oldw0


print 'getting ready to run bpnn'
bpnn_adjust_weights(delta, ndelta, ly, nlyp1, w0, oldw0);
print 'done :)'
py_adjust(delta, ndelta, ly, nlyp1, w1, oldw1);

for i in range(0,1024):
    for j in range(0,1024):
        d = w0[i][j]-w1[i][j];
        d = d*d;
        if(d > 0.01):
            print 'error @ ' + str(i) + ' ' + str(j);
            quit();
