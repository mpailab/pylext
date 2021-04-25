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
        parse_context().add_token(lhs, rhs, expand_func, for_export=True)


def new_lexer_rule(lhs: str, rhs: str):
    if parse_context() is not None:
        parse_context().add_lexer_rule(lhs, rhs, for_export=True)


def new_token_decorator(lhs: str, rhs: str):
    def set_func(expand_func):
        new_token(lhs, rhs, expand_func)
        return expand_func
    return set_func


def macro_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_macro_rule(lhs, rhs, expand_func, for_export=True,
                                           lpriority=lpriority if lpriority is not None else -1,
                                           rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


def syntax_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_syntax_rule(lhs, rhs, expand_func, for_export=True,
                                           lpriority=lpriority if lpriority is not None else -1,
                                           rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


def set_rule_expansion(lhs: str, rhs: list):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().set_rule_expansion(lhs, rhs, expand_func, for_export=True)
        return expand_func
    return set_func


def get_rule_id(lhs, rhs):
    if type(rhs) is str:
        rhs = rhs.split()
    return parse_context().rule_id(lhs, rhs)


def gexport(f):
    """
    Декоратор для функций, которые при выполнении команды gimport
    импортируются под тем же именем, под которым объявлены.
    Необходим для функций, используемых внутри квазицитат при раскрытии макросов.
    """
    parse_context().export_function(f)
    return f


class GImportError(Exception):
    pass


def _gimport___(module, as_name, global_vars: dict):
    """ Функция, реализующая команду gimport """
    mname = as_name if as_name else module
    if as_name:
        exec(f'import {module} as {as_name}', global_vars)
    else:
        exec(f'import {module}', global_vars)
    global_vars.setdefault('_imported_syntax_modules', set())
    exec(f'_imported_syntax_modules.add({mname})', global_vars)
    loc = {}
    exec(f'import {module} as m', loc)
    if hasattr(loc['m'], '_import_grammar'):
        loc['m']._import_grammar(mname)


module_vars = {
    'ParseContext': ParseContext,
    'parse_context': parse_context,
    'syntax_rule' : syntax_rule,
    'macro_rule'  : macro_rule,
    'syn_expand'  : syn_expand,
    'quasiquote'  : quasiquote,
    'new_token'   : new_token,
    'new_lexer_rule': new_lexer_rule,
    'new_token_decorator': new_token_decorator,
    'eval_in_context': eval_in_context,
    '_gimport___' : _gimport___,
    'get_rule_id' : get_rule_id,
    'gexport'     : gexport
}

_dbg_statements = False


def exec_expand_macros(text, vars, by_stmt=False):
    """
    Загружает текст в интерпретатор, предварительно раскрывая в нём макросы.
    В конце добавляет функцию _import_grammar() для импорта из
    других модулей грамматики, объявленной в этом тексте.
    При этом возвращает текст с раскрытыми макросами
    """
    vars.update(module_vars)
    res = [] if by_stmt else ''
    with ParseContext(vars) as px:
        for stmt_ast in parse_gen(px, text):
            if _dbg_statements:
                print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n')
            stmt_ast = macro_expand(px, stmt_ast)
            if _dbg_statements:
                print('Expanded :\n')
            stmt = px.ast_to_text(stmt_ast)
            if _dbg_statements:
                print(f'{stmt}\n===========================================\n')
            res.append(stmt)
            exec(stmt, vars)
        stmt = px.gen_syntax_import()
        res.append(stmt)
        exec(stmt, vars)

    if not by_stmt:
        return ''.join(res)
    return res


def exec_macros(text, vars, filename=None):
    """
    Загружает текст в интерпретатор, предварительно раскрывая в нём макросы.
    В конце добавляет функцию _import_grammar() для импорта из
    других модулей грамматики, объявленной в этом тексте.
    """
    vars.update(module_vars)
    with ParseContext(vars) as px:
        for stmt_ast in parse_gen(px, text):
            if _dbg_statements:
                print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n')
            stmt_ast = macro_expand(px, stmt_ast)
            if _dbg_statements:
                print('Expanded:\n')
            stmt = px.ast_to_text(stmt_ast)
            if _dbg_statements:
                print(f'{stmt}\n===========================================\n')
            exec(stmt, vars)
        stmt = px.gen_syntax_import()
        if _dbg_statements:
            print(f'{stmt}\n===========================================\n')
        exec(stmt, vars)


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
        module.__file__ = str(self.pyg_path)
        exec_macros(text, module.__dict__, str(self.pyg_path))

    def __repr__(self):
        """Nice representation of the class"""
        return f"{self.__class__.__name__}({str(self.pyg_path)!r})"