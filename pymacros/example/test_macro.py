# coding=pymacro
import_syn match_macro

# Тестируем импортированный макрос
for x in range(10):
	match x:
		1: print(f'{x} -> a')
		2: print(f'{x} -> b')
		_ if x<5 or x>8: print(f'{x} -> c')
		_: print(f'{x} -> d')
