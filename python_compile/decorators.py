"""
Decorators for Copperhead procedure declarations
"""

def vec(fn):
    """
    Decorator for declaring that a procedure is visible in Copperhead.

    For example:
        @cu
        def plus1(x):
            return [xi + 1 for xi in x]
    """
    from VecFunction import *

    # Wrap Python procedure in CuFunction object that will intercept
    # calls (e.g., for JIT compilation).
    vecfn = VecFunction(fn)

    return vecfn

def vec_check(fn):
    """
    Decorator for declaring that a procedure is visible in Copperhead.

    For example:
        @cu
        def plus1(x):
            return [xi + 1 for xi in x]
    """
    from VecCheckFunction import *

    # Wrap Python procedure in CuFunction object that will intercept
    # calls (e.g., for JIT compilation).
    vecfn = VecCheckFunction(fn)

    return vecfn

