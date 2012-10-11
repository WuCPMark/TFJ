from VecFunction import *
from decorators import *
import numpy as np

@vec
def distribution(A, B, C, D):
    for i in range(0,128):
        for j in range(0,128):
            A[i][j] = B[i][j] + C[i][j]
            D[i][j] = A[i][j-1] * 2.0
               
A = np.array(np.random.rand(128,128), dtype=np.float32);
B = np.array(np.random.rand(128,128), dtype=np.float32);
C = np.array(np.random.rand(128,128), dtype=np.float32);
D = np.array(np.random.rand(128,128), dtype=np.float32);

distribution(A,B,C,D);
