from VecFunction import *
from decorators import *
import numpy as np

@vec
def small_decoder(E):
    for t in range(1,1024):
        for s in range(1,1024):
            E[t][s] = E[t-1][s]+E[t-1][s-1]
E = np.array(np.random.rand(1024,1024), dtype=np.float32);
small_decoder(E);
