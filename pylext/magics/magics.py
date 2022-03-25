import warnings

from IPython.core.error import UsageError
from IPython.core.magic import Magics, magics_class, cell_magic, line_magic, line_cell_magic

from pylext.core.parse import ParseContext, parse, macro_expand
from pylext.base import insert_pyg_builtins


@magics_class
class PylextMagics(Magics):
    def __init__(self, debug=False, **kwargs):
        super().__init__(**kwargs)
        self._contexts = {}

    @line_cell_magic
    def pylext(self, line: str, cell = None):
        """
        Create pylext magic command of the form
            %pylext type context [context ...]
        if cell is None and
            %%pylext [-d] [-e] [context]
            cell
        otherwise, where 
            type is 'clear', 'del', 'delete' or 'erase',
            context is a context name,
            -d is option that enables debugging mode,
            -e is option that enables only expanding mode.
        
        Parameters
        ----------
        line: str
            Arguments of pylext magic command, i.e. it is 
            `type context [context ...]` if cell is None,
            and is `[-d] [-e] [context]` otherwise.
        cell: str
            Jupyter notebook cell content.

        Raises
        ------
        UserWarning
            If %pylext command with no arguments
            If context doesn't exist
        UsageError
            If invalid option in cell mode
            If more than one context in cell mode
            If no contexts to delete in line mode
            If unknown type in line mode
        """
        if cell is None:
            return self._pylext_line(line)
        name = ''
        debug = False
        expand_only = False
        for arg in line.strip().split():
            if arg[0] == '-':
                if arg == '-d':
                    print('cell evaluated in pylext debug mode')
                    debug = True
                elif arg == '-e':
                    expand_only = True
                else:
                    raise UsageError(f'invalid option {arg}')
            else:
                if name:
                    raise UsageError(f'cannot activate more than one context: {name} and {arg}')
                name = arg
        name = name or 'default'
        if expand_only:
            print(f'pylext macros expanded in {f"context `{name}`" if name != "default" else "default context"}:')
        if name not in self._contexts:
            if debug:
                print(f'create new context `{name}`')
            insert_pyg_builtins(self.shell.user_ns)
            self._contexts[name] = ParseContext(self.shell.user_ns)
            self._contexts[name].rule_override(True)

        px = self._contexts[name]

        with px:
            for stmt_ast in parse(cell, px):
                if debug:
                    print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n', flush=True)
                expanded_ast = macro_expand(px, stmt_ast)
                if debug:
                    print('Expanded :')
                stmt = px.ast_to_text(expanded_ast)
                if debug:
                    print(f'{stmt}\n===========================================\n', flush=True)
                elif expand_only:
                    print(stmt)
                if not expand_only:
                    self.shell.run_cell(stmt)

        return None

    def _pylext_line(self, line: str):
        args = line.split()
        if len(args) == 0:
            warnings.warn(UserWarning("%pylext command with no arguments is ignored"))
            return
        if args[0] in ['clear', 'del', 'delete', 'erase']:
            if len(args) == 1:
                raise UsageError(f"%pylext {args[0]}: no contexts to delete")
            for arg in args[1:]:
                if arg in self._contexts:
                    del self._contexts[arg]
                    print(f'Context `{arg}` deleted')
                else:
                    warnings.warn(UserWarning(f"Context `{arg}` doesn't exist"))
        else:
            raise UsageError(f"Unknown magic command %pylext {args[0]}")

