
from libcpp.string cimport string

cimport _wrap

def c_apply (text):
    cdef string s = text
    s = _wrap.apply(s)
    return s