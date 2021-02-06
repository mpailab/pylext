
from libcpp.string cimport string
from cpython.ref cimport PyObject

cimport wrap

cdef inline to_bytes (text):
    return text.encode('utf-8')

cdef inline to_str (text):
    return text.decode('utf-8')

def apply (text):
    return to_str(wrap.c_apply(to_bytes(text)))

def loadFile (filename):
    return wrap.c_loadFile(to_bytes(filename))

def pass_arg(int x):
    return wrap.c_pass_arg(x)

def pass_arg_except(int x):
    return wrap.c_pass_arg_except(x)

def pass_arg_cython(int x):
    return x

def quasiquote (px, nt, n, data, pn):
    return <object> wrap.c_quasiquote(<PyObject*> px, to_bytes(nt), n, <PyObject*> data, <PyObject*> pn)

def new_python_context (by_stmt, syntax_file):
    return <object> wrap.c_new_python_context(by_stmt, to_bytes(syntax_file))

def del_python_context (x):
    wrap.c_del_python_context(<PyObject*> x)

def inc_pn_num_refs (pn):
    wrap.c_inc_pn_num_refs(<PyObject*> pn)

def dec_pn_num_refs (pn):
    wrap.c_dec_pn_num_refs(<PyObject*> pn)

def get_pn_num_children (pn):
    return wrap.c_get_pn_num_children(<PyObject*> pn)

def get_pn_child (pn, i):
    return <object> wrap.c_get_pn_child(<PyObject*> pn, i)

def set_pn_child (pn, i, ch):
    return wrap.c_set_pn_child(<PyObject*> pn, i, <PyObject*> ch)

def get_pn_rule (pn):
    return wrap.c_get_pn_rule(<PyObject*> pn)

def pn_equal (pn1, pn2):
    return wrap.c_pn_equal(<PyObject*> pn1, <PyObject*> pn2)

def add_rule (px, lhs, rhs):
    return wrap.c_add_rule(<PyObject*> px, to_bytes(lhs), to_bytes(rhs))

def new_parser_state (px, text, start):
    return <object> wrap.c_new_parser_state(<PyObject*> px, to_bytes(text), to_bytes(start))

def continue_parse (state):
    return <object> wrap.c_continue_parse(<PyObject*> state)

def del_parser_state (state):
    wrap.c_del_parser_state(<PyObject*> state)