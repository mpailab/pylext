import sys, time
from typing import List
from cfgexts.core.wrap import *
from cfgexts.core.wrap import ast_to_text as _ast_to_text

__parse_context__ = None

class CppError(Exception):
    pass

class ParseNode:
    def __init__(self, cnode):
        self.p = cnode
        inc_pn_num_refs(self.p)

    def __del__(self):
        dec_pn_num_refs(self.p)

    def num_children(self):
        return get_pn_num_children(self.p)

    def __getitem__(self, i: int):
        return ParseNode(get_pn_child(self.p, i))

    def __setitem__(self, i: int, value):
        set_pn_child(self.p, i, value.p)

    @property
    def children(self):
        return [self[i] for i in range(self.num_children())]

    @property
    def rule(self):
        return get_pn_rule(self.p)

    def __eq__(self, other):
        assert type(other) is ParseNode
        return pn_equal(self.p, other.p) != 0

    def __repr__(self):
        return 'ParseNode:\n'+ast_to_text(parse_context(), self)


class ParseContext:
    def __init__(self, syntax_file):
        self.px = new_python_context(1, syntax_file)
        self.syntax_rules = {}
        self.macro_rules = {}

    def syntax_function(self, rule):
        return self.syntax_rules.get(rule, None)

    def macro_function(self, rule):
        return self.macro_rules.get(rule, None)

    def __enter__(self):
        global __parse_context__
        if __parse_context__ is not None:
            raise Exception('Enter parse context when previous context not deleted')
        __parse_context__ = self
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        global __parse_context__
        __parse_context__ = None

    def __del__(self):
        del_python_context(self.px)

    def add_macro_rule(self, lhs: str, rhs, apply):
        rhs = ' '.join(str(x) for x in rhs)
        rule_id = add_rule(self.px, lhs, rhs)
        self.macro_rules[rule_id] = apply

    def add_syntax_rule(self, lhs: str, rhs, apply):
        rhs = ' '.join(str(x) for x in rhs)
        rule_id = add_rule(self.px, lhs, rhs)
        self.syntax_rules[rule_id] = apply


def parse_context() -> ParseContext:
    return __parse_context__


def macro_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_macro_rule(lhs, rhs, expand_func)
        return expand_func
    return set_func


def syntax_rule(lhs: str, rhs: list):
    def set_func(expand_func):
        if parse_context() is not None:
            parse_context().add_syntax_rule(lhs, rhs, expand_func)
        return expand_func
    return set_func


def quasiquote(ntname, str_list, tree_list: List[ParseNode]):
    assert len(str_list) == len(tree_list)+1
    px = parse_context()
    nn = c_quasiquote(px.px, ntname, len(str_list), str_list, [t.p for t in tree_list])
    return ParseNode(nn)


class Parser:
    def __init__(self, px: ParseContext, text: str):
        self.px = px
        print(f'create parser state , px={px}, type = {type(px)}', end='...  ')
        self.state = new_parser_state(px.px, text, "")
        print('done')

    def __del__(self):
        if hasattr(self, 'state'):
            del_parser_state(self.state)

    def __iter__(self):
        return self

    def __next__(self):
        try:
            node = continue_parse(self.state)
            if not node:
                raise StopIteration
            return ParseNode(node)
        except CppError as e:
            if e.args[0]:
                raise
        raise StopIteration


def parse_gen(px, text):
    print(f'px = {px}')
    for node in Parser(px, text):
        # print('yield node')
        yield node


def syn_expand(node: ParseNode):
    px = parse_context()
    f = px.syntax_function(node.rule)
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function for rule {node.rule}')
    return f(*node.children) if f else node


def macro_expand(px: ParseContext, node: ParseNode):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule)
        if f is None:
            break
        node = f(*node.children)

    for i in range(len(node.children)):
        node[i] = macro_expand(px, node[i])
    return node


def ast_to_text(px: ParseContext, ast: ParseNode):
    """ Преобразует синтаксическое дерево в текст """
    return _ast_to_text(px.px, ast.p)


def load_file(text, globals, syntax_file):
    """ Читает файл, раскрывыая макросы и сразу выполняя раскрытые блоки """
    expanded = ""
    with ParseContext(syntax_file) as px:
        for stmt_ast in parse_gen(px, text):
            stmt_ast = macro_expand(px, stmt_ast)
            stmt = ast_to_text(px, stmt_ast)
            # print(f'=========================\nexecute:\n{stmt}\n--- Output: ---')
            expanded += stmt
            exec(stmt, globals, globals)
            # print('==========================')
    return expanded


with open(sys.argv[1]) as f:
    text = ''.join(f)

module_vars = {
    '__name__'    : '__main__',
    'syntax_rule' : syntax_rule,
    'macro_rule'  : macro_rule,
    'syn_expand'  : syn_expand,
    'quasiquote'  : quasiquote
}
start = time.time()
res = load_file(text, module_vars, sys.argv[2])
end = time.time()

print("Time of load_file :", end - start)
print(f'\n=========== Vars of macro module ===========')
for k, v in module_vars.items():
    if k != '__builtins__':
        print(f'{k} = {v}')
print(f'============================================')

print(f'result size = {len(res)}')

with open('output.py', 'w') as f:
    f.write('def syntax_rule(*args): return (lambda x: x)\n')
    f.write('def macro_rule(*args): return (lambda x: x)\n')
    f.write(res)

start = time.time()
import output
end = time.time()

print(f"Time of import output file: {end - start:.3f}")
