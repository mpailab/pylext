# PyLExt: PYthon Language EXTension library

This library allows to add new syntax into Python language. 
It is based on LR(1) algorithm implementation that allows dynamically
add new rules and tokens into grammar ([see parser description](pylext/core/README.md)).

Macro extension system works in two stages: parsing text and 
expanding macros, i.e. transforming syntax tree from extended 
grammar to standard Python grammar.
This procedure is applied to each statement in file separately, 
so it is possible to define new syntax and use it in the next statement.   

## Requirements

1. C++ compiler supporting c++17: 
   - Visual Studio 2019
   - gcc 8 or later
   - apple clang 11 or later
2. CMake 3.8 or later
3. Python >= 3.6. Recommended is Python 3.8.
4. Package python3-dev (for Ubuntu)

## Installation:

When package will be available on pypi.org: 
```shell
$ pip install pylext
```
Currently, only available installation using setup.py
```shell
$ python setup.py install
```

## Simple examples
### Custom operators
The simplest syntax extension is a new operator. For example, we want to define 
left-associative operator /@
that applies function to each element of collection and this operator has the lowest priority.
Then we should create file simple.pyg 

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
    return stmt`if not (${cond}): return False\n`
```
Now instead of writing `if not <expr>: return False` we can simply write `guard <expr>`.
This allows to write some functions in more declarative style. For example, we write solver for solve some task and 
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
       return len(self.coeffs) - 1

def no_solution(eqn: Eqn):
   guard eqn.degree == 0
   eqn.roots = []

def solve_linear(eqn: Eqn):
   guard eqn.degree == 1
   eqn.roots = [-eqn.coeffs[0]/eqn.coeffs[1]]
    
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
When pylext loads, it adds new importer for Python with extended grammar.
These files should have **.pyg** extension and UTF-8 encoding. New syntax can be defined and
used only in *pyg* files. One pyg file can import syntax defined 
in another *pyg* file using `gimport` command.
*pyg* files can be imported from *py* files using standard `import` command. 
When *pyg* file is imported with import command, actually loaded Python code 
obtained by expansion of all macros from this *pyg* file.

### New operator definition
New infix operation may be defined using `infixl` or `infixr` syntax construction defined in operator.pyg module.
First pylext/macros/operator.pyg should be imported:
```python
gimport pylext.macros.operator
```
Syntax of new left-associative operator definition is one of following:
```python
'infixl' '(' priority ')' op_name '(' x ',' y ')' ':' func
'infixl' '(' priority ')' op_name '(' x ',' y ')' '=' func_name
```
In first definition associated function for new operator is implemented inside infixl macro.
Second definition allows associating with new operator existing function.

Similarly, right-associative operator may be defined:
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

#### Example:
```python
infixl(0) '/@' (f, data):
    return [f(x) for x in data]
```
Here defined left-associative operator `/@` with priority 0, i.e. the lowest possible priority.
This is equivalent to following definition:
```python
def list_map(f, data):
    return [f(x) for x in data]
infixl(0) '/@' (f, data) = list_map
```
In this case we first define function, then operator. It is useful if function is recursive because
new operator cannot be used inside its own definition.

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

Note that in current grammar definition comparison and logical operators are defined via another nonterminals (not expr). 
So, they all have priority lower that those that can be defined using this `infix` macros. 

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

All auxiliary rules are defined using following syntax:
```python
'syntax' '(' rule ')' ':' definition 
```
where 
- **rule** is grammar rule in format described above,
- **definition** describes transformation of parse tree into some more convenient Python object

Macro is defined using similar syntax:
```python
'defmacro' name: ident '(' rule ')' ':' definition
```
where
- **name** is macro name that can be used for debug purposes,
- **rule** is grammar rule in format described above,
- **definition** describes transformation of parse tree into another parse tree. Important that **definition** 
  must return **ParseNode** object. 

### Quasiquotes
To define a macro it is necessary to build new parse trees. The only correct way to do this is to use quasiquotes.
Quasiquote allow correctly constructing new syntax tree with given subtrees. Quasiquote syntax is the following:
1. `<quoted expr>` -- quasiquote for expr
2. ``<nt_name>`<quoted nonterminal nt>` `` -- quasiquote for nonterminal with name **nt_name**.

where
- *quoted expr* can be parsed as expression. It allows f string-like braced expressions and
  in addition it allows inserting parse subtrees using `${<expr>}` where expr evaluates to ParseNode object.
- *nt_name* is nonterminal name (identifier)
- *quoted nt* can be parsed as nonterminal **nt** after substitution of all subtrees marked as `${<expr>}`.

#### Example
Guard macro from [simple examples](#simple-examples) uses this syntax:
```python
defmacro guard(stmt, 'guard', cond: test, EOL):
    return stmt`if not (${cond}): return False\n`
