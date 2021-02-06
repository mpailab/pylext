
from libcpp.string cimport string
from cpython.ref cimport PyObject

cimport wrap

def apply (text):
    cdef string s = text
    return wrap.apply(s)

def loadFile (filename):
    cdef string _filename = filename
    return wrap.loadFile(_filename)

def pass_arg(int x):
    return wrap.pass_arg(x)

def pass_arg_except(int x):
    return wrap.pass_arg_except(x)

def pass_arg_cython(int x):
    return x

def quasiquote (px, nt, n, data, pn):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _nt = nt
    cdef int _n = n
    cdef PyObject* _data = <PyObject *> data
    cdef PyObject* _pn = <PyObject *> pn
    return <object> wrap.c_quasiquote(_px, _nt, _n, _data, _pn)

def new_python_context (by_stmt, syntax_file):
    cdef int _by_stmt = by_stmt
    cdef string _syntax_file = syntax_file
    return <object> wrap.new_python_context(_by_stmt, syntax_file)

def del_python_context (x):
    cdef PyObject* _x = <PyObject *> x
    wrap.del_python_context(_x)

def inc_pn_num_refs (pn):
    cdef PyObject* _pn = <PyObject *> pn
    wrap.inc_pn_num_refs(_pn)

def dec_pn_num_refs (pn):
    cdef PyObject* _pn = <PyObject *> pn
    wrap.dec_pn_num_refs(_pn)

def get_pn_num_children (pn):
    cdef PyObject* _pn = <PyObject *> pn
    return wrap.get_pn_num_children(_pn)

def get_pn_child (pn, i):
    cdef PyObject* _pn = <PyObject *> pn
    cdef int _i = i
    return <object> wrap.get_pn_child(_pn, _i)

def set_pn_child (pn, i, ch):
    cdef PyObject* _pn = <PyObject *> pn
    cdef int _i = i
    cdef PyObject* _ch = <PyObject *> ch
    return wrap.set_pn_child(_pn, _i, _ch)

def get_pn_rule (pn):
    cdef PyObject* _pn = <PyObject *> pn
    return wrap.get_pn_rule(_pn)

def pn_equal (pn1, pn2):
    cdef PyObject* _pn1 = <PyObject *> pn1
    cdef PyObject* _pn2 = <PyObject *> pn2
    return wrap.pn_equal(_pn1, _pn2)

def add_rule (px, lhs, rhs):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _lhs = lhs
    cdef char* _rhs = rhs
    return wrap.add_rule(_px, _lhs, _rhs)

def new_parser_state (px, text, start):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _text = text
    cdef char* _start = start
    return <object> wrap.new_parser_state(_px, _text, _start)

def continue_parse (state):
    cdef PyObject* _state = <PyObject *> state
    return <object> wrap.continue_parse(_state)

def del_parser_state (state):
    cdef PyObject* _state = <PyObject *> state
    wrap.del_parser_state(_state)