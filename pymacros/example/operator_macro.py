1*2+3

sym_names = {
    '|': 'I',
    '&': 'amp',
    '.': 'dot',
    '^': 'pow',
    '_': 'u',
    '@': 'a',
    '[': 'lb',
    ']': 'rb',
    '(': 'lp',
    ')': 'rp',
    '{': 'lf',
    '}': 'rf',
    '>': 'gt',
    '<': 'lt',
    '=': 'eq',
    '%': 'mod',
    '/': 'div',
    '+': 'plus',
    '-': 'minus',
    '*': 'mul',
    '!': 'not',
    '~': 'neg',
    ':': 'colon',
    ';': 'semicolon',
    '?': 'q',
    ',': 'comma'
}


class GrammarError(Exception): pass


builtin_bin = {'+','-','*','/','//','%','|','&','@','^',
               ',','.','^',':',';',
               '=','!=','<','>','<=','>=',
               '<<','>>',
               '+=','-=','*=','/=','//=','%=','@=','|=','&=','^=',
               '>>=','<<='}
infix_symbols = set('<=>+-*/%~?@|!&^.:;')


def make_op_expand(f):
    def expand(*args):
        arglist = args[-1]
        for arg in args[-2::-1]:
            arglist = arglist`${arg}, ${arglist}`
        return `{f.__name__}(${arglist})`
    expand.__name__ = f'make_op_expand({f.__name__})'
    return expand


def decl_infix_operator(op, lpr, rpr):
    def set_func(f):
        parse_context().add_macro_rule('expr', ['expr', f"'{op}'", 'expr'], make_op_expand(f), export=True, lpriority=lpr, rpriority=rpr)
        return f
    return set_func

#set_cpp_debug(0x20)

new_token('operation', '[<=>+\\-*/%~?@|!&^.:;]+')

# Определим макрос для простого определения операций
# Следующее определение слишком упрощённое и лишь показывает идею,
# как примерно можно было бы писать такие макросы
defmacro infxl(root_stmt, 'infxl', '(', priority: expr, ')', op: *stringliteral, '(', x: *ident,',',y: *ident, ')',':', c:func_body_suite):
    op_name = eval(op)
    pr = parse_context().eval(priority)
    lpr = pr*2+1
    rpr = pr*2
    if any(not c in infix_symbols for c in op_name):
        raise GrammarError(f'invalid operator name {op_name}')
    if op_name in builtin_bin:
        raise GrammarError(f'operator name {op_name} conflicts with built-in operator')

    fnm = 'infix_'+'_'.join(sym_names[c] for c in op_name) # Каким-то способом генерируется уникальное имя
    return stmt`@decl_infix_operator({op}, {lpr}, {rpr})\ndef {fnm}({x}, {y}): $c`


defmacro infxl(root_stmt, 'infxl', '(', priority: expr, ')', x: *ident, op: *operation, y: *ident, ':', c:func_body_suite):
    op_name = op
    pr = parse_context().eval(priority)
    lpr = pr*2+1
    rpr = pr*2
    if any(not c in infix_symbols for c in op_name):
        raise GrammarError(f'invalid operator name {op_name}')
    if op_name in builtin_bin:
        raise GrammarError(f'operator name {op_name} conflicts with built-in operator')

    fnm = 'infix_'+'_'.join(sym_names[c] for c in op_name) # Каким-то способом генерируется уникальное имя
    return stmt`@decl_infix_operator({repr(op)}, {lpr}, {rpr})\ndef {fnm}({x}, {y}): $c`



#Упрощённое создание макросов для операторов. Здесь используется только что определённый макрос
infxl(0) '..' (b, e): return range(b, e+1)

infxl(0) b !! e: return range(b, e+1)

# Тестируем макросы
if __name__ == '__main__':
    print(list(0+1..10))
