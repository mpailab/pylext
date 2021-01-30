from ctypes import *

# dll = cdll.LoadLibrary('../lib/libpymacro.dylib')
# new_python_context = dll.new_python_context
# new_parser_state = dll.new_parser_state
# new_python_context.restype = c_void_p
# new_parser_state.restype = c_void_p

# for x in (dir(dll)):
#    print(x)
# print(c_void_p(dll.new_python_context()))
# print(dll.test_exn(-100))

from pymacros import *

with open('example/match_macro.py') as f:
    text = ''.join(f)

px = new_python_context()
ps = new_parser_state(px, b'0', b'')  # text.encode('utf8'), b'')
del_parser_state(ps)
del_python_context(px)

res = load_file(text, globals())
print(globals())
print(res)

