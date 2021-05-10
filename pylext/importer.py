"""Importer of pyg files"""  

# External imports
import pathlib
import sys
from importlib.machinery import ModuleSpec

# Internal imports
from .base import exec_macros


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
        text = self.pyg_path.read_text(encoding="utf-8")
        module.__file__ = str(self.pyg_path)
        exec_macros(text, module.__dict__, str(self.pyg_path))

    def __repr__(self):
        """Nice representation of the class"""
        return f"{self.__class__.__name__}({str(self.pyg_path)!r})"