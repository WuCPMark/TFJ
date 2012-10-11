from VecFunction import *
from decorators import *
import numpy as np

@vec
def colorConv(cin,cout,mat):
    for p in range(0,16384):
        for i in range(0,3):
            for j in range(0,3): 
                cout[p][i] = cout[p][i] + cin[p][j]*mat[i][j];

cin = np.array(np.random.rand(16384,3),dtype=np.int32);
cout = np.array(np.random.rand(16384,3),dtype=np.int32);
mat = np.array(np.random.rand(3,3),dtype=np.int32);

colorConv(cin,cout,mat);

