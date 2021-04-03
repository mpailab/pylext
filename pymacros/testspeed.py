from pymacros import *
from time import time

if 1:
    print('\n Create python context:')
    repeat=1000
    t = time()
    for i in range(1000):
        px = ParseContext()
        del px

    t = time() - t

    print(f'Time  = {t:.3f} s')
    print(f'Speed = {repeat/t:.3f} empty files/s')


if 0:
    print('\n Single large file test:')
    with open('example/match_macro_speed.py') as f:
        text = ''.join(f)

    module_vars = {'__name__': '__main__'}
    exec('from pymacros import syntax_rule, macro_rule, syn_expand, quasiquote', module_vars, module_vars)
    t = time()
    res = load_file(text, module_vars)
    t = time() - t

    print(f'Time  = {t:.3f} s')
    print(f'Size  = {len(text)*1e-6:.3f} MB')
    print(f'Speed = {len(text)/t*1e-6:.3f} MB/s')

if 1:
    print('\n Many small files test:')
    t = time()
    repeat = 100
    for i in range(repeat):
        with open('example/match_macro.py') as f:
            text = ''.join(f)

        module_vars = {}
        exec('from pymacros import syntax_rule, macro_rule, syn_expand, quasiquote', module_vars, module_vars)
        res = load_file(text, module_vars)
    t = time() - t

    print(f'Time  = {t:.3f} s')
    print(f'Size  = {repeat} x {len(text)*1e-3:.3f} KB')
    print(f'Speed = {len(text)*repeat/t*1e-6:.3f} MB/s')
