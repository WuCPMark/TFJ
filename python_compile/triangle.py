from VecFunction import *
from decorators import *
import numpy as np

@vec
def triangle(Y):
    for i in range(0,8):
        for j in range((i+1),8):
            Y[i][j] = 1;

Y = np.array(np.random.rand(8,8), dtype=np.float32);
print 'getting ready to run triangle'
triangle(Y);
