from VecFunction import *
from decorators import *
from numpy import *

@vec
def gaussian_elimination(A, L, U, scale):
    for c in range(0,127):
        scale[0] = 1.0 / A[c][c]
        A[c][c] = 1.0;
        for i in range(0,127):
            L[i][c] = A[i][c] * scale[0]
	for j in range(0,127):
	    U[j] = A[c][j]
	for ii in range(0,127):
	    for jj in range(0,127):
	        A[ii][jj] = A[ii][jj] - L[ii][c] * U[jj]
    return L;
            
A = random.rand(128,128);
L = random.rand(128,128);
U = random.rand(128);
scale = random.rand(1);
gaussian_elimination(A,L,U, scale);

