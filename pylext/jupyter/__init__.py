"""Pylext magic command for jupyter notebook"""
__version__ = '0.1.0'

from .magics import PylextMagics


def load_ipython_extension(ipython):
    ipython.register_magics(PylextMagics)
