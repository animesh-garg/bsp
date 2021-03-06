import numpy as np
from numpy import matlib as ml
import math
import time

import matplotlib
import matplotlib.pyplot as plt
from scipy.interpolate import spline
import scipy.interpolate as interp


import IPython


def plot_state_and_particles(x, particles, target, xDim, M, N):
    plt.clf()
    plt.cla()
    
    plt.axis([-1, 6, -1, 6])
    
    P_t = np.matrix(particles)
    P_t = np.reshape(P_t, (xDim,M))
    
    X = np.matrix(x)
    X = np.reshape(X, (N*xDim, len(x)/(N*xDim)))
    
    colors = plt.cm.RdYlGn(np.linspace(0, 1, N))
    for n in xrange(N):
        plt.plot(X[n*xDim,:],X[n*xDim+1,:], color=colors[n], marker='o', markersize=10, mew=2)
        
    plt.plot(P_t[0,:], P_t[1,:], color='blue', marker='x', markersize=5, mew=1)
    plt.plot(target[0], target[1], color='green', marker='^', markersize=10, mew=2.5)
        
    plt.show(block=False)
    plt.pause(.05)
    