```
Here we defined macro associated with new grammar rule `stmt -> 'guard' test EOL` where
   - **test** -- nonterminal for logic expressions
   - **EOL** -- end of line token

Macro expansion here is single command that builds new statement `if not (cond): return False`. 
`\n` is needed because statement must end by EOL token. 

### New token definition 
Lexer in pylext is based on packrat parser for [PEG](https://en.wikipedia.org/wiki/Parsing_expression_grammar). 
parsing expressions are used instead of regular expressions to define tokens.
Every non-constant token in language is described by PEG nonterminal. 

New terminals (tokens) may be defined by command:
```python
new_token(x, peg_expr)
```
New auxiliary lexer rules may be defined by command:
```python
new_lexer_rule(x, peg_expr)
```
where
- **x** is string, name of new terminal,
- **peg_expr** is string containing parsing expression describing new token.

When new auxiliary lexer rule is defined, left side doesn't become a token, it can be used only in
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
   | `e1 / e2`        | ordered choice: e1 or (!e1 e2)      |

NOTE: direct or indirect left recursion in PEG rules currently not supported

#### Example
New token was defined in lambda literal example:
```python
new_token('op_lambda', '"(" [<=>+\\-*/%~?@|!&\\^.:;]+ ")"')
```
In this definition constant string "(", then one or more symbols from set `<=>+-*/%~?@|!&^.:;`, then constant string ")".
Symbols `-` and `^` are special inside `[ ]`, so they should be escaped.

### Importing macros from other modules

To import macros from another modules use `gimport` command.
Its syntax is similar to `import` statement, but it imports module together with grammar
defined in that module.
It is available in 2 forms:
```python
gimport <module name>
gimport <module name> as <name> 
```
#### Example
Importing macros `infixl`, `infixr` and `infix` for defining new operators:
```python
gimport pylext.macros.operator
```

## How does it work

PyLExt extension adds new importer for .pyg files. PyLExt uses parser independent of Python's parser,
 here it is called *pylext parser*.

### Pyg importing procedure
1. Create pylext parser context `ctx` containing current macro and syntax definitions and some another metadata.
2. Load text from file and initialize pylext parser in `ctx` context. 
   Parser is always initialized with Python grammar and built-in pylext syntax extensions
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
   exec(syn_gimport_func, M.__dict__)
   ```
   Function adds all grammar defined in this module and in all 
   gimported modules into current context. 
   This function is called when module is imported using `gimport` command.

### Macro expansion procedure
All macros have 2 types: built-in and user-defined. Built-in macros expanded at moment 
when corresponding parse tree node is created. User-defined macros are expanded in **macro_expand** function.

