from VecFunction import *
from decorators import *
import numpy as np

@vec
def nofuse(A, B, C):
    for i in range(0, 16):
        A[i-1] = A[i+3] * 2.0
        A[i+3] = A[i-1] + B[i]

A = np.array(np.random.rand(16), dtype=np.float32);
B = np.array(np.random.rand(16), dtype=np.float32);
C = np.array(np.random.rand(16), dtype=np.float32);

nofuse(A,B,C);




