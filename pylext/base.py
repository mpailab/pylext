"""Base methods for pyg files"""  

# External imports

# Internal imports
from .core.parse import *


# decorator for token expansion declaration
def _new_token_decorator(lhs: str, rhs: str):
    def set_func(expand_func):
        new_token(lhs, rhs, expand_func)
        return expand_func
    return set_func


# decorator for macro rule declaration
def _macro_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        ctx = parse_context_not_check()
        if ctx is not None:
            ctx.add_macro_rule( lhs, rhs, expand_func, for_export=True,
                                lpriority=lpriority if lpriority is not None else -1,
                                rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


# decorator for syntax rule declaration
def _syntax_rule(lhs: str, rhs: list, lpriority=None, rpriority=None):
    def set_func(expand_func):
        ctx = parse_context_not_check()
        if ctx is not None:
            ctx.add_syntax_rule(lhs, rhs, expand_func, for_export=True,
                                lpriority=lpriority if lpriority is not None else -1,
                                rpriority=rpriority if lpriority is not None else -1)
        return expand_func
    return set_func


# decorator for expansion function for built-in grammar rule
def _add_pyexpand_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        ctx = parse_context_not_check()
        if ctx is not None:
            ctx.add_pyexpand_rule(lhs, rhs, expand_func, for_export=True)
        return expand_func
    return set_func


def new_token(lhs: str, rhs: str, expand_func=None):
    """
    Add new token into grammar.
    This adds new nonterminal with rule in lexer's parsing expression grammar

    Parameters
    ----------
    lhs: str
        Name of new token (PEG nonterminal).
    rhs: str
        Parsing expression describing new token.
    expand_func: callable, optional
        Function called when expansion of token is requested.

    Raises
    ------
    InvalidParseContext
        If function called not during pyg module import
    ValueError
        If token with this name already exist
        If error in parsing expression
    """
    ctx = parse_context_not_check()
    if ctx is not None:
        ctx.add_token_rule(lhs, rhs, expand_func, for_export=True)


def new_lexer_rule(lhs: str, rhs: str):
    """
    Add rule to lexer's parsing expression grammar

    Parameters
    ----------
    lhs: str
        Name of PEG nonterminal.
    rhs: str
        Parsing expression for new PEG nonterminal.

    Raises
    ------
    InvalidParseContext
        If function called not during pyg module import
    ValueError
        If token with this name already exist
        If error in parsing expression
    """
    ctx = parse_context_not_check()
    if ctx is not None:
        ctx.add_lexer_rule(lhs, rhs, for_export=True)


def get_rule_id(lhs, rhs):
    """
    Finds rule in current parse context and returns its id

    Parameters
    ----------
    lhs: str
        Rule left-hand side, nonterminal name
    rhs: list of str
        Rule right-hand side, list of terminal names and nonterminal names

    Returns
    -------
    int
        Identifier of rule lhs -> rhs in current parse context

    Raises
    ------
    ValueError
        If such rule lrs -> rhs doesn't exist in current context
    InvalidParseContext
        If function called not during pyg module import
    """
    if type(rhs) is str:
        rhs = rhs.split()
    return parse_context().rule_id(lhs, rhs)


def gexport(f):
    """
    Decorator for functions that need to be imported
    under same name as they declared without prefix
    when import is done by gimport command.

    This is necessary for functions used inside quasiquotes in macro definitions.

    Raises
    ------
    InvalidParseContext
        If function called not during pyg module import
    """
    parse_context().add_export_func(f)
    return f


def _gimport___(module, as_name, global_vars: dict):
    """ Function implements gimport command """
    mname = as_name if as_name else module
    if _dbg_statements:
        print(f'importing module {module} as {mname}', flush=True)
    if as_name:
        exec(f'import {module} as {as_name}', global_vars)
    else:
        exec(f'import {module}', global_vars)
    if _dbg_statements:
        print(f'module {module} imported successfully', flush=True)
    global_vars.setdefault('_imported_syntax_modules', set())
    exec(f'_imported_syntax_modules.add({mname})', global_vars)
    loc = {}
    exec(f'import {module} as m', loc)
    if hasattr(loc['m'], '_import_grammar'):
        if _dbg_statements:
            print(f'Execute {module}._import_grammar({mname})',flush=True)
        loc['m']._import_grammar(mname)
        if _dbg_statements:
            print('_import_grammar finished', flush=True)


def exec_in_context(text, return_expanded=False):
    """
    Parses text statement by statement and expand macros,
    then loads it into interpreter. In the end of text adds function
    _import_grammar() that will be executed if this module will be imported
    by gimport command.

    Parameters
    ----------
    text: str
        Text to be parsed
    return_expanded: bool
        Whether to return expanded macros

    Returns
    -------
    str or None
        Expanded macros if return_expanded = True
        or None otherwise
    """
    res = []
    px = parse_context()
    vars = px.globals()
    #insert_pyg_builtins(vars)

    for stmt_ast in parse(text, px):
        if _dbg_statements:
            print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n', flush=True)
        stmt_ast = macro_expand(px, stmt_ast)
        if _dbg_statements:
            print('Expanded :\n')
        stmt = px.ast_to_text(stmt_ast)
        if _dbg_statements:
            print(f'{stmt}\n===========================================\n', flush=True)
        res.append(stmt)
        exec(stmt, vars)

    return ''.join(res) if return_expanded else None


_module_vars = {
    # internal built-in functions:
    '_syntax_rule' : _syntax_rule,
    '_macro_rule'  : _macro_rule,
    '_new_token_decorator': _new_token_decorator,
    '_gimport___'  : _gimport___,
    '_add_pyexpand_rule': _add_pyexpand_rule,

    # built-in functions that can be called by user:
    'syn_expand'   : syn_expand,
    'parse_context': parse_context,
    'quasiquote'   : quasiquote,
    'new_token'    : new_token,
    'new_lexer_rule': new_lexer_rule,
    'eval_in_context': eval_in_context,
    'exec_in_context': exec_in_context,
    'get_rule_id' : get_rule_id,
    'gexport'     : gexport
}


def insert_pyg_builtins(vars: dict):
    """
    Inserts pyg built-in functions into vars dictionary

    Parameters
    ----------
    vars : dict
        Dictionary of imported module global variables
    """
    vars.update(_module_vars)


def pyg_builtins():
    """
    Creates dictionary with pyg built-in functions

    Returns
    ----------
    dict
        Copy of dictionary of pyg built-in functions
    """
    return {**_module_vars}


_dbg_statements = False


def exec_macros(text, vars, filename=None, by_stmt=False):
    """
    Parses text statement by statement and expand macros,
    then loads it into interpreter. In the end of text adds function
    _import_grammar() that will be executed if this module will be imported
    by gimport command.

    Parameters
    ----------
    text: str
        Text to be parsed
    vars: dict
        Global variables to pass in exec()

    Returns
    -------
    str or list of str
        Text with expanded macros if by_stmt = False
        or list of statements with expanded macros if by_stmt = True
    """
    insert_pyg_builtins(vars)
    res = []
    with ParseContext(vars) as px:
        for stmt_ast in parse(text, px):
            if _dbg_statements:
                print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n', flush=True)
            stmt_ast = macro_expand(px, stmt_ast)
            if _dbg_statements:
                print('Expanded :\n')
            stmt = px.ast_to_text(stmt_ast)
            if _dbg_statements:
                print(f'{stmt}\n===========================================\n', flush=True)
            res.append(stmt)
            exec(stmt, vars)
        stmt = px.import_grammar_str()
        if _dbg_statements:
            print(f'===== Generated _import_grammar function =====\n')
            print(f'{stmt}\n===========================================\n', flush=True)
        res.append(stmt)
        exec(stmt, vars)

    return res if by_stmt else ''.join(res)


# functions imported by command `from pylext.base import *`
__all__ = ['exec_macros', 'pyg_builtins', 'insert_pyg_builtins']
