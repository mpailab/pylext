import os
from ctypes import *
from typing import List

parser = cdll.LoadLibrary('parser.dll')

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
        parser.inc_pn_num_refs(self.p)

    def __del__(self):
        parser.dec_pn_num_refs(self.p)

    def num_children(self):
        return int(c_int32(parser.get_pn_num_children(self.p)).value)

    def __getitem__(self, i: int):
        return ParseNode(c_void_p(parser.get_pn_child(self.p, c_int32(i))))

    def __setitem__(self, i: int, value):
        parser.set_pn_child(self.p, c_int32(i), value.p)

    @property
    def children(self):
        return [self[i] for i in range(self.num_children())]

    @property
    def rule(self):
        return int(c_int32(parser.get_pn_rule(self.p)).value)

    def __eq__(self, other):
        assert type(other) is ParseNode
        return c_int32(parser.pn_equal(self.p, other.p)).value != 0


class ParseContext:
    def __init__(self):  # , globals: dict):
        self.px: c_void_p = parser.new_python_context()
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

    def __exit__(self, exc_type, exc_val, exc_tb):
        global __parse_context__
        __parse_context__ = None

    def __del__(self):
        parser.del_python_context(self.px)

    def add_macro_rule(self, lhs: str, rhs, apply):
        rhs = b' '.join(x.str() for x in rhs)
        lhs = lhs.encode('utf8')
        rule_id = c_int32(parser.add_rule(self.px, c_char_p(lhs), c_char_p(rhs)))
        self.macro_rules[rule_id] = apply

    def add_syntax_rule(self, lhs: str, rhs, apply):
        rhs = b' '.join(x.str() for x in rhs)
        lhs = lhs.encode('utf8')
        rule_id = int(c_int32(parser.add_rule(self.px, c_char_p(lhs), c_char_p(rhs))).value)
        self.syntax_rules[rule_id] = apply


def parse_context() -> ParseContext:
    return __parse_context__


def quasiquote(ntname, str_list, tree_list: List[ParseNode]):
    assert len(str_list) == len(tree_list)+1
    b = [x.encode('utf8') for x in str_list]
    ntname = ntname.encode('utf8')
    nn = c_void_p(parser.c_quasiquote(ntname, c_int32(len(b)), (c_char_p*len(b))(*b),
                                      (c_void_p*len(tree_list))(*[t.p for t in tree_list])))
    return ParseNode(nn)


class Parser:
    def __init__(self, px: ParseContext, text: str):
        b = text.encode('utf8')
        self.px = px
        self.p = c_void_p(parser.new_parser_state(c_char_p(b)))

    def __del__(self):
        parser.del_parser_state(self.p)

    def __iter__(self):
        return self

    def __next__(self):
        node = c_void_p(parser.continue_parse(self.px.px, self.p))
        if not node.value:
            raise StopIteration
        return ParseNode(node)


def parse_gen(px, text):
    for node in Parser(text):
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
    return str(c_char_p(parser.ast_to_text(px.px, ast.p)))


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
