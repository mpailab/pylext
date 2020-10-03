
from libcpp.string cimport string

cdef extern from "apply.h":
    string apply (string text)