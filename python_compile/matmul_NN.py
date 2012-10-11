from VecFunction import *
from decorators import *
import numpy as np

@vec
def matmul_NN(A, B, C):
    for i in range(0,127):
        for j in range(0,127):
            for k in range(0,127):
                C[i][j] = C[i][j] + A[i][k]*B[k][j];
    return C;
            
A = np.array(np.random.rand(128,128), dtype=np.float32);
B = np.array(np.random.rand(128,128), dtype=np.float32);
C = np.array(np.random.rand(128,128), dtype=np.float32);

matmul_NN(C,A,B);
