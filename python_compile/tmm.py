from VecFunction import *
from decorators import *
import numpy as np

@vec
def tmm(A,B):
    for i in range(0,1024):
        for j in range(0,1024):
            for k in range(0,1024):
                A[i][j] = A[i][j] + A[i][k]*B[k][j];

A = np.array(np.random.rand(1024,1024), dtype=np.float32);
B = np.array(np.random.rand(1024,1024), dtype=np.float32);
print 'getting ready to run tmm'
tmm(A,B);
