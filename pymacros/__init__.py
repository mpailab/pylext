# External imports
import sys

# Internal imports
from .importer import PygImporter, exec_macros, exec_expand_macros

# Add the PYG importer at the begin of the list of finders
sys.meta_path.insert(0, PygImporter)

print('pymacros initialized')
