from vecfunction import *
from decorators import *

@vec
def spvv_csr(x, cols, y):
    """
    Multiply a sparse row vector x -- whose non-zero values are in the
    specified columns -- with a dense column vector y.
    """
    z = gather(y, cols)
    return sum(map(lambda a, b: a * b, x, z))

@vec
def spmv_csr(Ax, Aj, x):
    """
    Compute y = Ax for CSR matrix A and dense vector x.
 
    Ax and Aj are nested sequences where Ax[i] are the non-zero entries
    for row i and Aj[i] are the corresponding column indices.
    """
    return map(lambda y, cols: spvv_csr(y, cols, x), Ax, Aj)

@vec
def spmv_ell(data, idx, x):
    def kernel(i):
        return sum(map(lambda Aj, J: Aj[i] * x[J[i]], data, idx))
    return map(kernel, indices(x))


