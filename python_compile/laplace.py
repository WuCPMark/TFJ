from VecFunction import *
from decorators import *
import numpy as np

@vec
def laplace(In,Out):
    for i in range(1,1023):
        for j in range(1,1023):
            Out[i][j] = (In[i-1][j] + In[i+1][j] + In[i][j-1] + In[i][j+1])/4;

In = np.array(np.random.rand(1024,1024), dtype=np.float32);
Out = np.array(np.random.rand(1024,1024), dtype=np.float32);
print 'getting ready to run matmul'
laplace(In,Out);
