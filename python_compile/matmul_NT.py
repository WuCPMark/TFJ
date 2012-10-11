from VecFunction import *
from decorators import *
import numpy as np

@vec
def matmul_NT(A, B, C):
    for i in range(0,127):
        for j in range(0,127):
            for k in range(0,127):
                C[i][j] = C[i][j] + A[i][k]*B[j][k];
            
A = np.array(np.random.rand(128,128), dtype=np.float32);
B = np.array(np.random.rand(128,128), dtype=np.float32);
C = np.array(np.random.rand(128,128), dtype=np.float32);
matmul_NT(C,A,B);
