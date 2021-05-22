
**PyLExt** allows you to add the following constructions to the Python language:
- new operators with specified priorities,
- syntax extensions given by [LR(1)](https://en.wikipedia.org/wiki/Canonical_LR_parser) grammars,
- macros associated with new syntax rules,
- new token types and literals.

Language extension system works in two stages: parsing text and 
expanding macros, i.e. transforming syntax tree from extended 
grammar to standard Python grammar.
This procedure is applied to each statement in file separately, 
so it is possible to define new syntax and use it in the next statement. 

## Simple examples
### Custom operators
The simplest syntax extension is a new operator. For example, we want to define 
left-associative operator /@
that applies function to each element of collection and this operator has the lowest priority.
Than we should create file simple.pyg 

```python
# Import syntax extension for new operator definition
gimport pylext.macros.operator 

# Add new operator 
infixl(0) '/@' (f, data):
    return [f(x) for x in data]

# Test new operator:
from math import *
def test(n):
   print(exp /@ range(n))
```
Main file should be a Python file, so we create main.py:
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
new_token('op_lambda', '"(" [<=>+\\-*/%~?@|!&\\^.:;]+ ")"')

defmacro op_lambda(expr, op:*op_lambda):
    op = op[1:-1]  # remove parentheses
    try:
        return `(lambda x, y: x {op} y)`
    except RuntimeError as e:  # exception will be thrown if op is not binary operator
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

See more examples and the documentation at [github](https://github.com/mpailab/pylext).

## Requirements

1. C++ compiler supporting c++17: 
   - Visual Studio 2019
   - gcc 8 or later
   - apple clang 11 or later
2. CMake 3.8 or later
3. Python >= 3.6. Recommended is Python 3.8.
4. Package python3-dev (for Ubuntu)

## Installation:

You can install pylext from PyPI using pip:
```shell
$ pip install pylext
```
