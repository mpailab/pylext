# coding=pymacro
# Вспомогательные правила для макроса match. 
# Они используются при преобразовании узла дерева в питоновский объект функцией syn_expand:
# Если узел соответствует правилу, объявленному с ключевым словом syntax, то он раскрывается описанной функцией
# функция syn_expand применяется к аргументам макроса и других синтаксических расширений, помеченных символом *
syntax(matchcase, INDENTED , pattern: expr, ':', action: suite):
    print(f'in matchcase 0')
    return (pattern, `True`, action)
syntax(matchcase, INDENTED, pattern: expr, 'if', cond:test, ':', action: suite):
    print(f'in matchcase 1')
    return (pattern,cond,action)
syntax(matchcases, x:*matchcase): # здесь к x применяется syn_expand
    print(f'x = {x}')
    return [x]
syntax(matchcases, xs:*matchcases, x:*matchcase): # здесь к xs и x применяется syn_expand
    if type(xs) is not list:
        xs = [xs]
    print(f'xs = {xs}, x = {x}')
    xs.append(x)
    return xs

#Максимально упрощённый pattern-matching (просто сравнение на равенство)
def match2cmp(x, pat):
    print('in match2cmp')
    if pat == `_`: # должна быть встроенная функция, сравнивающая деревья разбора
        return `True`
    return comparison`${x} == ${pat}`

defmacro match(stmt, 'match', x:expr, ':', INDENT, mc:*matchcases, DEDENT):
    print(f'in match expand, mc = {mc}')
    conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in mc]
    print(f'conds[0][0] = {ast_to_text(parse_context(), conds[0][0])}')
    head = if_stmt_noelse`if ${conds[0][0]} and ${conds[0][1]}: ${conds[0][2]}`
    print(2)
    for (c, cond, s) in conds[1:]:
        head = if_stmt_noelse`${head} $$INDENTED elif ${c} and ${cond}: ${s}`
    return head

# Тестируем макрос
if __name__ == '__main__':
    for x in range(10):
        match x:
            1: print(f'{x} -> a')
            2: print(f'{x} -> b')
            _ if x<5 or x>8: print(f'{x} -> c')
            _: print(f'{x} -> d')
