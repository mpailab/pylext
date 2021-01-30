import match_macro # команда сразу выполняется в интерпретаторе,
                   # а также выполняется код match_macro._import_grammar(), который не включается в текст stage1
_imported_syntax_modules = [match_macro] # команда добавляется после импорта грамматики из модуля

for x in range(10):
    if x==1 and True: print(f'{x} -> a')
    elif x==2 and True: print(f'{x} -> b')
    elif True and x<5 or x>8: print(f'{x} -> c')
    elif True and True: print(f'{x} -> d')
    
######################################
def _import_grammar():
    # во-первых, импортируем грамматику из всех подмодулей, если такие были
    if '_imported_syntax_modules' in globals(): 
        for sm in _imported_syntax_modules:
            if hasattr(sm, '_import_grammar'):
                sm._import_grammar()
