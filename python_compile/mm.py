from VecFunction import *
from decorators import *
import numpy as np
import time

@vec
def matmul(A,B,Y):
    for i in range(0,2048):
        for j in range(0,2048):
            for k in range(0,2048):
                Y[i][j] = Y[i][j] + A[i][k]*B[k][j];

A = 48.0 * np.array(np.random.rand(2048,2048), dtype=np.float32);
B = np.array(np.random.rand(2048,2048), dtype=np.float32);
Y0 = np.zeros((2048,2048), dtype=np.float32);
Z0 = np.zeros((2048,2048), dtype=np.float32);

AM = np.asmatrix(A);
BM = np.asmatrix(B);

noblk_rt = [];
blkd_rt = [];
matmul(A,B,Z0);

start = time.time();
matmul(A,B,Y0);
elapsed = (time.time() - start);
fps = 2*(2048*2048*2048) / (elapsed * 1024*1024)
print 'mflops=%f' % fps
print elapsed

start = time.time();
MY1 = AM*BM;
elapsed = (time.time() - start);
fps = 2*(2048*2048*2048) / (elapsed * 1024*1024)
print 'mflops=%f' % fps
print elapsed

Y1 = np.squeeze(np.asarray(MY1))

for i in range(0,2048):
    for j in range(0,2048):
        d = Y0[i][j] - Y1[i][j];
        if( d*d > 0.01):
            print 'wrong ' + str(i) + ' ' + str(j) + ' ' + str(d)
            quit();


