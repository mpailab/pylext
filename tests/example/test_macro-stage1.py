import match_macro # команда сразу выполняется в интерпретаторе,
                   # а также выполняется код match_macro._import_grammar(), который не включается в текст stage1
_imported_syntax_modules = [match_macro] # команда добавляется после импорта грамматики из модуля

#######################################
expand_macros("""
    for x in range(10):
        match x:
            1: print(f'{x} -> a')
            2: print(f'{x} -> b')
            _ if x<5 or x>8: print(f'{x} -> c')
            _: print(f'{x} -> d')
    """)
    
######################################
def _import_grammar():
    # во-первых, импортируем грамматику из всех подмодулей, если такие были
    if '_imported_syntax_modules' in globals(): 
        for sm in _imported_syntax_modules:
            if hasattr(sm, '_import_grammar'):
                sm._import_grammar()
