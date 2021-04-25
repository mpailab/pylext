from time import time
import os
import pylext.core.wrap as parser
from pylext import exec_macros, exec_expand_macros
from pylext.importer import module_vars


print('\n Create python context:')
repeat = 100
t = time()
for i in range(repeat):
    px = parser.ParseContext({})
    del px

t = time() - t

print(f'Time  = {t:.3f} s')
print(f'Speed = {repeat/t:.3f} empty files/s')


print('\n Single large file test:')
with open('macros/big_file.pyg') as f:
    text = ''.join(f)

vars = {}
t = time()
res = exec_expand_macros(text, vars, by_stmt=True)
t = time() - t

print(f'Time           = {t:.3f} s')
print(f'Size          = {len(text)*1e-6:.3f} MB')
print(f'Speed         = {len(text)/t*1e-6:.3f} MB/s')
print(f'Expanded size = {len("".join(res))*1e-6:.3f} MB')


try:
    os.mkdir("temp")
except FileExistsError:
    pass

with open('temp/expanded.py', 'w') as f:
    f.write("from pylext.importer import *\n")
    f.write(''.join(res))
t1 = time()
import temp.expanded
t1 = time() - t1
os.remove("temp/expanded.py")
try:
    os.rmdir("temp")
except OSError:
    pass
print(f'Standard import time   = {t1:.3f} s')
# Почему-то обычный импорт дольше, надо бы разобраться

t2 = time()
vars = {**module_vars}
for stmt in res:
    exec(stmt, vars)
t2 = time() - t2
print(f'Exec by statement time = {t2:.3f} s')


print('\n Many small files test:')
t = time()
repeat = 100
for i in range(repeat):
    with open('macros/match.pyg') as f:
        text = ''.join(f)
    vars = {}
    res = exec_macros(text, vars)
t = time() - t

print(f'Time  = {t:.3f} s')
print(f'Size  = {repeat} x {len(text)*1e-3:.3f} KB')
print(f'Speed = {len(text)*repeat/t*1e-6:.3f} MB/s')
