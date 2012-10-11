from VecFunction import *
from decorators import *
import numpy as np

@vec
def fusion(A, B, C):
    for i in range(0, 16):
        A[i] = B[i] * 2.0
        C[i] = A[i] + B[i]

A = np.array(np.random.rand(16), dtype=np.float32);
B = np.array(np.random.rand(16), dtype=np.float32);
C = np.array(np.random.rand(16), dtype=np.float32);

fusion(A,B,C);




