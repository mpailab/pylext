import os
from ctypes import *
from typing import List


def get_dll_func(dll, func, restype=None):
    f = getattr(dll, func)
    f.restype = restype
    if restype is None:
        return f
    return lambda *args: restype(f(*args))


parser = cdll.LoadLibrary('../lib/libpymacro.dylib')

c_quasiquote = get_dll_func(parser, 'c_quasiquote', c_void_p)

new_python_context = get_dll_func(parser, 'new_python_context', c_void_p)
del_python_context = get_dll_func(parser, 'del_python_context')

inc_pn_num_refs = get_dll_func(parser, 'inc_pn_num_refs')
dec_pn_num_refs = get_dll_func(parser, 'dec_pn_num_refs')

get_pn_num_children = get_dll_func(parser, 'get_pn_num_children', c_int)
get_pn_child = get_dll_func(parser, 'get_pn_child', c_void_p)
set_pn_child = get_dll_func(parser, 'set_pn_child')

get_pn_rule = get_dll_func(parser, 'get_pn_rule', c_int)
pn_equal = get_dll_func(parser, 'pn_equal', c_int)

add_rule = get_dll_func(parser, 'add_rule', c_int)

new_parser_state = get_dll_func(parser, 'new_parser_state', c_void_p)
continue_parse = get_dll_func(parser, 'continue_parse', c_void_p)
del_parser_state = get_dll_func(parser, 'del_parser_state')

_ast_to_text = get_dll_func(parser, 'ast_to_text', c_char_p)


new_python_context.restype = c_void_p
new_parser_state.restype = c_void_p

__parse_context__ = None


class Terminal:
    def __init__(self, text: bytes):
        self.text = text if type(text) is bytes else text.encode('utf8')

    def str(self):
        return b"'"+self.text+b"'"


class Nonterminal:
    def __init__(self, text: bytes):
        self.text = text if type(text) is bytes else text.encode('utf8')

    def str(self):
        return self.text


class ParseNode:
    def __init__(self, cnode: c_void_p):
        self.p = cnode
        inc_pn_num_refs(self.p)

    def __del__(self):
        dec_pn_num_refs(self.p)

    def num_children(self):
        return int(get_pn_num_children(self.p).value)

    def __getitem__(self, i: int):
        return ParseNode(get_pn_child(self.p, c_int32(i)))

    def __setitem__(self, i: int, value):
        set_pn_child(self.p, c_int32(i), value.p)

    @property
    def children(self):
        return [self[i] for i in range(self.num_children())]

    @property
    def rule(self):
        return int(get_pn_rule(self.p).value)

    def __eq__(self, other):
        assert type(other) is ParseNode
        return parser.pn_equal(self.p, other.p).value != 0


class ParseContext:
    def __init__(self):  # , globals: dict):
        self.px: c_void_p = new_python_context()
        # self.globals = globals
        self.syntax_rules = {}
        self.macro_rules = {}

    def syntax_function(self, rule):
        return self.syntax_rules.get(rule, None)
        # res = c_char_p(parser.get_syntax_function(self.px, c_int64(rule)))
        # return self.globals.get(str(res.value), None) if res.value else None

    def macro_function(self, rule):
        return self.macro_rules.get(rule, None)
        # res = c_char_p(parser.get_macro_function(self.px, c_int64(rule)))
        # return self.globals.get(str(res.value), None) if res.value else None

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

    def add_macro_rule(self, lhs: str, rhs, apply):
        rhs = b' '.join(x.str() for x in rhs)
        lhs = lhs.encode('utf8')
        rule_id = int(add_rule(self.px, c_char_p(lhs), c_char_p(rhs)).value)
        self.macro_rules[rule_id] = apply

    def add_syntax_rule(self, lhs: str, rhs, apply):
        rhs = b' '.join(x.str() for x in rhs)
        lhs = lhs.encode('utf8')
        rule_id = int(add_rule(self.px, c_char_p(lhs), c_char_p(rhs)).value)
        self.syntax_rules[rule_id] = apply


def parse_context() -> ParseContext:
    return __parse_context__


def quasiquote(ntname, str_list, tree_list: List[ParseNode]):
    assert len(str_list) == len(tree_list)+1
    b = [x.encode('utf8') for x in str_list]
    ntname = ntname.encode('utf8')
    nn = c_quasiquote(ntname, c_int32(len(b)), (c_char_p*len(b))(*b),
                      (c_void_p*len(tree_list))(*[t.p for t in tree_list]))
    return ParseNode(nn)


class Parser:
    def __init__(self, px: ParseContext, text: str):
        b = text.encode('utf8')
        self.px = px
        print(f'create parser state , px={px}, type = {type(px)}', end='...  ')
        self.state = new_parser_state(px.px, c_char_p(b), c_char_p(b''))
        print('done')

    def __del__(self):
        if hasattr(self, 'state'):
            del_parser_state(self.state)

    def __iter__(self):
        return self

    def __next__(self):
        node = continue_parse(self.state)
        if not node.value:
            raise StopIteration
        return ParseNode(node)


def parse_gen(px, text):
    print(f'px = {px}')
    for node in Parser(px, text):
        print('yield node')
        yield node


def syn_expand(node: ParseNode):
    px = parse_context()
    f = px.syntax_function(node.rule)
    return f(px, *node.children) if f else node


def macro_expand(px: ParseContext, node: ParseNode):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule)
        if f is None:
            break
        node = f(px, *node.children)

    for i in range(len(node.children)):
        node.children[i] = macro_expand(px, node.children[i])

    return node


def ast_to_text(px: ParseContext, ast: ParseNode):
    """ Преобразует синтаксическое дерево в текст """
    return str(c_char_p(_ast_to_text(px.px, ast.p)).value)


def load_file(text, globals):
    """ Читает файл, раскрывыая макросы и сразу выполняя раскрытые блоки """
    expanded = ""
    with ParseContext() as px:
        for stmt_ast in parse_gen(px, text):
            stmt_ast = macro_expand(px, stmt_ast)
            stmt = ast_to_text(px, stmt_ast)
            expanded += stmt
            exec(stmt, globals, globals)
    return expanded
