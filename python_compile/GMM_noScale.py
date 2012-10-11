from VecFunction import *
from decorators import *
import numpy as np

@vec_check
def GMM_noScale(In, Mean, Var, GaussianBuffer):
    for i in range(0,5300):
        for m in range(0,16):
            for f in range(0,39):
                GaussianBuffer[i][m] = GaussianBuffer[i][m] + (In[f] - Mean[i][f][m])*(In[f] - Mean[i][f][m])*(Var[i][f][m]);
    

In = np.array(np.random.rand(39), dtype=np.float32);
Mean = np.array(np.random.rand(5300,39,16), dtype=np.float32);
Var = np.array(np.random.rand(5300,39,16), dtype=np.float32);
GaussianBuffer = np.array(np.random.rand(5300,16), dtype=np.float32);

GMM_noScale(In,Mean,Var,GaussianBuffer);



