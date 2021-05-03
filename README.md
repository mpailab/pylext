# PYLEXT: PYthon Language EXTension library

This library allows to add new syntax into Python language. 
It is based on LR(1) algorithm implementation that allows dynamically
add new rules and tokens into grammar ([see parser description](pylext/core/README.md)).

Macro extension system works in two stages: parsing text and 
expanding macros, i.e. transforming syntax tree from extended 
grammar to standard python grammar.
This procedure is applied to each statement in file separately, 
so it is possible to define new syntax and use it in the next statement.   

## Requirements

1. C++ compiler supporting c++17: 
   - Visual Studio 2019
   - gcc 8 or later
   - apple clang 11 or later
2. CMake 3.8 or later
2. Python >= 3.6. Recommended is Python 3.8. Packages:
   - cython
   - scikit-build
   - setuptools

## Installation:

When package will be available on pypi.org: 
```shell
pip install pylext
```
Currently only available installation using setup.py
```shell
python setup.py install
```

## Simple examples
### Custom operators
The simplest syntax extension is a new operator. For example we want to define 
left-assotiative operator /@
that applies function to each element of collection and this operator has lowest priority.
Than we should create file simple.pyg 

```python
# Import syntax extension for new operator definition
gimport pylext.macros.operator 

infixl(0) '/@' (f, data):
    if hasattr(f, '__vcall__'):       # check whether defined magic method f.__vcall__
       return f.__vcall__(data)
    if hasattr(data, '__vapply__'):   # check whether defined magic method data.__vapply__
       return data.__vapply__(f)
    return [f(x) for x in data]       # default implementation

# Test new operator:
from math import *
def test(n):
   print(exp /@ range(n))
```
Main file should be a python file, so we create main.py:
```python
import simple
simple.test(10)
```

Custom operators may be useful as a syntactic sugar for
symbolic computations libraries such as SymPy or SageMath.

### Function literals for binary operators
Sometimes we need to use binary operator as a function object, for example if we want
to reduce array using some binary operation. 
```python
# define new literal
new_token('op_lambda', '[(][<=>+\\-*/%~?@|!&\\^.:;]+[)]')

defmacro op_lambda(expr, op:*op_lambda):
    op = op[1:-1]  # remove parentheses
    try:
        return `(lambda x, y: x {op} y)`
    except RuntimeError as e:  # excetion will be thrown if op is not binary operator
        pass
    raise Exception(f'`{op}` is not a binary operator')
```
This simple macro for each binary operator
`op` creates function literal `(op)` which represents lambda function
`lambda x, y: x op y`.

After macros expansion these 2 lines will be equivalent:
```python
reduce((^), range(100))
reduce(lambda x,y: x ^ y, range(100))
```

We can write test function checking that result is the same
```python
def test():
    from functools import reduce
    result  = reduce((^), range(100))  # reduce array by XOR operation
    correct = reduce(lambda x, y: x ^ y, range(100))
    return result == correct
```


### Guard operator
Sometimes function checks a lot of conditions, and returns false of any condition is false.
In this case some part of function has form:
```python
if not <cond1>: return False
do_something()
if not <cond2>: return False
do_something()
...
if not <condN>: return False
do_something()
return True
```
We can define simple macro to simplify these repeated statements.
Create file `guard.pyg`:
```python
# define guard macro
defmacro guard(stmt, 'guard', cond: test, EOL):
    return stmt`if not (${test}): return False\n`
```
Now instead of writing `if not <expr>: return False` we can simply write `guard <expr>`.
This allow write some functions in more declarative style. For example, we write solver for solve some task and 
solution is a sequence of transformations of input data. Each transformation has some conditions when it can be applied.
At each moment we select applicable transformation and apply it. 
For example, we want to find real roots of the polynomial, and there are simple rules for linear and quadratic equations:
```python
class Eqn:
   def __init__(self, coeffs):
      self.coeffs = coeffs
      while self.coeffs and self.coeffs[-1]==0:
         del self.coeffs[-1]
      self.roots = None
   @property
   def degree(self):
       return len(self.coeffs)

def no_solution(eqn: Eqn):
   guard eqn.degree == 0
   eqn.roots = []

def solve_linear(eqn: Eqn):
   guard eqn.degree == 1
   eqn.roots = [eqn.coeffs[0]/eqn.coeffs[1]]
    
def solve_quadratic(eqn: Eqn):
   guard eqn.degree == 2
   c, b, a = eqn.coeffs 
   d = b*b - 4*a*c
   guard d >= 0
   eqn.roots = [(-b-d**0.5)/2/a, (-b+d**0.5)/2/a]
```

