# mypy error: Module 'cfgexts.parser' has no attribute 'parser'
from . import parser # type: ignore

def apply (text):
    return parser.c_apply(text.encode('utf-8')).decode('utf-8')