from VecFunction import *
from decorators import *
import numpy as np

@vec_check
def fuckMe(A, X):
    for i in range(0,15):
        A[i] = X[i+1] + X[i];
        X[i+1] = A[i] + 10;

A = np.array(np.random.rand(16), dtype=np.float32);
X = np.array(np.random.rand(16), dtype=np.float32);

fuckMe(A,X);



