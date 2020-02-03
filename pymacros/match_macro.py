# coding=pymacro
# Вспомогательные правила для макроса match. Они не подставляются на место кода,
#   а лишь запоминаются в каком-то виде в дереве разбора
# Запоминаем их в таком виде, чтобы удобнее было использовать
syntax(matchcase, INDENTED, pattern: expr, ':', action: suite):
	return (pattern, test(`true`), action)
syntax(matchcase, INDENTED, pattern: expr, 'if', cond:test, ':', action: suite):
	return (pattern,cond,action)
syntax(matchcases, x:matchcase):
	return [x]
syntax(matchcases, xs:matchcases, x:matchcase):
	xs.append(x)
	return xs
	
#Максимально упрощённый pattern-matching (просто сравнение на равенство)
def match2cmp(x,pat):
	if tmatch($pat, expr(`_`)): # должна быть встроенная функция, сравнивающая деревья разбора
		return expr(`true`)
	return expr(`$x == $pat`)
		
macro(stmt, 'match', x:expr, ':', INDENT, mc:matchcases, DEDENT):
	conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in matchcases]
	head = if_stmt_no_else(`if $(conds[0][0]) and $(conds[0][1]): $(conds[0][2])`)
	for (c, cond, s) in conds[1:]:
		head = if_stmt_no_else(`$head $$INDENTED elif $c and $cond: $s`)
	return head

# Определим макрос для простого определения операций
# Следующее определение слишком упрощённое и лишь показывает идею,
# как примерно можно было бы писать такие макросы
macro(stmt, 'operator', '(', args:var_or_op_list, priority:integer,')',':', c:suite):
	vars = [v in args if head(v) is ident] # Вытаскиваем переменные
	fnm = create_name(args) # Каким-то способом генерируется уникальное имя 
	fdef = stmt(`def $fnm($vars): $c`)
	mdef = stmt(`macro(expr, $..args): return $fnm($vars)
	return node(stmts, [fnm, mdef]) # Создаём узел дерева напрямую, без квазицитирования

#Упрощённое создание макросов для операторов. Здесь используется только что определённый макрос
operator(b, '..', e, 0): return range(b,e+1)

#Вводим новый перегружаемый оператор логическое или (как в C++)
# В отличие от встроенного or его можно перегружать, определив функцию __or__
operator(b, '||', e, 1): 
	if hasattr(b,'__or__'):
		return b.__or__(e)
	else: return b or e

# Тестируем макросы
if __name__ == '__main__':
	for x in 0 .. 10:
		match x:
			1: print(f'{x} -> a')
			2: print(f'{x} -> b')
			_ if x<5 || x>8: print(f'{x} -> c')
			_: print(f'{x} -> d')
