# PYLEXT: PYthon Language EXTension library

This library allows to add new syntax into Python language. 
It is based on LR(1) algorithm implementation that allows dynamically
add new rules and tokens into grammar ([see parser description](pylext/core/README.md)).

Macro extension system works in two stages: parsing text and 
expanding macros, i.e. transforming syntax tree from extended 
grammar to standard python grammar.
This procedure is applied to each statement in file separately, 
tso it is possible to define new syntax and use it in the next statement.   

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

### function literals for binary operators
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
This allow write some functions in more declarative style.

## Library usage
To activate library, add command to main file:
```python
import pylext
```
When pylext loads, it adds new importer for python with extended grammar.
These files should have .pyg extension. New syntax can be defined and
used only in .pyg files. One .pyg file can import syntax defined 
in another .pyg file using `gimport` command.
.pyg files can be imported from .py files using standard `import` command.

## More examples

### Simple match operator
This example is match operator that is an analog of C++ switch operator.

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