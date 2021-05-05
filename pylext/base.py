"""Внутренние функции, которые могут использовать в pyg файлах
"""  

# External imports
import pathlib
import sys
from importlib.machinery import ModuleSpec
from typing import List

# Internal imports
from .core.parse import *

def new_token(lhs: str, rhs: str, expand_func=None):
    ctx = parse_context()
    if ctx is not None:
        ctx.add_token_rule(lhs, rhs, expand_func, for_export=True)


def new_lexer_rule(lhs: str, rhs: str):
    ctx = parse_context()
    if ctx is not None:
        ctx.add_lexer_rule(lhs, rhs, for_export=True)


def new_token_decorator(lhs: str, rhs: str):
    def set_func(expand_func):
        new_token(lhs, rhs, expand_func)
        return expand_func
    return set_func


def macro_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        ctx = parse_context()
        if ctx is not None:
            ctx.add_macro_rule( lhs, rhs, expand_func, for_export=True,
                                lpriority=lpriority if lpriority is not None else -1,
                                rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


def syntax_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        ctx = parse_context()
        if ctx is not None:
            ctx.add_syntax_rule(lhs, rhs, expand_func, for_export=True,
                                lpriority=lpriority if lpriority is not None else -1,
                                rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


def add_pyexpand_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        ctx = parse_context()
        if ctx is not None:
            ctx.add_pyexpand_rule(lhs, rhs, expand_func, for_export=True)
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
    parse_context().add_export_func(f)
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
    'parse_context': parse_context,
    'syntax_rule' : syntax_rule,
    'macro_rule'  : macro_rule,
    'syn_expand' : syn_expand,
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

def exec_macros(text, vars, filename=None, by_stmt=False):
    """
    Загружает текст в интерпретатор, предварительно раскрывая в нём макросы.
    В конце добавляет функцию _import_grammar() для импорта из
    других модулей грамматики, объявленной в этом тексте.
    При этом возвращает текст с раскрытыми макросами
    """
    vars.update(module_vars)
    res = []
    with ParseContext(vars) as px:
        for stmt_ast in parse(text, px):
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
        stmt = px.import_grammar_str()
        res.append(stmt)
        exec(stmt, vars)

    return res if by_stmt else ''.join(res)