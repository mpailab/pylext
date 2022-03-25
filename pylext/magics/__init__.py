"""Pylext magic command for jupyter notebook"""

from .magics import PylextMagics


def load_ipython_extension(ipython):
    ipython.register_magics(PylextMagics)
