from VecFunction import *
from decorators import *
import numpy as np

@vec
def matmul_int(A, B, C):
    for i in range(0,1024):
        for j in range(0,1024):
            for k in range(0,1024):
                C[i][j] = C[i][j] + A[i][k]*B[k][j];
    return C;
            
A = np.array(np.random.rand(1024,1024), dtype=np.int32);
B = np.array(np.random.rand(1024,1024), dtype=np.int32);
C = np.array(np.random.rand(1024,1024), dtype=np.int32);

matmul_int(C,A,B);
