"""Importer of pyg files
"""  

# External imports
import pathlib
import sys
from importlib.machinery import ModuleSpec
from typing import List

# Internal imports
from .parser.wrap import *
from .parser.wrap import ast_to_text as _ast_to_text

__parse_context__ = None
__syntax_file__ = None

class CppError(Exception):
    pass

class ParseNode:
    def __init__(self, cnode):
        self.p = cnode
        inc_pn_num_refs(self.p)

    def __del__(self):
        dec_pn_num_refs(self.p)

    def num_children(self):
        return get_pn_num_children(self.p)

    def __getitem__(self, i: int):
        return ParseNode(get_pn_child(self.p, i))

    def __setitem__(self, i: int, value):
        set_pn_child(self.p, i, value.p)

    @property
    def children(self):
        return [self[i] for i in range(self.num_children())]

    @property
    def rule(self):
        return get_pn_rule(self.p)

    def __eq__(self, other):
        assert type(other) is ParseNode
        return pn_equal(self.p, other.p) != 0

    def __repr__(self):
        return 'ParseNode:\n'+ast_to_text(parse_context(), self)


class ParseContext:
    def __init__(self):
        self.px = new_python_context(1, __syntax_file__)
        self.syntax_rules = {}
        self.macro_rules = {}
        self.exported_syntax = []
        self.exported_macro = []

    def syntax_function(self, rule):
        return self.syntax_rules.get(rule, None)

    def macro_function(self, rule):
        return self.macro_rules.get(rule, None)

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

    def add_macro_rule(self, lhs: str, rhs, apply, export=False):
        rhs = ' '.join(str(x) for x in rhs)
        rule_id = add_rule(self.px, lhs, rhs)
        self.macro_rules[rule_id] = apply
        if export:
             self.exported_macro.append(dict(lhs=lhs, rhs=rhs, apply=apply))

    def add_syntax_rule(self, lhs: str, rhs, apply, export=False):
        rhs = ' '.join(str(x) for x in rhs)
        rule_id = add_rule(self.px, lhs, rhs)
        self.syntax_rules[rule_id] = apply
        if export:
             self.exported_syntax.append(dict(lhs=lhs, rhs=rhs, apply=apply))

    def gen_syntax_import(self):
        res = "def _import_grammar(px: ParseContext):\n" \
            "  # во-первых, импортируем грамматику из всех подмодулей, если такие были\n" \
            "  if '_imported_syntax_modules' in globals():\n" \
            "    for sm in _imported_syntax_modules:\n" \
            "      if hasattr(sm, '_import_grammar'):\n" \
            "        sm._import_grammar(px)\n\n"
        for d in self.exported_syntax:
            res += f'''  px.add_syntax_rule({d['lhs']}, {repr(d['rhs'])}, apply={d['apply'].__name__})\n'''
        for d in self.exported_macro:
            res += f'''  px.add_macro_rule({d['lhs']}, {repr(d['rhs'])}, apply={d['apply'].__name__})\n'''
        return res


def parse_context() -> ParseContext:
    return __parse_context__


def macro_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_macro_rule(lhs, rhs, expand_func, export=True)
        return expand_func
    return set_func


def syntax_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_syntax_rule(lhs, rhs, expand_func, export=True)
        return expand_func
    return set_func


def quasiquote(ntname, str_list, tree_list: List[ParseNode]):
    assert len(str_list) == len(tree_list)+1
    px = parse_context()
    nn = c_quasiquote(px.px, ntname, len(str_list), str_list, [t.p for t in tree_list])
    return ParseNode(nn)


class Parser:
    def __init__(self, px: ParseContext, text: str):
        self.px = px
        self.state = new_parser_state(px.px, text, "")

    def __del__(self):
        if hasattr(self, 'state'):
            del_parser_state(self.state)

    def __iter__(self):
        return self

    def __next__(self):
        try:
            node = continue_parse(self.state)
            if not node:
                raise StopIteration
            return ParseNode(node)
        except CppError as e:
            if e.args[0]:
                raise
        raise StopIteration


def parse_gen(px, text):
    for node in Parser(px, text):
        yield node


def syn_expand(node: ParseNode):
    px = parse_context()
    f = px.syntax_function(node.rule)
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function for rule {node.rule}')
    return f(*node.children) if f else node


def macro_expand(px: ParseContext, node: ParseNode):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule)
        if f is None:
            break
        node = f(*node.children)

    for i in range(len(node.children)):
        node[i] = macro_expand(px, node[i])
    return node


def ast_to_text(px: ParseContext, ast: ParseNode):
    """ Преобразует синтаксическое дерево в текст """
    return _ast_to_text(px.px, ast.p)

module_vars = {
    'ParseContext': ParseContext,
    'syntax_rule' : syntax_rule,
    'macro_rule'  : macro_rule,
    'syn_expand'  : syn_expand,
    'quasiquote'  : quasiquote
}

class PygImporter():
    def __init__(self, pyg_path):
        """Store path to PYG file"""
        self.pyg_path = pyg_path

    @classmethod
    def find_spec(cls, name, path, target=None):
        """Look for PYG file"""
        package, _, module_name = name.rpartition(".")
        pyg_file_name = f"{module_name}.pyg"
        directories = sys.path if path is None else path
        for directory in directories:
            pyg_path = pathlib.Path(directory) / pyg_file_name
            if pyg_path.exists():
                return ModuleSpec(name, cls(pyg_path))

    def create_module(self, spec):
        """Returning None uses the standard machinery for creating modules"""
        return None

    def exec_module(self, module):
        """Executing the module means reading the PYG file"""
        text = self.pyg_path.read_text()
        module.__dict__.update(module_vars)
        module.__file__ = str(self.pyg_path)
        with ParseContext() as px:
            for stmt_ast in parse_gen(px, text):
                stmt_ast = macro_expand(px, stmt_ast)
                stmt = ast_to_text(px, stmt_ast)
                exec(stmt, module.__dict__)
            stmt = px.gen_syntax_import()
            exec(stmt, module.__dict__)

    def __repr__(self):
        """Nice representation of the class"""
        return f"{self.__class__.__name__}({str(self.pyg_path)!r})"