#### Built-in macros
There are following built-in macros for basic support of macro extension system
1. `defmacro`, `syntax` described in [new syntax definition section](#new-syntax-definition)
   This macro adds new syntax rule into grammar at the moment when ':' is read, 
   even before macro definition is parsed. 
   
   This macro converts code
   ```python
   defmacro name(lhs, <rhs args>): 
      definition
   ```
   into
   ```python
   @syntax_rule(lhs, rhs_names)
   def macro_name_<unique id>(<rhs_vars>):
      <syn_expand definitions>
      definition
   ```
   where 
   - **rhs args** -- list of defmacro arguments containing variables and its types (terminal and nonterminal names)
   - **rhs_names** is list of terminal and nonterminal names from rhs args
   - **rhs_vars** -- list of vars in **rhs args**
   - **unique id** -- unique number to make sure that expansion functions for different macros
     have different names
   - **syn_expand definitions** -- sequence of commands `x = syn_expand(x)`
     for all vars **x** occurred in **rhs args** in the form `x: *y` 
2. Quasiquotes. 
   Quasiquote `` `<quoted expr>` `` processed in the same way as `` expr`<quoted expr>` ``.
   In general quasiquote is 
   ``` 
   <nonterminal>` <some code with insertions of {<expr>[format]} and ${<expr>}> ` 
   ```
   Fragments `{...}` expanded in the same way as in f string. 
   For fragments `${expr}` it is assumed that, result of expr is parse tree, i.e. **ParseNode** object.
   This type of insertion after parsing become a leaf in parse tree, and then it is replaced by subtree
   which is result of expression **expr**.

   Technically expansion algorithm of quasiqoute ``[type]`s0 ${expr1} s1 ${expr2} s_2 ... ${exprN} s_N ` `` is following:
   1. First quasiquote is translated into function call `quasiquote(type, [f"""s0""", f"""s1""",..., f"""sN"""], [expr1, ..., exprN])`.
      Fragments `s0, s1, ..., sN` become f strings and automatically when it will be executed, all `{...}` fragments 
      will be expanded.
   2. When resulting code is executed, all fragments `{...}` in `f"""si"""` automatically substituted 
      by Python interpreter as in formatted string.
   3. Function `quasiquote(type, frags, subtrees)` works as follows: 
      - For each subtree `s[i]` in **subtrees** get root nonterminal type `nt[i]`.
      - Form string concatenation `code = f" {frags[0]} ${nt[0]} {frags[1]} ... ${nt[N-1]} {frags[N]}"`.
      - Parse string `code` into parse tree `tree`. For each nonterminal **nt** in grammar there is rule `nt -> '$nt'`
      - Traverse `tree` and replace node with i-th occurrence of rule `nt -> '$nt'` by `subtrees[i]`
      - Return resulting parse tree.
3. Importing grammar macro, command `gimport`. It allows to use syntax similar to `import` statement but import module together with grammar
   defined in that module. 
   1. `gimport M` is converted to function call `_gimport___('M', None, globals())`
   2. `gimport M as N` is converted to function call `_gimport___('M', 'N', globals())`
   Function `_gimport___` do following:
      - imports module M using standard *import*/*import as* command
      - registers imported module in variable `_imported_syntax_modules`
      - executes `_import_grammar` function of module M (if exists). Thus, all macro definitions from M and from all 
        modules imported from M added to current parse context.

#### User-defined macros
Each macro is associated with some syntax rule. Function **expand_macro** search in parse tree nodes with rules 
associated with user-defined macros. Search order is depth-first, starting from root. 
If in some node macro rule is detected, then it is expanded using function from macro definition:
1. While current node `curr` associated with macro
   - `f <- macro expansion function`
   - `curr <- f(*curr.children)`
2. For all children run **macro_expand**
### Examples
#### Expansion of guard macro definition
```python
defmacro guard(stmt, 'guard', cond: test, EOL):
    return stmt`if not (${cond}): return False\n`
```
Expansion of this definition consists of following steps:
- Add grammar rule `stmt -> 'guard' test EOL`
- Convert syntax tree of initial macro definition to Python syntax.

  Here right-hand part of rule contains 3 elements but only one is nonconstant, so
  macro expansion function variable `macro_guard_0` has one argument.
  
  Quasiquote `` stmt`if not (${test}): return False\n` `` doesn't contain `{...}` fragments, 
  only one `${...}` fragment. Hence, quasiquote content is split into parts `s0 = """if not ("""` and 
  `s1 = """): return False\n"""` which are simple strings, not f strings. The expansion result is:
  ```python
  quasiquote("stmt", [s0, s1], [cond])
  ```
  The whole statement is expanded to following:
   ```python
   @ macro_rule ( "stmt" , [ "'guard'" , "test" , "EOL" ] )
   def macro_guard_0 ( cond ) :
      return quasiquote ( "stmt" , [ """if not (""" , """): return False\n""" ] , [ cond ] )
   ```  

#### Expansion of lambda literals
Here is simplified version of lambda literal macro. Consider file op_lambda.pyg containing following code:
```python
new_token('op_lambda', '"(" [<=>+\\-*/%~?@|!&\\^.:;]+ ")"')

defmacro op_lambda(expr, op:*op_lambda):
    op = op[1:-1]  # remove parentheses
    return `(lambda x, y: x {op} y)`
 
