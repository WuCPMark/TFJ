from VecFunction import *
from decorators import *
from numpy import *

@vec
def matmul_TT(A, B, C):
    for i in range(0,1023):
        for j in range(0,1023):
            for k in range(0,1023):
                C[i][j] = C[i][j] + A[k][i]*B[j][k];
            
A = random.rand(1024,1024);
B = random.rand(1024,1024);
C = random.rand(1024,1024);
matmul_TT(C,A,B);
