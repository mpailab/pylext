# External imports
import sys

# Internal imports
from .base import exec_macros
from .importer import PygImporter as _PygImporter

# Add the PYG importer at the begin of the list of finders
sys.meta_path.insert(0, _PygImporter)
