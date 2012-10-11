import Image
import ImageFilter

import numpy as np
import math
from VecFunction import *
from decorators import *
import time

def mk_kernel(K):
    s = 0.84089642;
    c0 = 1.0 / math.sqrt(2.0*math.pi*s*s);
    c1 = -1.0 / (2.0*s*s);
    for y in range(0,5):
        ym = y - 2;
        for x in range(0,5):
            xm = x - 2;
            K[y][x] = c0 * math.exp((xm*xm + ym*ym)*c1);

@vec
def py_conv2d(I,O,K):
    for y in range(3,390):
        for x in range(3,590):
            for yy in range(-2,3):
                for xx in range(-2, 3):
                    O[y][x] = O[y][x] + K[2+yy][2+xx] * I[y+yy][x+xx];
global ext
ext = ".jpg"

imageFile = "test.jpg"
im1 = Image.open(imageFile);
width,height = im1.size
im1 = im1.convert('L');

print 'width=%d' % width
print 'height=%d' % height

I = np.asarray(im1,dtype=np.float32);



K = np.ones((5,5), dtype=np.float32);
mk_kernel(K);
py_rt = [];
opt_rt = [];

opt_rt = [];
py_rt = [];

for i in range(0,7):
    O = np.zeros((height,width),dtype=np.float32);
    start = time.time();
    py_conv2d(I,O,K);
    elapsed = (time.time() - start);
    opt_rt.append(elapsed);
    print 'run = %f' % elapsed

im2 = Image.fromarray(O);
im2.show()
KK = np.reshape(K,25);
iK = ImageFilter.Kernel((5,5),KK,1);


print '\n\nPython library:'
for i in range(0,7):
    start = time.time();
    im3 = im1.filter(iK);
    elapsed = (time.time() - start);
    py_rt.append(elapsed);
    print 'run = %f' % elapsed

#im3.show()

for i in range(0,7):
    print py_rt[i] / opt_rt[i];
