from VecFunction import *
from decorators import *
import numpy as np
import time

@vec
def mm_test(A,B,Y):
    for i in range(0,n):
        for j in range(0,n):
            for k in range(0,n):
                Y[i][j] = Y[i][j] + A[i][k]*B[k][j];


l = [16,32,64,128,256,512,1024,2048,4096];
f = [];
for i in l:
    A = np.array(np.random.rand(i,i), dtype=np.float32);
    B = np.array(np.random.rand(i,i), dtype=np.float32);
    AM = np.asmatrix(A);
    BM = np.asmatrix(B);
    MY1 = AM*BM;

    Y0 = np.zeros((i,i), dtype=np.float32);
    n = np.int32(i);

    start = time.time();
    mm_test(A,B,Y0);
    elapsed = (time.time() - start);
  
    fps = 2*(i*i*i) / (elapsed * 1024*1024)
    print 'n = ' + str(n) + ' => mflops= ' +  str(fps)
    f.append(fps);

    Y1 = np.squeeze(np.asarray(MY1));
    for ii in range(0,i):
        for jj in range(0,i):
            d = Y0[ii][jj] - Y1[ii][jj];
            if( d*d > 0.01):
                print 'wrong ' + str(ii) + ' ' + str(jj) + ' ' + str(d)
                quit();


  #print elapsed

print f
