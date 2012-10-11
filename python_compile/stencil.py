from VecFunction import *
from decorators import *
import numpy as np

@vec
def stencil(In, Out):
    for i in range(0,1023):
        for j in range(0,1023):
            Out[1*i+0][1*j+0] = (In[1*i+1][1*j] + In[1*i-1][1*j] + In[1*i][1*j+1] + In[1*i][1*j-1])*0.25

In = np.array(np.random.rand(1024,1024), dtype=np.float32);
Out = np.array(np.random.rand(1024,1024), dtype=np.float32);

stencil(In,Out)

