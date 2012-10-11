from vecfunction import *
from decorators import *

@vec
def test(A, B, C):
    for i in range(1,1024):
        for j in range(1,1024):
            for k in range(1,1024):
                C[i][j] = C[i][j] + A[i][k] * B[k][j]
