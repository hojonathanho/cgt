import numpy as np
import cgt
from cgt import nn, core
from . import reset_config

@reset_config
def test_flatvec():
    cgt.set_precision('double')
    cgt.core.modify_config(backend="python") # XXX

    N = 10
    K = 3

    Xval = np.random.randn(N,K)
    wval = np.random.randn(K)
    bval = np.random.randn()
    yval = np.random.randn(N)

    X_nk = cgt.shared(Xval, "X")
    y_n = cgt.shared(yval, "y")
    w_k = cgt.shared(wval, "w")
    b = cgt.shared(bval, name="b")

    ypred = cgt.dot(X_nk, w_k) + b

    err = cgt.sum(cgt.square(ypred - y_n))
    g = cgt.grad(err, [w_k, b])
    g = core.simplify(g)

    pars = [w_k, b]
    flatx = nn.setup_contiguous_storage(pars)
    f = cgt.function([], [err,cgt.flatcat(g)])