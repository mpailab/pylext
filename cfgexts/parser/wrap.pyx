
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdlib cimport malloc, free
from cpython.long cimport PyLong_FromVoidPtr as from_cptr
from cpython.long cimport PyLong_AsVoidPtr as to_cptr

from wrap cimport *

cdef inline to_bytes (text):
    return text.encode('utf-8')

cdef inline to_str (text):
    return text.decode('utf-8')

def apply (text):
    return to_str(c_apply(to_bytes(text)))

def loadFile (filename):
    return c_loadFile(to_bytes(filename))

def pass_arg(int x):
    return c_pass_arg(x)

def pass_arg_except(int x):
    return c_pass_arg_except(x)

def pass_arg_cython(int x):
    return x

def c_quasiquote (px, nt, n, data, pn):
    cdef vector[string] _data = [to_bytes(x) for x in data]
    cdef vector[ParseNode*] subtrees
    for i in range(len(pn)):
        subtrees.push_back(<ParseNode*> to_cptr(pn[i]))
    return from_cptr(c_c_quasiquote(to_cptr(px), to_bytes(nt), _data, subtrees))

def new_python_context (by_stmt, syntax_file):
    return from_cptr(c_new_python_context(by_stmt, to_bytes(syntax_file)))

def del_python_context (x):
    c_del_python_context(to_cptr(x))

def inc_pn_num_refs (pn):
    c_inc_pn_num_refs(to_cptr(pn))

def dec_pn_num_refs (pn):
    c_dec_pn_num_refs(to_cptr(pn))

def get_pn_num_children (pn):
    return c_get_pn_num_children(to_cptr(pn))

def get_pn_child (pn, i):
    return from_cptr(c_get_pn_child(to_cptr(pn), i))

def set_pn_child (pn, i, ch):
    return c_set_pn_child(to_cptr(pn), i, to_cptr(ch))

def get_pn_rule (pn):
    return c_get_pn_rule(to_cptr(pn))

def pn_equal (pn1, pn2):
    return c_pn_equal(to_cptr(pn1), to_cptr(pn2))

def add_rule (px, lhs, rhs):
    return c_add_rule(to_cptr(px), to_bytes(lhs), to_bytes(rhs))

def new_parser_state (px, text, start):
    return from_cptr(c_new_parser_state(to_cptr(px), to_bytes(text), to_bytes(start)))

def continue_parse (state):
    return from_cptr(c_continue_parse(to_cptr(state)))

def del_parser_state (state):
    c_del_parser_state(to_cptr(state))

def ast_to_text (pcontext, pn):
    return to_str(c_ast_to_text(to_cptr(pcontext), to_cptr(pn)))
