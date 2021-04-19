# cython: c_string_type=unicode, c_string_encoding=utf8

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map as cpp_map
from libc.stdlib cimport malloc, free
from cpython.long cimport PyLong_FromVoidPtr as from_cptr
from cpython.long cimport PyLong_AsVoidPtr as to_cptr

from .wrap cimport *
from cython.operator cimport dereference as deref

from .python_grammar import python_grammar_str
cdef string python_grammar = python_grammar_str

cdef inline to_bytes (text):
    return text.encode('utf-8')

cdef inline to_str (text):
    return text.decode('utf-8')

__parse_context__ = None
def parse_context():
    global __parse_context__
    return __parse_context__

cdef class ParseNode:
    cdef CParseNode* p

    def __init__(self):
        self.p = NULL
        if self.p:
            self.p.refs += 1

    cdef ParseNode init(self, CParseNode* n):
        if self.p:
            self.p.refs -= 1
        self.p = n
        if n:
            n.refs += 1
        return self

    def __del__(self):
        if self.p:
            self.p.refs -= 1

    cdef CParseNode* get(self):
        return self.p

    cpdef int num_children(self):
        return self.p.ch.size()

    cpdef ParseNode getitem(self, int i):
        if i < 0 or i >= self.p.ch.size():
            raise ValueError(f"Parse node child index {i} out of range ({self.p.ch.size()})")
        return ParseNode().init(self.p.ch[i])

    cpdef void setitem(self, int i, ParseNode value):
        if i < 0 or i >= self.p.ch.size():
            raise ValueError(f"Parse node child index {i} out of range ({self.p.ch.size()})")
        self.p.ch[i] = value.p

    def __getitem__(self, i):
        return self.getitem(i)

    def __setitem__(self, i, val):
        self.setitem(i, val)

    def children(self):
        return [ParseNode().init(x) for x in self.p.ch]

    cpdef int rule(self):
        return self.p.rule

    cpdef string str(self):
        assert self.p.isTerminal()
        return self.p.term

    cpdef bool is_terminal(self):
        return self.p.isTerminal()

    cpdef int ntnum(self):
        return self.p.nt

    cdef bool equal(self, ParseNode other):
        return equal_subtrees(self.p, other.p) != 0

    def __eq__(self, other):
        return self.equal(other)
        #assert type(other) is ParseNode
        #return equal_subtrees(self.p, other.p) != 0

    def __repr__(self):
        return 'ParseNode:\n'+parse_context().ast_to_text(self)


