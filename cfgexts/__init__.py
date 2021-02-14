# External imports
import sys

# Internal imports
from .importer import *

def init (syntax_file):
    importer.__dict__["__syntax_file__"] = syntax_file

    # Add the PYG importer at the begin of the list of finders
    sys.meta_path.insert(0, PygImporter)

def close ():
    sys.meta_path = [ finder for finder in sys.meta_path if finder is not PygImporter ]