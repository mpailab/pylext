# coding=pymacro
# Вспомогательные правила для макроса match. 
# Они используются при преобразовании узла дерева в питоновский объект функцией syn_expand:
# Если узел соответствует правилу, объявленному с ключевым словом syntax, то он раскрывается описанной функцией
# функция syn_expand применяется к аргументам макроса и других синтаксических расширений, помеченных символом *
syntax(matchcase, INDENTED, pattern: expr, ':', action: suite):
	return (pattern, test`True`, action)
syntax(matchcase, INDENTED, pattern: expr, 'if', cond:test, ':', action: suite):
	return (pattern,cond,action)
syntax(matchcases, x:*matchcase): # здесь к x применяется syn_expand
	return [x]
syntax(matchcases, xs:*matchcases, x:*matchcase): # здесь к xs и x применяется syn_expand
	xs.append(x)
	return xs
	
#Максимально упрощённый pattern-matching (просто сравнение на равенство)
def match2cmp(x, pat):
	if tmatch(pat, expr`_`): # должна быть встроенная функция, сравнивающая деревья разбора
		return expr`True`
	return expr`$x == $pat`
		
defmacro match(stmt, 'match', x:expr, ':', INDENT, mc:*matchcases, DEDENT):
	conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in mс]
	head = if_stmt_no_else`if ${conds[0][0]} and ${conds[0][1]}: ${suite:conds[0][2]}`
	for (c, cond, s) in conds[1:]:
		head = if_stmt_no_else`${if_stmt_no_else:head} $$INDENTED elif $c and $cond: ${suite:s}`
	return head

# Тестируем макрос
if __name__ == '__main__':
	for x in range(10):
		match x:
			1: print(f'{x} -> a')
			2: print(f'{x} -> b')
			_ if x<5 or x>8: print(f'{x} -> c')
			_: print(f'{x} -> d')