print(reduce((^), range(100)))
```
1. First statement doesn't contain macros, preprocessing doesn't change it, when it executed,
new token **op_lambda** is added to grammar.

2. Second statement introduces new macro corresponding to grammar rule `expr -> op_lambda`.
   This rule is added into grammar and then **defmacro** is expanded.
   Key points in this macro definition is:
   - Right-hand side consists of one terminal `op_lambda`, so expansion function is decorated by 
      `@macro_rule("expr", ["op_lambda"])`.
   - Variable *op* was declared as `op: *op_lambda`, asterisk means that **syn_expand** should be applied to op.
   - Quasiquote `` `(lambda x, y: x {op} y)` `` doesn't have `${...}` fragments, so it is expanded to 
     `quasiquote("expr", [f"""(lambda x, y: x {op} y)"""], [])`

   As a result we have:
   ```python
   @ macro_rule ( "expr" , [ "op_lambda" ] )
   def macro_op_lambda_0 ( op ) :
      op = syn_expand ( op )
      op = op [ 1 : - 1 ]
      return quasiquote ( "expr" , [ f"""(lambda x, y: x {op} y)""" ] , [ ] )
   ```
   Then this code is executed and function `macro_op_lambda_0` is loaded in interpreter and decorator 
   `@macro_rule` associates it with new syntax rule `expr -> op_lambda`.
3. Last statement uses new macro. When macro_expand applied to node corresponding to token `(^)`, then 
   - it finds that rule `expr -> op_lambda` is macro with expansion function macro_op_lambda_0.
   - calls `macro_op_lambda_0('(^)')` which returns parse tree **T** for expression `( lambda x , y : x ^ y )`
   - replaces node `(^)` by tree **T**
   
   After macro_expand is finished, it parse tree converted back to text:
   ```python
   print ( reduce ( ( lambda x , y : x ^ y ) , range ( 100 ) ) )
   ```
   And then executed by interpreter
4. When file op_lambda.pyg is parsed, then `_import_grammar` function is generated. In this module 2 things were introduced:
   1. New token **op_lambda**.
   2. New macro associated with grammar rule `expr -> op_lambda`. 
   `_import_grammar` consists of following steps:
      - Get current context and check whether this module already was gimported into this context:
         ```python
         px = parse_context()
         if not px.import_new_module(_import_grammar): return
         ```
         In parse context there is a set of `_import_grammar` functions from all modules.
         `px.import_new_module` function adds current `_import_grammar` function to this set 
         and returns False if it is already in that set (in this case this function 
         already was executed in current context `px`) 
      - Recursively importing grammar from imported syntax modules:
        ```python
        if '_imported_syntax_modules' in globals():
           for sm in _imported_syntax_modules:
              if hasattr(sm, '_import_grammar'):
                  sm._import_grammar(px)
        ```
      - Import token defined in this module
        ```python
        px.add_token('op_lambda', '[(][<=>+\\-*/%~?@|!&\\^.:;]+[)]', apply=None)
        ```
      - Import macro defined in this module  
        ```python
        px.add_macro_rule('expr', ['op_lambda'], apply=macro_op_lambda_0, lpriority=-1, rpriority=-1)
        ```
      Load resulting `_import_grammar` definition into interpreter:  
      ```python
      def _import_grammar(module_name=None):
         px = parse_context()
         if not px.import_new_module(_import_grammar): return
         if '_imported_syntax_modules' in globals():
            for sm in _imported_syntax_modules:
               if hasattr(sm, '_import_grammar'):
                   sm._import_grammar(px)
         
         px.add_token('op_lambda', '[(][<=>+\\-*/%~?@|!&\\^.:;]+[)]', apply=None)
         px.add_macro_rule('expr', ['op_lambda'], apply=macro_op_lambda_0, lpriority=-1, rpriority=-1)
      ```
## More examples

### Simple match operator
This example is match operator that is an analog of C++ switch operator. Actually, 
Python 3.10 already has [more powerful match operator](https://docs.python.org/3.10/whatsnew/3.10.html#pep-634-structural-pattern-matching)
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
After macro expansion it is equivalent to implementation using if-elif-else construction available in Python 3.8:
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
as a function, we need to convert `x -> f(x)` to `(lambda x: f(x))`. Since x can be 
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
   # simplest conversion is: ast -> text -> ast
   args = parse_context().ast_to_text(x)
   return `(lambda {args}: $y)`
```
Here we defined operator `->` with the highest left priority and lowest right priority. 
This means that left side must be atomic expression like a variable or tuple, 
right side can contain arbitrary operations except `or`, `and` and `not`. 
Also, this allows to combine `->` operators in a chain to produce lambda returning 
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