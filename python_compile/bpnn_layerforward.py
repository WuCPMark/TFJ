from VecFunction import *
from decorators import *
import numpy as np

@vec
def bpnn_layerforward(sums, l1, l2, conn, n1p1, n2):
    for j in range(0, n2):        
        sums[j] = 0.0
        for k in range(0, n1p1):
            l2[j] = l2[j] + conn[k][j] * l1[k];
        l2[j] = 1.0 / (1.0 + l2[j])
        #l2[j] = sums[j]

def py_layer(sums, l1, l2, conn, n1p1, n2):
    for j in range(0, n2):        
        sums[j] = 0.0
        for k in range(0, n1p1):
            l2[j] = l2[j] + conn[k][j] * l1[k];
        l2[j] = 1.0 / (1.0 + l2[j])
        #l2[j] = sums[j]
            

sums_0 = np.array(np.random.rand(1024), dtype=np.float32);
sums_1 = sums_0;

l1 = np.array(np.random.rand(1024), dtype=np.float32);
l2_0 = 256.0 * np.array(np.random.rand(1024), dtype=np.float32);
l2_1 = l2_0;

conn = np.array(np.random.rand(1024,1024), dtype=np.float32);
n1p1 = np.int32(8);
n2 = np.int32(16);


bpnn_layerforward(sums_0, l1, l2_0, conn, n1p1, n2)
print 'done with specialized, running py version'
py_layer(sums_1, l1, l2_1, conn, n1p1, n2)

for i in range(0,1024):
    d = sums_0[i]-sums_1[i];
    d = d*d;
    if(d > 0.01):
        print 'sums: error @ %d' % i
        quit();

for i in range(0,1024):
    d = l2_0[i]-l2_1[i];
    d = d*d;
    if(d > 0.01):
        print 'l2: error @ %d' % i
        quit();

print 'correctness check complete'
