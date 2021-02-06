import time
import cfgexts.parser.wrap as parser 

def py_pass_arg(x):
    return x

if __name__ == "__main__":

    start = time.time()
    for i in range(1000000):
        py_pass_arg(0)
    end = time.time()
    print("py_pass_arg:", end - start)

    start = time.time()
    for i in range(1000000):
        parser.pass_arg_cython(0)
    end = time.time()
    print("cython_pass_arg:", end - start)

    start = time.time()
    for i in range(1000000):
        parser.pass_arg(0)
    end = time.time()
    print("c_pass_arg:", end - start)

    start = time.time()
    for i in range(1000000):
        parser.pass_arg_except(0)
    end = time.time()
    print("c_pass_arg_except:", end - start)