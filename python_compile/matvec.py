from VecFunction import *
from decorators import *
import numpy as np

@vec
def matvec(A, X, Y):
    for i in range(0,127):
        for j in range(0,127):
            Y[i] = Y[i] + A[i][j]*X[j];

            
A = np.array(np.random.rand(128,128), dtype=np.float32);
X = np.array(np.random.rand(128), dtype=np.float32);
Y = np.array(np.random.rand(128), dtype=np.float32);
matvec(A,X,Y);
