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

def py_matmul(A,B,Y):
    for i in range(0,2048):
        for j in range(0,2048):
            for k in range(0,2048):
                Y[i][j] = Y[i][j] + A[i][k]*B[k][j];

@vec
def bmatmul(A,B,Y):
    for ii in range(0,8):
        for jj in range(0,8):
            for kk in range(0,8):
                for i in range(0,256):
                    for j in range(0,256):
                        for k in range(0,256):
                            Y[i+ii*256][j+jj*256] = Y[i+ii*256][j+jj*256] + A[i+ii*256][k+kk*256]*B[k+kk*256][j+jj*256];

A = 2048.0 * np.array(np.random.rand(2048,2048), dtype=np.float32);
B = 2048.0 * np.array(np.random.rand(2048,2048), dtype=np.float32);
Y0 = np.zeros((2048,2048), dtype=np.float32);
Y1 = np.zeros((2048,2048), dtype=np.float32);

noblk_rt = [];
blkd_rt = [];

for i in range(0,8):
    start = time.time();
    matmul(A,B,Y0);
    elapsed = (time.time() - start);
    noblk_rt.append(elapsed);
    fps = 2*(2048*2048*2048) / (elapsed * 1024*1024)
    print 'mflops=%f' % fps
    print elapsed

for i in range(0,8):
    start = time.time();
    bmatmul(A,B,Y1);
    elapsed = (time.time() - start);
    blkd_rt.append(elapsed);
    fps = 2*(2048*2048*2048) / (elapsed * 1024*1024)
    print 'mflops=%f' % fps
    print elapsed



print 'getting ready to run matmul'
#matmul(A,B,Y0);
#print 'optmized mm done'
#py_matmul(A,B,Y1);
#bmatmul(A,B,Y1);

for i in range(0,2048):
    for j in range(0,2048):
        d = Y0[i][j] - Y1[i][j];
        if( d*d > 0.01):
            print 'wrong ' + str(i) + ' ' + str(j) + ' ' + str(d)
            quit();

for i in range(0,8):
    print noblk_rt[i] / blkd_rt[i]