## Library usage
To activate library, add command to main file:
```python
import pylext
```
When pylext loads, it adds new importer for python with extended grammar.
These files should have **.pyg** extension. New syntax can be defined and
used only in *pyg* files. One pyg file can import syntax defined 
in another *pyg* file using `gimport` command.
*pyg* files can be imported from *py* files using standard `import` command. 
When *pyg* file is imported with import command, actually loaded python code 
obtained by expansion of all macros from this *pyg* file. 

### New operator definition
New infix operation may be defined using `infixl` or `infixr` syntax construction defined in operator.pyg module.
First pylext/macros/operator.pyg should be imported:
```python
gimport pylext.macros.operator
```
Syntax of new left-assotiative operator definition is one of following:
```python
'infixl' '(' priority ')' op_name '(' x ',' y ')' ':' func
'infixl' '(' priority ')' op_name '(' x ',' y ')' '=' func_name
```
In first definition assotiated function for new operator is implemented inside infixl macro.
Second definition allows to assotiate with new operator existing function.

Similarly, right-assotiative operator may be defined:
```python
'infixr' '(' priority: expr ')' op_name '(' x ',' y ')' ':' func
'infixr' '(' priority: expr ')' op_name '(' x ',' y ')' '=' func_name
```
Also it is possible to set custom left and right operation priorities
```python
'infix' '(' lpriority ',' rpriority ')' op_name '(' x ',' y ')' ':' func
'infix' '(' lpriority ',' rpriority ')' op_name '(' x ',' y ')' '=' func_name
```
Macro parameter description:
- **priority** -- expression evaluating to priority of defined operator. Usually a constant number.
- **lpriority** -- expression evaluating to left priority of defined operator. Usually a constant number.
- **rpriority** -- expression evaluating to right priority of defined operator. Usually a constant number.
- **op_name** -- string literal, name of defined operator enclosed in quotes. Must consist of symbols from this set: `<=>+-*/%~?@|!&^.:;`
- **x**, **y** -- identifiers, arguments passed to function
- **func** -- definition of function assigned to new operator. 
  After macro expansion function definition becomes `def f(x,y): func`.
- **func_name** -- name of function assigned with new operator.

#### Built-in operation priorities
Following table contains priorities of built-in operators.

| operator | priority | assotiativity   |
|:---------|:--------:|:---------------:|
| unary `+`, `-`, `~` | 100  |          |
| `**`       | 70       |  right        |
| `*`, `@`, `/`, `%`, `//` | 60 |  left |
| `+`, `-`   | 50       |  left         |
| `<<`, `>>` | 40       |  left         |
| `&`        | 30       |  left         |
| `^`        | 20       |  left         |
| `\|`       | 10       |  left         |

Note that in current grammar definition comparison and logical operators don't have priorities 
and defined via another nonterminals. They all have priority lower that those that can be defined using this macro. 

### New syntax definition
Usually to define a complex macro it is needed to have a lot of additional rules 
that can be expanded using **syn_expand** function.
Rule `A -> A1 A2 ... AN` is written as comma separated list `A, V1, ..., VN` where
each element `Vi` has one of following format:
- string literal, in this case Ai is terminal equal to value of this literal
- `x : Ai` where x and **Ai** is identifier, **Ai** is terminal or nonterminal name. 
    If no terminal with name **Ai**, then **Ai** is considered as new nonterminal. 
   - If **Ai** is nonterminal, then **x** is name of variable representing parse tree for nonterminal **Ai**.
   - If **Ai** is terminal name, then **x** is name of variable containing token of type **Ai**.
- `x: *Ai` where **x** is identifier, **Ai** is terminal or nonterminal name. If no terminal with name **Ai**, then
  Ai is considered as new nonterminal. 
  - If **Ai** is nonterminal, then `x = syn_expand(t)` where `t` parse tree for nonterminal **Ai**.
  - If **Ai** is terminal name, then **x** is string content of token of type **Ai**.

All auxilliary rules are defined using following syntax:
```python
'syntax' '(' rule ')' ':' definition 
```
where 
- **rule** is grammar rule in format described above,
- **definition** describes transformation of parse tree into some more convenient python object

Macro is defined using similar syntax:
```python
'defmacro' name: ident '(' rule ')' ':' definition
```
where
- **name** is macro name that can be used for debug purposes,
- **rule** is grammar rule in format described above,
- **definition** describes transformation of parse tree into another parse tree. Important that **definition** 
  must return **ParseNode** object. 

### New token definition 
Lexer in pylext is based on packrat parser for PEG. 
parsing expressions are used instead of regular expressions to define tokens.
Every non-constant token in language is described by PEG nonterminal. 

