from numpy import asarray, diag
import numpy as np
import math
from scipy.linalg import svd

def svd2(m):

    y1, x1 = (m[1, 0] + m[0, 1]), (m[0, 0] - m[1, 1])
    y2, x2 = (m[1, 0] - m[0, 1]), (m[0, 0] + m[1, 1])

    h1 = math.sqrt(y1*y1 + x1*x1)
    h2 = math.sqrt(y2*y2 + x2*x2)

    t1 = x1 / h1
    t2 = x2 / h2

    cc = math.sqrt((1 + t1) * (1 + t2))
    ss = math.sqrt((1 - t1) * (1 - t2))
    cs = math.sqrt((1 + t1) * (1 - t2))
    sc = math.sqrt((1 - t1) * (1 + t2))

    c1, s1 = (cc - ss) / 2, (sc + cs) / 2,
    u1 = asarray([[c1, -s1], [s1, c1]])

    d = asarray([(h1 + h2) / 2, (h1 - h2) / 2])
    sigma = diag(d)

    if h1 != h2:
        u2 = diag(1 / d).dot(u1.T).dot(m)
    else:
        u2 = diag([1 / d[0], 0]).dot(u1.T).dot(m)

    return u1, sigma, u2

# m = np.array([[-1,7],[5,5]])
# m = np.array([[1,2],[3,4]])
m = np.array([[2125.000000,1312.500000],[1312.500000,1093.750000]])



# u, sigma, v = svd2(m)
u, sigma, v = svd2(m)
print("U")
print(u)
print("Sigma")
print(sigma)
print("v")
print(v)