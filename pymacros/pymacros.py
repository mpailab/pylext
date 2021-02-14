import os
from ctypes import *
from typing import List

parser = cdll.LoadLibrary('../lib/libpymacro.dylib')
# parser = cdll.LoadLibrary('../bin/pymacro.dll')

parser.get_last_error.restype = c_char_p


dbg_flag = 0


def dbgprint(msg, lvl):
    if lvl <= dbg_flag:
        print(msg)


class CppError(Exception):
    pass


def get_dll_func(dll, func, restype=None, err_code=None):
    f = getattr(dll, func)
    f.restype = restype
    if restype is None:
        return f
    if err_code is None:
        return lambda *args: restype(f(*args))
    else:
        def g(*args):
            r = f(*args)
            if (restype is c_int and r == err_code) or (restype is not c_int and not r):
                raise CppError(c_char_p(parser.get_last_error()).value.decode('utf8'))
            return restype(r)
        g.__name__ = func
        return g


c_quasiquote = get_dll_func(parser, 'c_quasiquote', c_void_p, 0)

new_python_context = get_dll_func(parser, 'new_python_context', c_void_p, 0)
del_python_context = get_dll_func(parser, 'del_python_context')

inc_pn_num_refs = get_dll_func(parser, 'inc_pn_num_refs')
dec_pn_num_refs = get_dll_func(parser, 'dec_pn_num_refs')

get_pn_num_children = get_dll_func(parser, 'get_pn_num_children', c_int)
get_pn_child = get_dll_func(parser, 'get_pn_child', c_void_p, 0)
set_pn_child = get_dll_func(parser, 'set_pn_child', c_int, err_code=-1)

get_pn_rule = get_dll_func(parser, 'get_pn_rule', c_int)
get_terminal_str = get_dll_func(parser, 'get_terminal_str', c_char_p, 0)

pn_equal = get_dll_func(parser, 'pn_equal', c_int)

add_rule = get_dll_func(parser, 'add_rule', c_int, -1)

new_parser_state = get_dll_func(parser, 'new_parser_state', c_void_p, 0)
continue_parse = get_dll_func(parser, 'continue_parse', c_void_p, 0)
del_parser_state = get_dll_func(parser, 'del_parser_state')

_ast_to_text = get_dll_func(parser, 'ast_to_text', c_char_p, 0)

set_cpp_debug = get_dll_func(parser, 'set_cpp_debug', c_int)

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

    @property
    def str(self):
        # print(f'str = {get_terminal_str(self.p).value}')
        return get_terminal_str(self.p).value.decode('utf8')

    @property
    def is_terminal(self):
        return self.rule < 0

    def __eq__(self, other):
        assert type(other) is ParseNode
        return parser.pn_equal(self.p, other.p) != 0

    def __repr__(self):
        return 'ParseNode:\n'+ast_to_text(parse_context(), self)


class ParseContext:
    def __init__(self, globals: dict):
        self.px = new_python_context()
        self.globals = globals
        self.syntax_rules = {}
        self.macro_rules = {}
        self.exported_syntax = []
        self.exported_macro = []

    def eval(self, expr):
        if type(expr) is ParseNode:
            expr = ast_to_text(self, expr)
        return eval(expr, self.globals)

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

    def add_macro_rule(self, lhs: str, rhs, apply, export=False, lpriority=None, rpriority=None):
        if lpriority is None: lpriority = -1
        if rpriority is None: rpriority = -1
        rhs = b' '.join(str(x).encode('utf8') for x in rhs)
        lhs = lhs.encode('utf8')
        # print(f'add rule: {lhs} -> {rhs}:',end='')
        rule_id = int(add_rule(self.px, c_char_p(lhs), c_char_p(rhs)).value)
        # print(f' id = {rule_id}, f = {apply}')
        # print(f'add macro rule {rule_id}')
        self.macro_rules[rule_id] = apply
        if export:
            self.exported_macro.append(dict(lhs=lhs, rhs=rhs, apply=apply, lpr=lpriority, rpr=rpriority))

    def add_syntax_rule(self, lhs: str, rhs, apply, export=False, lpriority=None, rpriority=None):
        if lpriority is None: lpriority = -1
        if rpriority is None: rpriority = -1
        rhs = b' '.join(str(x).encode('utf8') for x in rhs)
        lhs = lhs.encode('utf8')
        rule_id = int(add_rule(self.px, c_char_p(lhs), c_char_p(rhs)).value)
        # print(f'add syntax rule {rule_id}')
        self.syntax_rules[rule_id] = apply
        if export:
            self.exported_syntax.append(dict(lhs=lhs, rhs=rhs, apply=apply, lpr=lpriority, rpr=rpriority))

    def gen_syntax_import(self):
        res = "def _import_grammar(px: ParseContext):\n" \
              "  # во-первых, импортируем грамматику из всех подмодулей, если такие были\n" \
              "  if '_imported_syntax_modules' in globals():\n" \
              "    for sm in _imported_syntax_modules:\n" \
              "      if hasattr(sm, '_import_grammar'):\n" \
              "        sm._import_grammar(px)\n\n"
        for d in self.exported_syntax:
            res += f'''  px.add_syntax_rule({d['lhs']}, {repr(d['rhs'])}, apply={d['apply'].__name__}, lpriority={d['lpr']}, rpriority={d['rpr']})\n'''
        for d in self.exported_macro:
            res += f'''  px.add_macro_rule({d['lhs']}, {repr(d['rhs'])}, apply={d['apply'].__name__}, lpriority={d['lpr']}, rpriority={d['rpr']})\n'''
        return res


