# from __future__ import print_function
from IPython.core.magic import Magics, magics_class, cell_magic

from pylext.core.parse import ParseContext, parse, macro_expand
from pylext.base import insert_pyg_builtins


@magics_class
class PylextMagics(Magics):
    def __init__(self, debug=False, **kwargs):
        super().__init__(**kwargs)
        self._contexts = {}
        # self._context = None
        self._debug = debug

    @cell_magic
    def pylext(self, line: str, cell):
        """pylext cell magic"""
        line = line.strip()
        if line not in self._contexts:
            print(f'create new context `{line}`')
            insert_pyg_builtins(self.shell.user_ns)
            self._contexts[line] = ParseContext(self.shell.user_ns)
        px = self._contexts[line]

        with px:
            for stmt_ast in parse(cell, px):
                if self._debug:
                    print(f'\nProcess statement:\n\n{px.ast_to_text(stmt_ast)}\n', flush=True)
                expanded_ast = macro_expand(px, stmt_ast)
                if self._debug:
                    print('Expanded :\n')
                stmt = px.ast_to_text(expanded_ast)
                if self._debug:
                    print(f'{stmt}\n===========================================\n', flush=True)
                self.shell.run_cell(stmt)

        return None
