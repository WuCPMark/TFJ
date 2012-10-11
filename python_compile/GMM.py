from VecFunction import *
from decorators import *
import numpy as np

@vec_check
def GMM(In, Mean, Var, GaussianBuffer):
    for i in range(0,3006):
        for m in range(0,16):
            for f in range(0,39):
                GaussianBuffer[i][m] = GaussianBuffer[i][m] + (In[f] - Mean[i][f][m])*(In[f] - Mean[i][f][m])*(Var[i][f][m]);
            
In = np.array(np.random.rand(39), dtype=np.float32);
Mean = np.array(np.random.rand(3006,39,16), dtype=np.float32);
Var = np.array(np.random.rand(3006,39,16), dtype=np.float32);
GaussianBuffer = np.array(np.random.rand(3006,16), dtype=np.float32);

GMM(In,Mean,Var,GaussianBuffer);



