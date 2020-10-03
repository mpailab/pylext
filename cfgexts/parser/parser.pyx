
from libcpp.string cimport string

cimport parser

def c_apply (text):
    cdef string s = text
    s = parser.apply(s)
    return s