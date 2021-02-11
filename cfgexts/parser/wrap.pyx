
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
    cdef vector[string] parts = [to_bytes(x) for x in data]
    cdef vector[ParseNode*] subtrees
    for i in range(len(pn)):
        subtrees.push_back(<ParseNode*> to_cptr(pn[i]))
    return from_cptr(quasiquote(<ParseContext*> to_cptr(px), to_bytes(nt), parts, subtrees))

def new_python_context (by_stmt, syntax_file):
    cdef PythonParseContext* px = new PythonParseContext()
    init_python_grammar(px, by_stmt != 0, to_bytes(syntax_file))
    return from_cptr(px)

def del_python_context (px):
    cdef ParseContext* c_px = <ParseContext*> to_cptr(px)
    del c_px

def inc_pn_num_refs (pn):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)
    if c_pn is not NULL:
        c_pn.refs += 1

def dec_pn_num_refs (pn):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)
    if c_pn is not NULL:
        c_pn.refs -= 1

def get_pn_num_children (pn):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)
    return 0 if c_pn is NULL else c_pn.ch.size()

def get_pn_child (pn, i):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)

    if i < 0 or i >= c_pn.ch.size():
        raise ValueError(f"Parse node child index {i} out of range ({c_pn.ch.size()})")

    return from_cptr(c_pn.ch[i])

def set_pn_child (pn, i, ch):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)
    cdef ParseNode* c_ch = <ParseNode*> to_cptr(ch)

    if c_ch is NULL:
        raise ValueError("Cannot set null parse node as child")

    if i < 0 or i >= c_pn.ch.size():
        raise ValueError(f"Parse node child index {i} out of range ({c_pn.ch.size()})")

    c_pn.ch[i] = c_ch

def get_pn_rule (pn):
    cdef ParseNode* c_pn = <ParseNode*> to_cptr(pn)
    return c_pn.rule

def pn_equal (pn1, pn2):
    return equal_subtrees(<ParseNode*> to_cptr(pn1), <ParseNode*> to_cptr(pn2))

def add_rule (px, lhs, rhs):
    cdef ParseContext* c_px = <ParseContext*> to_cptr(px)
    return addRule(c_px.grammar(), to_bytes(lhs + " -> " + rhs))

def new_parser_state (px, text, start):
    cdef ParserState* _state = new ParserState(<ParseContext*> to_cptr(px), to_bytes(text), to_bytes(start))
    return from_cptr(_state)

def continue_parse (state):
    cdef ParserState* c_state = <ParserState*> to_cptr(state)
    cdef ParseTree tree = c_state.parse_next()
    cdef ParseNode* root = tree.root.get()
    return from_cptr(root)

def del_parser_state (state):
    cdef ParserState* c_state = <ParserState*> to_cptr(state)
    del c_state

def ast_to_text (px, pn):
    return to_str(c_ast_to_text(<ParseContext*> to_cptr(px), <ParseNode*> to_cptr(pn)))
