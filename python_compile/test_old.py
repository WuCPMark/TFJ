from VecFunction import *
from decorators import *

@vec
def test(x, c):
	for i in range(1,10):
		x[i] = x[i+1] + 1
	return 