cdef class ParseContext:
    cdef PythonParseContext* px
    cdef dict syntax_rules
    cdef dict macro_rules
    cdef dict token_rules
    # cdef dict lexer_rules
    cdef list exported_syntax
    cdef list exported_macro
    cdef list exported_tokens
    cdef list exported_lexer_rules
    cdef dict global_vars

    def __init__(self, global_vars):
        self.px = create_python_context(True, python_grammar)
        self.syntax_rules = {}
        self.macro_rules = {}
        self.token_rules = {}

        self.exported_syntax = []
        self.exported_macro = []
        self.exported_tokens = []
        self.exported_lexer_rules = []

        self.global_vars=global_vars or {}

    def eval(self, expr):
        if type(expr) is ParseNode:
            expr = self.ast_to_text(expr)
        return eval(expr, self.global_vars)

    def syntax_function(self, int rule):
        return self.syntax_rules.get(rule, None)

    def macro_function(self, int rule):
        return self.macro_rules.get(rule, None)

    def token_function(self, int rule):
        return self.token_rules.get(rule, None)

    def __enter__(self):
        global __parse_context__
        if __parse_context__ is not None:
            raise Exception('Enter parse context when previous context not deleted')
        __parse_context__ = self
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        global __parse_context__
        __parse_context__ = None

    def __del__(self):
        del_python_context(self.px)

    cpdef add_token(self, string name, string rhs, object apply=None, bool for_export=False):
        cdef int rule_id = add_token(self.px, name, rhs)
        self.token_rules[rule_id] = apply
        if for_export:
            self.exported_tokens.append((name, rhs, apply))

    cpdef add_lexer_rule(self, string lhs, string rhs, bool for_export=False):
        cdef int rule_id = add_lexer_rule(self.px, lhs, rhs)
        if for_export:
            self.exported_lexer_rules.append((lhs, rhs))

    def add_macro_rule(self, lhs: str, rhs: list, apply, for_export=False,
                         int lpriority=-1, int rpriority=-1):
        cdef string rhss = ' '.join(str(x) for x in rhs)
        # print(f'add rule: {lhs} -> {rhs}:',end='')
        cdef int rule_id = self._add_rule(lhs, rhss, lpriority, rpriority)
        # print(f' id = {rule_id}, f = {apply}')
        # print(f'add macro rule {rule_id}')
        self.macro_rules[rule_id] = apply
        if for_export:
            self.exported_macro.append((lhs, rhss, apply, lpriority, rpriority))

    def add_syntax_rule(self, lhs: str, rhs, apply, for_export=False, lpriority=-1, rpriority=-1):
        cdef string rhss = ' '.join(str(x) for x in rhs)
        cdef int rule_id = self._add_rule(lhs, rhss, lpriority, rpriority)
        self.syntax_rules[rule_id] = apply
        if for_export:
            self.exported_syntax.append((lhs, rhss, apply, lpriority, rpriority))

    def gen_syntax_import(self):
        res = "def _import_grammar(px: ParseContext):\n" \
              "  # во-первых, импортируем грамматику из всех подмодулей, если такие были\n" \
              "  if '_imported_syntax_modules' in globals():\n" \
              "    for sm in _imported_syntax_modules:\n" \
              "      if hasattr(sm, '_import_grammar'):\n" \
              "        sm._import_grammar(px)\n\n"
        for lhs, rhs, apply, lpr, rpr in self.exported_syntax:
            res += f'''  px.add_syntax_rule({lhs}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})\n'''
        for lhs, rhs, apply, lpr, rpr in self.exported_macro:
            res += f'''  px.add_macro_rule({lhs}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})\n'''
        for lhs, rhs, apply in self.exported_tokens:
            res += f'''  px.add_token({lhs}, {repr(rhs)}, apply={apply.__name__ if apply else 'None'})\n'''
        for lhs, rhs in self.exported_lexer_rules:
            res += f'''  px.add_lexer_rule({lhs}, {repr(rhs)})\n'''
        return res

    cpdef ast_to_text(self, ParseNode pn):
        return c_ast_to_text(self.px, pn.p)

    cdef int _add_rule(self, lhs, rhs, lpr, rpr):
        return addRule(self.px.grammar(), lhs + " -> " + rhs, -1, lpr, rpr)


cdef class Parser:
    cdef PythonParseContext* px
    cdef ParserState* state

    def __init__(self, px: ParseContext, text: str):
        self.px = px.px
        #self.init(text)
        self.state = new ParserState(self.px, text, b"")

    # cdef void init(self, const string& text):
    #     self.state = new ParserState(self.px, text, "")

    def __del__(self):
        if self.state:
            del self.state

    def __iter__(self):
        return self

    def __next__(self):
        cdef CParseNode* root = self.state.parse_next().root.get()
        if not root:
            raise StopIteration
        return ParseNode().init(root)


def parse_gen(px, text):
    for node in Parser(px, text):
        yield node

def syn_expand(node: ParseNode):
    px = parse_context()
    if node.is_terminal():
        apply = px.token_function(node.ntnum())
        if apply is not None:
            return apply(node.str())
        return node.str()

    # print(f'in syn_expand, rule = {node.rule}')
    f = px.syntax_function(node.rule())
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function for rule {node.rule}')
    return f(*node.children())

cpdef macro_expand(ParseContext px, ParseNode node):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule())
        if f is None:
            break
        node = f(*node.children())

    cdef int i
    for i in range(node.num_children()):
        node[i] = macro_expand(px, node[i])
    return node

cdef ParseContext c_parse_context():
    assert __parse_context__ is not None
    return __parse_context__

#ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);
cpdef quasiquote(ntname, vector[string] str_list, tree_list):
    assert str_list.size() == len(tree_list)+1
    cdef PythonParseContext* px = c_parse_context().px
    cdef vector[CParseNode*] subtrees
    cdef int i, n = len(tree_list)
    cdef ParseNode pn
    subtrees.resize(n)
    for i in range(n):
        pn = tree_list[i]
        subtrees[i] = pn.get()
    cdef CParseNode* nn = c_quasiquote(px, ntname, str_list, subtrees)
    return ParseNode().init(nn)

def ast_to_text(px, pn):
    return px.ast_to_text(pn)

def eval_in_context(node):
    return c_parse_context().eval(node)

class CppDbgFlags:
    SHIFT = 0x1
    REDUCE = 0x2
    STATE = 0x4
    LOOKAHEAD = 0x8
    TOKEN = 0x10
    RULES = 0x20
    QQ = 0x40
    ALL = 0xFFFFFFF


def set_debug(flags):
    set_cpp_debug(flags)
