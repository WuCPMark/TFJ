from VecFunction import *
from decorators import *
import numpy as np
import time

def sel(s, x, y):
    if(s):
        return x;
    else:
        return y;

@vec
def nearest_cluster(dist, min_dist, idx, cluster_idx, c_x, c_y, p_x, p_y):
    for i in range(0, 1024):
        for j in range(0, 8):
            dist[i][j] = (c_x[j] - p_x[i])*(c_x[j] - p_x[i]) + \
                (c_y[j] - p_y[i])*(c_y[j] - p_y[i])
            cluster_idx[i] = sel(dist[i][j] < min_dist[i], idx[j], cluster_idx[i])
            min_dist[i] = sel(dist[i][j] < min_dist[i], dist[i][j], min_dist[i])
 
def py_nearest_cluster(dist, min_dist, idx, cluster_idx, c_x, c_y, p_x, p_y):
    for i in range(0, 1024):
        for j in range(0, 8):
            dist[i][j] = (c_x[j] - p_x[i])*(c_x[j] - p_x[i]) + \
                (c_y[j] - p_y[i])*(c_y[j] - p_y[i])
            cluster_idx[i] = sel(dist[i][j] < min_dist[i], idx[j], cluster_idx[i])
            min_dist[i] = sel(dist[i][j] < min_dist[i], dist[i][j], min_dist[i])
       
        



dist = np.array(np.random.rand(1024,8), dtype=np.float32);
min_dist = np.array(np.random.rand(1024), dtype=np.float32);
for i in range(0,1024):
    min_dist[i] = 1000000.0;

cluster_idx = np.array(np.random.rand(1024), dtype=np.int32);

idx = np.array(np.random.rand(8), dtype=np.int32);

for i in range(0,8):
    idx[i] = i;

c_x = np.array(np.random.rand(8), dtype=np.float32);
c_y = np.array(np.random.rand(8), dtype=np.float32);
p_x = np.array(np.random.rand(1024), dtype=np.float32);
p_y = np.array(np.random.rand(1024), dtype=np.float32);

print 'getting ready to run nearest_cluster'

opt_rt = [];
py_rt = [];

for i in range(0,16):
    start = time.time();
    nearest_cluster(dist, min_dist, idx, cluster_idx, c_x, c_y, p_x, p_y);
    elapsed = (time.time() - start);
    opt_rt.append(elapsed);
    print 'run = %f' % elapsed

for i in range(0,16):
    start = time.time();
    py_nearest_cluster(dist, min_dist, idx, cluster_idx, c_x, c_y, p_x, p_y);
    elapsed = (time.time() - start);
    py_rt.append(elapsed);
    print 'run = %f' % elapsed

for i in range(0,16):
    print py_rt[i] / opt_rt[i]
