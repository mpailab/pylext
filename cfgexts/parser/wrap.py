# mypy error: Module 'cfgexts.parser' has no attribute 'parser'
from . import parser # type: ignore

def to_c_string (str):
    return str.encode('utf-8')

def from_c_string (str):
    return str.decode('utf-8')

def apply (text):
    c_text = to_c_string(text)
    c_res = parser.c_apply(c_text)
    return from_c_string(c_res)

def loadFile (filename):
    parser.c_loadFile(to_c_string(filename))

def quasiquote (px, nt, n, data, pn):
    return parser.c_quasiquote(px, nt, n, data, pn)

def new_python_context (by_stmt, syntax_file):
    return parser.c_new_python_context(by_stmt, to_c_string(syntax_file))

def del_python_context (x):
    return parser.c_del_python_context(x)

def inc_pn_num_refs (pn):
    return parser.c_inc_pn_num_refs(pn)

def dec_pn_num_refs (pn):
    return parser.c_dec_pn_num_refs(pn)

def get_pn_num_children (pn):
    return parser.c_get_pn_num_children(pn)

def get_pn_child (pn, i):
    return parser.c_get_pn_child(pn, i)

def set_pn_child (pn, i, ch):
    return parser.c_set_pn_child(pn, i, ch)

def get_pn_rule (pn):
    return parser.c_get_pn_rule(pn)

def pn_equal (pn1, pn2):
    return parser.c_pn_equal(pn1, pn2)

def add_rule (px, lhs, rhs):
    return parser.c_add_rule(px, lhs, rhs)

def new_parser_state (px, text, start):
    return parser.c_new_parser_state(px, to_c_string(text), to_c_string(start))

def continue_parse (state):
    return parser.c_continue_parse(state)

def del_parser_state (state):
    parser.c_del_parser_state(state)
