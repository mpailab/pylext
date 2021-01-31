# mypy error: Module 'cfgexts.parser' has no attribute 'parser'
from . import parser # type: ignore

def apply (text):
    return parser.c_apply(text.encode('utf-8')).decode('utf-8')

# def quasiquote (px, nt, n, data, pn):
#     return parser.c_quasiquote(px, nt, n, data, pn)

# def new_python_context (by_stmt):
#     return parser.c_new_python_context(by_stmt)

# def del_python_context (x):
#     return parser.c_del_python_context(x)

# def inc_pn_num_refs (pn):
#     return parser.c_inc_pn_num_refs(pn)

# def dec_pn_num_refs (pn):
#     return parser.c_dec_pn_num_refs(pn)

# def get_pn_num_children (pn):
#     return parser.c_get_pn_num_children(pn)

# def get_pn_child (pn, i):
#     return parser.c_get_pn_child(pn, i)

# def set_pn_child (pn, i, ch):
#     return parser.c_set_pn_child(pn, i, ch)

# def get_pn_rule (pn):
#     return parser.c_get_pn_rule(pn)

# def pn_equal (pn1, pn2):
#     return parser.c_pn_equal(pn1, pn2)

# def add_rule (px, lhs, rhs):
#     return parser.c_add_rule(px, lhs, rhs)

def new_parser_state (px, text, start):
    return parser.c_new_parser_state(px, text, start)

# def continue_parse (state):
#     return parser.c_continue_parse(state)

# def del_parser_state (state):
#     return parser.c_del_parser_state(state)
