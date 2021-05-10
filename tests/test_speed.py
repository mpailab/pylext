from time import time
import pytest
import os
from pylext.core.parse import ParseContext
from pylext import exec_macros
from pylext.base import *


@pytest.mark.long
@pytest.mark.benchmark(group="pylext-parser")
def test_context_creation(benchmark):
    @benchmark
    def create_context():
        px = ParseContext({})
        del px


@pytest.mark.long
@pytest.mark.benchmark(group="pylext-parser", min_rounds=1, warmup=False)
def test_big_file(benchmark):
    with open('tests/macros/big_file.pyg') as f:
        text = ''.join(f)
    benchmark(exec_macros, text, {})


@pytest.mark.long
@pytest.mark.benchmark(group="pylext-parser", min_rounds=1)
def test_small_file(benchmark):
    with open('tests/macros/match.pyg') as f:
        text = ''.join(f)
    benchmark(exec_macros, text, {})


def measure_speed():
    print('\n Create python context:')
    repeat = 100
    t = time()
    for i in range(repeat):
        px = ParseContext({})
        del px

    t = time() - t

    print(f'Time  = {t:.3f} s')
    print(f'Speed = {repeat/t:.3f} empty files/s')

    print('\n Single large file test:')
    with open('tests/macros/big_file.pyg') as f:
        text = ''.join(f)

    vars = {}
    t = time()
    res = exec_macros(text, vars, by_stmt=True)
    t = time() - t

    print(f'Time           = {t:.3f} s')
    print(f'Size          = {len(text)*1e-6:.3f} MB')
    print(f'Speed         = {len(text)/t*1e-6:.3f} MB/s')
    print(f'Expanded size = {len("".join(res))*1e-6:.3f} MB')

    try:
        os.mkdir("tests/temp")
    except FileExistsError:
        pass

    with open('tests/temp/expanded.py', 'w') as f:
        f.write("import pylext.base\n")
        f.write("pylext.base.insert_pyg_builtins(globals())\n")
        f.write(''.join(res))
    t1 = time()
    import temp.expanded
    t1 = time() - t1
    os.remove("tests/temp/expanded.py")
    try:
        os.rmdir("tests/temp")
    except OSError:
        pass
    print(f'Standard import time   = {t1:.3f} s')
    # Почему-то обычный импорт дольше, надо бы разобраться

    t2 = time()
    vars = pyg_builtins()
    for stmt in res:
        exec(stmt, vars)
    t2 = time() - t2
    print(f'Exec by statement time = {t2:.3f} s')

    print('\n Many small files test:')
    t = time()
    repeat = 100
    for i in range(repeat):
        with open('tests/macros/match.pyg') as f:
            text = ''.join(f)
        vars = {}
        res = exec_macros(text, vars)
    t = time() - t

    print(f'Time  = {t:.3f} s')
    print(f'Size  = {repeat} x {len(text)*1e-3:.3f} KB')
    print(f'Speed = {len(text)*repeat/t*1e-6:.3f} MB/s')


if __name__ == '__main__':
    measure_speed()
