"""Importer of pyg files
"""  

# External imports
import pathlib
import sys
from importlib.machinery import ModuleSpec
from typing import List

# Internal imports
from .core.wrap import *
from .core.wrap import ast_to_text as _ast_to_text


def new_token(lhs: str, rhs: str, expand_func=None):
    if parse_context() is not None:
        parse_context().add_token(lhs, rhs, expand_func, export=True)


def new_lexer_rule(lhs: str, rhs: str):
    if parse_context() is not None:
        parse_context().add_lexer_rule(lhs, rhs, export=True)


def new_token_decorator(lhs: str, rhs: str):
    def set_func(expand_func):
        new_token(lhs, rhs, expand_func)
        return expand_func
    return set_func


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