New terminals (tokens) may be defined by command:
```python
new_token(x, peg_expr)
```
New auxilliary lexer rules may be defined by command:
```python
new_lexer_rule(x, peg_expr)
```
where
- **x** is string, name of new terminal,
- **peg_expr** is string containing parsing expression describing new token.

When new auxilliary lexer rule is defined, left side doesn't become a token, it can be used only in
other lexer rule definitions.

Parsing expression syntax is following:
1. Atomic expressions:

   | syntax           | description                   |
   |:-----------------|:------------------------------|
   | `[<symbols>]`    | any of listed symbols**       |
   | `[^<symbols>]`   | any symbol except listed symbols**       |
   | `'seq'`, `"seq"` | sequence of symbols in quotes |
   |  identifier T | token of type T, T is nonterminal of PEG |
   **   There are some special symbols in `<symbols>`: 
   - `x-y` means any symbol from `x` to `y`
   - escape sequences `\n`, `\r`, `\t`, `\\`, `\]`, `\-`

2. Combine expressions 

   | syntax           | description                   |
   |:-----------------|:------------------------------|
   | `!e`             | negative lookahead for expression e |
   | `&e`             | positive lookahead for expression e |
   | `(e)`            | parentheses around subexpression    |
   | `e?`             | optional                            |
   | `e*`             | zero or more                        |
   | `e+`             | one or more                         |
   | `e1 e2`          | sequence: e1, then e2               |
   | `e1 / e2`        | ordered choise: e1 or (!e1 e2)      |

NOTE: direct or indirect left recursion in PEG rules currently not supported

## How does it work

PyLExt extension adds new importer for .pyg files. PyLExt uses parser independent of python's parser,
 here it is called *pylext parser*.

### Pyg importing procedure
1. Create pylext parser context `ctx` containing current macro and syntax definitions and some another metadata.
2. Load text from file and initialize pylext parser in `ctx` context. 
   Parser is always initialized with python grammar and built-in pylext syntax extensions
   allowing to add new rules.
3. Initialize module object `M` and add pylext built-ins into `M.__dict__`.
4. Pylext parser reads text statement by statement. At each step it can do one of the following actions:
   - return parse tree for next statement
   - return NULL which means end of file
   - throw exception if there is syntax error
   
   For each statement parse tree S we do:
   1. `S <- macro_expand(ctx, S)` -- expanding all macros in `S` using rules from context `ctx`
   2. `expanded <- ast_to_text(ctx, S)` -- convert S back to text
   3. `exec(expanded, M.__dict__)` -- execute expanded statement in context of module M
5. Generate definition of function `_import_grammar(module_name)` from current parse context
   and load it into interpreter:
   ```python
   syn_gimport_func = px.gen_syntax_import()
   exec(stmt, M.__dict__)
   ```
   Function adds all grammar defined in this module and in all 
   gimported modules into current context. 
   This function is called when module is imported using `gimport` command.

### Macro expansion procedure
All macros have 2 types: built-in and user-defined. Built-in macros expanded at moment 
when corresponding parse tree node is created. User-defined macros are expanded in **macro_expand** function.

