from VecFunction import *
from decorators import *
from numpy import *

@vec
def test_ternary(scale):
    for i in range(0,127):
        scale[i] = scale[i]+1 if (1) else scale[i]+2;
            
#A = random.rand(128,128);
print test_ternary.get_ast()
scale = random.rand(128);
test_ternary(scale);
print scale[0]
test_ternary.python_function()(scale[0])
print scale[0]
