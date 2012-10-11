from VecFunction import *
from decorators import *
import numpy as np
import time

@vec
def heat1d(u):
    for t in range(0,1023):
        for x in range(0,1023):
            u[t+1][x] = u[t][x] + 0.25*(u[t][x+1] - 2.0*u[t][x] + u[t][x-1]);

def py_heat1d(u):
    for t in range(0,1023):
        for x in range(0,1023):
            u[t+1][x] = u[t][x] + 0.25*(u[t][x+1] - 2.0*u[t][x] + u[t][x-1]);

u0 = 2048.0 * np.array(np.random.rand(1024,1024), dtype=np.float32);
u1 = u0;

tfj_rt = [];
py_rt = [];

print 'getting ready to run heat1d'
for i in range(0,8):
    start = time.time();
    heat1d(u0);
    elapsed = (time.time() - start);
    tfj_rt.append(elapsed);
    print elapsed

print 'heat1d done'
for i in range(0,8):
    start = time.time();
    py_heat1d(u1);
    elapsed = (time.time() - start);
    py_rt.append(elapsed);
    print elapsed

for i in range(0,8):
    print py_rt[i] / tfj_rt[i];

for i in range(0,1024):
    for j in range(0,1024):
        d = u0[i][j] - u1[i][j];
        if( d*d > 0.01):
            print 'wrong ' + str(i) + ' ' + str(j) + ' ' + str(d)
            quit();
