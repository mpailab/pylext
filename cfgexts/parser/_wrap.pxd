
from libcpp.string cimport string

cdef extern from "src/apply.h":
    string apply (string text)