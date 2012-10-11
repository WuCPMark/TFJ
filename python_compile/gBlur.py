from VecFunction import *
from decorators import *
import numpy as np
import math

def mk_kernel(K):
    s = 0.84089642;
    c0 = 1.0 / math.sqrt(2.0*math.pi*s*s);
    c1 = -1.0 / (2.0*s*s);
    for y in range(0,7):
        ym = y - 3;
        for x in range(0,7):
            xm = x -3;
            K[y][x] = c0 * math.exp((xm*xm + ym*ym)*c1);

@vec
def myConv2D(Out,In,Kernel):
    for y in range(3,1021):
        for x in range(3,1021):
            for yy in range(-3,4):
                for xx in range(-3,4):
                    Out[y][x] = Kernel[3+yy][3+xx]*In[y+yy][x+xx];

def slowConv2D(Out,In,Kernel):
    for y in range(3,1021):
        for x in range(3,1021):
            for yy in range(-3,4):
                for xx in range(-3,4):
                    Out[y][x] = Kernel[3+yy][3+xx]*In[y+yy][x+xx];

K = np.array(np.random.rand(7,7), dtype=np.float32);
In = np.array(np.random.rand(1024,1024), dtype=np.float32);

Out0 = np.array(np.random.rand(1024,1024), dtype=np.float32);
Out1 = np.array(np.random.rand(1024,1024), dtype=np.float32);

Out1 = Out0;

mk_kernel(K)
myConv2D(Out0,In,K);
print 'done with TFJ'
#slowConv2D(Out1,In,K);

#for y in range(0,1024):
#    for x in range(0,1024):
#        d = Out1[y][x] - Out0[y][x];
#        if(d*d > 0.001):
#            print str(x) + str(y);
