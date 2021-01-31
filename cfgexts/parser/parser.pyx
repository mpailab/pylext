
from libcpp.string cimport string

cimport parser

def c_apply (text):
    cdef string s = text
    s = parser.apply(s)
    return s

def c_quasiquote (px, nt, n, data, pn):
    cdef void* _px = <void *> px
    cdef char* _nt = nt
    cdef int _n = n
    cdef void* _data = <void *> data
    cdef void* _pn = <void *> pn
    res = <object> parser.c_quasiquote(_px, _nt, _n, _data, _pn)
    return res

def c_new_python_context (by_stmt):
    cdef int _by_stmt = by_stmt
    res = <object> parser.new_python_context(_by_stmt)
    return res

def c_del_python_context (x):
    cdef void* _x = <void *> x
    res = parser.del_python_context(_x)
    return res

def c_inc_pn_num_refs (pn):
    cdef void* _pn = <void *> pn
    res = parser.inc_pn_num_refs(_pn)
    return res

def c_dec_pn_num_refs (pn):
    cdef void* _pn = <void *> pn
    res = parser.dec_pn_num_refs(_pn)
    return res

def c_get_pn_num_children (pn):
    cdef void* _pn = <void *> pn
    res = parser.get_pn_num_children(_pn)
    return res

def c_get_pn_child (pn, i):
    cdef void* _pn = <void *> pn
    cdef int _i = i
    res = <object> parser.get_pn_child(_pn, _i)
    return res

def c_set_pn_child (pn, i, ch):
    cdef void* _pn = <void *> pn
    cdef int _i = i
    cdef void* _ch = <void *> ch
    res = parser.set_pn_child(_pn, _i, _ch)
    return res

def c_get_pn_rule (pn):
    cdef void* _pn = <void *> pn
    res = parser.get_pn_rule(_pn)
    return res

def c_pn_equal (pn1, pn2):
    cdef void* _pn1 = <void *> pn1
    cdef void* _pn2 = <void *> pn2
    res = parser.pn_equal(_pn1, _pn2)
    return res

def c_add_rule (px, lhs, rhs):
    cdef void* _px = <void *> px
    cdef char* _lhs = lhs
    cdef char* _rhs = rhs
    res = parser.add_rule(_px, _lhs, _rhs)
    return res

def c_new_parser_state (px, text, start):
    cdef void* _px = <void *> px
    cdef char* _text = text
    cdef char* _start = start
    res = <object> parser.new_parser_state(_px, _text, _start)
    return res

def c_continue_parse (state):
    cdef void* _state = <void *> state
    res = <object> parser.continue_parse(_state)
    return res

def c_del_parser_state (state):
    cdef void* _state = <void *> state
    res = parser.del_parser_state(_state)
    return res
