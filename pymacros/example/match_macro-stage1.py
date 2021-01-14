########### state1: список строк или вызовов функций раскрытия макросов -- результат работы парсера ###########
## элемент 1: строка, полученная из поддерева, не использующего макрорасширения
def _syntax_rule_matchcase_0(pattern, action):
    return (pattern, quasiquote("expr", ["true"], []), action)
    
def _syntax_rule_matchcase_1(pattern, cond, action):
    return (pattern,cond,action)

def _syntax_rule_matchcases_0(x):
    x = syn_expand(x)
    return [x]

def _syntax_rule_matchcases_1(xs, x):
    xs = syn_expand(xs)
    x = syn_expand(x)
    xs.append(x)
    return xs

#Максимально упрощённый pattern-matching (просто сравнение на равенство)
def match2cmp(x,pat):
    # tmatch(x, y) сравнивает верх дерева x с деревом y. На месте подстановочных символов (в квазицитате -- $_) в y могут быть поддеревья из x
    if tmatch(pat, quasiquote("expr", ["_"], [])): # должна быть встроенная функция, сравнивающая деревья разбора
        return quasiquote("expr", ["true"], [])
    # функция quasiquote(q, args) читает дерево из бинарного представления q, и листья, соответсвующие аргументам заменяет на аргументы из списка args		
    return quasiquote("expr", ["","==",""], [x, pat])

def _macro_match(x, mc):
    mc = syn_expand(mc)
    conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in mc]
    head = quasiquote("if_stmt_no_else", ["if", " and ", ": ", ""], [conds[0][0], conds[0][1], conds[0][2]])
    for (c, cond, s) in conds[1:]:
        head = quasiquote("expr", ["", " $$INDENTED elif ", " and ", ": ", ""], [head, c, cond, s])
    return head

###########################################################
##### элемент 2: вызов функции, раскрывающей макрос #######

expand_macros(""" ### на этом месте на самом деле стоит просто идентификатор дерева разбора этого текста
if __name__ == '__main__':
    for x in range(10):
        match x:
            1: print(f'{x} -> a')
            2: print(f'{x} -> b')
            _ if x<5 or x>8: print(f'{x} -> c')
            _: print(f'{x} -> d')
""")

###########################################################
##### элемент 3: строка -- функция для импорта расширений грамматики из данного модуля

def _import_grammar():
    # во-первых, импортируем грамматику из всех подмодулей, если такие были
    if '_imported_syntax_modules' in globals(): 
        for sm in _imported_syntax_modules:
            if hasattr(sm, '_import_grammar'):
                sm._import_grammar()

    t  = terminal
    nt = nonterminal
    # добавляем грамматику из этого модуля
    add_grammar_rules(current_grammar(), [
    syntax_rule('matchcase', [t('INDENTED'), nt('expr'), t(':'), nt('suite')], apply=_syntax_rule_matchcase_0),
    syntax_rule('matchcase', [t('INDENTED'), nt('expr'), t('if'), nt('test'), t(':'), nt('suite')], apply=_syntax_rule_matchcase_1),
    syntax_rule('matchcases', [nt('matchcase')], apply=_syntax_rule_matchcases_0),
    syntax_rule('matchcases', [nt('matchcases'), nt('matchcase')], apply=_syntax_rule_matchcases_1),
    macro_rule('stmt', [t('match'), nt('expr'), t(':'), t('INDENT'), nt('matchcases'), t('DEDENT')], apply=_macro_stmt_match)
    ]) # в принципе, для ускорения здесь может быть использовано бинарное представление списка экспортируемых правил
    
###########################################################
## список интерпретируется, результирующие строки конкатенируются: stage2 = ''.join([exec(x) for x in stage1])
