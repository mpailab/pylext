
from libcpp.string cimport string
from cpython.ref cimport PyObject

cimport parser

def c_apply (text):
    cdef string s = text
    return parser.apply(s)

def c_loadFile (filename):
    cdef string _filename = filename
    return parser.loadFile(_filename)

def c_quasiquote (px, nt, n, data, pn):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _nt = nt
    cdef int _n = n
    cdef PyObject* _data = <PyObject *> data
    cdef PyObject* _pn = <PyObject *> pn
    return <object> parser.c_quasiquote(_px, _nt, _n, _data, _pn)

def c_new_python_context (by_stmt, syntax_file):
    cdef int _by_stmt = by_stmt
    cdef string _syntax_file = syntax_file
    return <object> parser.new_python_context(_by_stmt, syntax_file)

def c_del_python_context (x):
    cdef PyObject* _x = <PyObject *> x
    parser.del_python_context(_x)

def c_inc_pn_num_refs (pn):
    cdef PyObject* _pn = <PyObject *> pn
    parser.inc_pn_num_refs(_pn)

def c_dec_pn_num_refs (pn):
    cdef PyObject* _pn = <PyObject *> pn
    parser.dec_pn_num_refs(_pn)

def c_get_pn_num_children (pn):
    cdef PyObject* _pn = <PyObject *> pn
    return parser.get_pn_num_children(_pn)

def c_get_pn_child (pn, i):
    cdef PyObject* _pn = <PyObject *> pn
    cdef int _i = i
    return <object> parser.get_pn_child(_pn, _i)

def c_set_pn_child (pn, i, ch):
    cdef PyObject* _pn = <PyObject *> pn
    cdef int _i = i
    cdef PyObject* _ch = <PyObject *> ch
    return parser.set_pn_child(_pn, _i, _ch)

def c_get_pn_rule (pn):
    cdef PyObject* _pn = <PyObject *> pn
    return parser.get_pn_rule(_pn)

def c_pn_equal (pn1, pn2):
    cdef PyObject* _pn1 = <PyObject *> pn1
    cdef PyObject* _pn2 = <PyObject *> pn2
    return parser.pn_equal(_pn1, _pn2)

def c_add_rule (px, lhs, rhs):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _lhs = lhs
    cdef char* _rhs = rhs
    return parser.add_rule(_px, _lhs, _rhs)

def c_new_parser_state (px, text, start):
    cdef PyObject* _px = <PyObject *> px
    cdef char* _text = text
    cdef char* _start = start
    return <object> parser.new_parser_state(_px, _text, _start)

def c_continue_parse (state):
    cdef PyObject* _state = <PyObject *> state
    return <object> parser.continue_parse(_state)

def c_del_parser_state (state):
    cdef PyObject* _state = <PyObject *> state
    parser.del_parser_state(_state)