#### Built-in macros
There are following built-in macros for basic support of macro extension system
1. `defmacro`, `syntax` described in [new syntax definition section](###new-syntax-definition)
2. Quasiquotes. Quasiquote syntax is following:
   ```python
   `<quoted expr>`
   <nt_name>`<quoted nonterminal nt>`
   ```
   where 
   - *quoted expr* can be parsed as expression. It allows f string-like braced expressions and
     in addition it allows inserting parse subtrees using `${<expr>}` where expr evaluates to ParseNode object. 
   - *nt_name* is nonterminal name (identifier)
   - *quoted nt* can be parsed as nonterminal **nt** after substitution of all subtrees marked as `${<expr>}`.
3. gimport macro. It allows to use syntax similar to `import` statement but import module together with grammar
   defined in that module.
   It allows 2 forms:
   ```python
   gimport <module name>
   gimport <module name> as <name> 
   ```

#### User-defined macros
Each macro is assotiated with some syntax rule. Function **expand_macro** search in parse tree nodes with rules 
assotiated with user-defined macros. Search order is depth-first, starting from root. 
If in some node macro rule is detected, then it is expanded using function from macro definition. 

## More examples

### Simple match operator
This example is match operator that is an analog of C++ switch operator. Actually, 
python 3.10 already has [more powerful match operator](https://docs.python.org/3.10/whatsnew/3.10.html#pep-634-structural-pattern-matching)
but here is example how simple version may be defined using pylext.

File match.pyg:
```python
syntax(matchcase, pattern: expr, ':', action: suite):
    return (pattern, None, action)

syntax(matchcase, pattern: expr, 'if', cond:test, ':', action: suite):
    return (pattern,cond,action)

syntax(matchcases, x:*matchcase):
    return [x]

syntax(matchcases, xs:*matchcases, x:*matchcase):
    if type(xs) is not list:
        xs = [xs]
    xs.append(x)
    return xs

def match2cmp(x, pat):
    if pat == `_`:
        return None
    return `($x == $pat)`

def make_and(c1, c2):
    if c1 is None: return c2
    if c2 is None: return c1
    return `(($c1) and ($c2))`

defmacro match(stmt, 'match', x:expr, ':', EOL, INDENT, mc:*matchcases, DEDENT):
    if type(mc) is not list:
        mc=[mc]
    conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in mc]
    condexpr = make_and(conds[0][0],conds[0][1])
    if condexpr is None:
        return conds[0][2]
    head = stmt`if $condexpr: ${conds[0][2]}`
    for (c, cond, s) in conds[1:]:
        condexpr = make_and(c, cond)
        if condexpr is None:
            head = stmt`$head else: $s`
            break
        else:
            head = stmt`$head elif $condexpr: $s`
    return head
```

For example, it can be used for simple data serialization function:
```python
def serialize_match(x):
    match type(x):
        int:       return b'i' + struct.pack('<q', x)
        bool:      return b'b' + struct.pack('<q', int(x))
        float:     return b'f' + struct.pack('<d', float(x))
        bytes:     return b'b' + struct.pack('<q', len(x)) + x
        str:       return b's' + serialize_match(x.encode('utf8'))
        list:
            data = b'l' + struct.pack('<q', len(x))
            for elem in x:
                data += serialize_match(elem)
            return data
        _:         raise Exception(f'Unsupported type {type(x)}')
```
After macro expansion it is equivalent to implementation using if-elif-else construction available in python 3.8:
```python
def serialize(x):
    if type(x) == int:
        return b'i' + struct.pack('<q', x)
    elif type(x) == bool:
        return b'b' + struct.pack('<q', int(x))
    elif type(x) == float:
        return b'f' + struct.pack('<d', float(x))
    elif type(x) == bytes:
        return b'b' + struct.pack('<q', len(x)) + x
    elif type(x) == str:
        return b's' + serialize(x.encode('utf8'))
    elif type(x) == list:
        data = b'l' + struct.pack('<q', len(x))
        for elem in x:
            data += serialize(elem)
        return data
    else:
        raise Exception(f'Unsupported type {type(x)}')
```

### Short lambda functions
We can define operator `->` creating lambda functions. In this case we cannot simply define it
as a function, we need to convert `x -> f(x)` to `(lambda x: f(x))`. Sinse x can be 
arbitrary expression we also must check that it is a variable or tuple of variables. 
```python
star_rule  = get_rule_id('star_expr', "'*' expr")
ident_rule = get_rule_id('atom', 'ident')
tuple_rule = get_rule_id('atom', "'(' testlist_comp ')'")
inside_tuple_rule = get_rule_id('identd_or_star_list', "identd_or_star_list ',' identd_or_star")

def check_arg_list(x):  
   """ checking that tree represents valid argument list """
   if x.rule() == inside_tuple_rule:
      check_arg_list(x[0])
      check_arg_list(x[1])
   elif x.rule() != ident_rule and (x.rule() != star_rule or x[0].rule() != ident_rule):
      raise Exception(f'Invalid function argument {parse_context().ast_to_text(x)}')

infix_macro(101, 1) '->' (x, y):
   args = ''
   if x.rule() == tuple_rule:
      x = x[0]  # remove parentheses
   check_arg_list(x)
   # lambda arg list and expr list are formally different nonterminals.
   # simplest convertion is: ast -> text -> ast
   args = parse_context().ast_to_text(x)
   return `(lambda {args}: $y)`
```
Here we defined operator `->` with highest left priority and lowest right priority. 
This means that left side must be atomic expression like a variable or tuple, 
right side can contain arbitrary operations except `or`, `and` and `not`. 
Also this allows to combine `->` operators in a chain to produce lambda returning 
lambda functions.

We can also define `<$>` operator similar to Haskell `fmap` operator and our `/@` 
operator from first example:
```python
infixl(40) '<$>' (f, data):
    return type(data)(f(x) for x in data)
```
Now we can test it together:
```python
print((x -> y -> x**y) <$> (2,1,3) <$> [1,4])
```