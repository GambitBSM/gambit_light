# To import gambit_light_interface, first append the directory containing 
# gambit_light_interface.so to sys.path. (Alternatively, add this directory 
# to the PYTHONPATH environment variable.)
import sys
import os
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(current_dir, "../lib"))
import gambit_light_interface as gambit_light


import numpy as np

def rosenbrock(x, y):
    a = 1.
    b = 100.
    return (a - x)**2 + b * (y - x**2)**2

def rosenbrock_general(x):
    n = len(x)
    return sum(rosenbrock(x[i], x[i+1]) for i in range(n - 1))

def loglike(x):
    return -0.5 * rosenbrock_general(x)


# User-side log-likelihood function, which can be called by GAMBIT-light.
def user_loglike(input_names, input_vals, output):

    # Make a dictionary of the inputs?
    input = {input_names[i]: input_vals[i] for i in range(len(input_names))}

    x = np.array([input["x1"], input["x2"]])

    # Store some more output?
    # output["my_output_1"] = 10;

    return loglike(x)
