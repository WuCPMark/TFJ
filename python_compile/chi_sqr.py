from VecFunction import *
from decorators import *
import numpy as np

@vec
def chi_sqr(A,B,Y):
    for i in range(0,1024):
        for j in range(0,1024):
            for k in range(0,1024):
                Y[i][j] = Y[i][j] + (A[i][k] - B[k][j])*(A[i][k] - B[k][j]) / (A[i][k] - B[k][j]);
            Y[i][j] = Y[i][j] * 0.5

A = np.array(np.random.rand(1024,1024), dtype=np.float32);
B = np.array(np.random.rand(1024,1024), dtype=np.float32);
Y = np.array(np.random.rand(1024,1024), dtype=np.float32);
print 'getting ready to run chi_sqr'
chi_sqr(A,B,Y);
