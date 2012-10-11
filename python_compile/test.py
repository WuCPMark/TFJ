from VecFunction import *
from decorators import *

@vec
def test(A, B, C):
    for i in range(0,1024):
        B[i] = 5.0
        for j in range(0,1024):
            C[i][j] = B[i]

            
         