def parse_context() -> ParseContext:
    return __parse_context__


def macro_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_macro_rule(lhs, rhs, expand_func, export=True, lpriority=lpriority, rpriority=rpriority)
        return expand_func
    return set_func


def syntax_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_syntax_rule(lhs, rhs, expand_func, export=True, lpriority=lpriority, rpriority=rpriority)
        return expand_func
    return set_func


def quasiquote(ntname, str_list, tree_list: List[ParseNode]):
    assert len(str_list) == len(tree_list)+1
    b = [x.encode('utf8') for x in str_list]
    ntname = ntname.encode('utf8')
    px = parse_context()
    nn = c_quasiquote(px.px, ntname, c_int32(len(b)), (c_char_p*len(b))(*b),
                      (c_void_p*len(tree_list))(*[t.p for t in tree_list]))
    return ParseNode(nn)


class Parser:
    def __init__(self, px: ParseContext, text: str):
        b = text.encode('utf8')
        self.px = px
        # print(f'create parser state , px={px}, type = {type(px)}', end='...  ')
        self.state = new_parser_state(px.px, c_char_p(b), c_char_p(b''))
        # print('done')

    def __del__(self):
        if hasattr(self, 'state'):
            del_parser_state(self.state)

    def __iter__(self):
        return self

    def __next__(self):
        try:
            node = continue_parse(self.state)
            if not node.value:
                raise StopIteration
            return ParseNode(node)
        except CppError as e:
            if e.args[0]:
                raise
        raise StopIteration


def parse_gen(px, text):
    # print(f'px = {px}')
    for node in Parser(px, text):
        dbgprint('yield node', 1)
        yield node


def syn_expand(node: ParseNode):
    if node.is_terminal:
        return node.str

    # print(f'in syn_expand, rule = {node.rule}')
    px = parse_context()
    f = px.syntax_function(node.rule)
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function for rule {node.rule}')
    return f(*node.children) if f else node


def macro_expand(px: ParseContext, node: ParseNode):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule)
        # print(f'{node.rule}, f = {f}')
        if f is None:
            break
        node = f(*node.children)

    for i in range(len(node.children)):
        node[i] = macro_expand(px, node[i])
    #print(node)
    return node


def ast_to_text(px: ParseContext, ast: ParseNode):
    """ Преобразует синтаксическое дерево в текст """
    res = _ast_to_text(px.px, ast.p)
    #print(res.value.decode('utf8'))
    return res.value.decode('utf8')


def load_file(text, globals):
    """ Читает файл, раскрывыая макросы и сразу выполняя раскрытые блоки """
    expanded = ""
    with ParseContext(globals) as px:
        for stmt_ast in parse_gen(px, text):
            stmt_ast = macro_expand(px, stmt_ast)
            stmt = ast_to_text(px, stmt_ast)
            if dbg_flag >= 1:
                dbgprint(f'=========================\nexecute:\n{stmt}\n--- Output: ---', 1)
            expanded += stmt
            exec(stmt, globals, globals)
            dbgprint('==========================', 1)
        stmt = px.gen_syntax_import()
        print(f"syntax import function:\n{stmt}")
        exec(stmt)
        expanded += stmt

    return expanded
