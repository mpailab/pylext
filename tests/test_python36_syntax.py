"""Test parsing of Python 3.6-3.9 syntax features (baseline)"""

from conftest import parse_and_compare


def test_simple_assignment():
    """Python 3.6: Simple assignment"""
    parse_and_compare("""
x = 1 + 2
y = [1, 2, 3]
z = {'a': 1, 'b': 2}
""")


def test_function_definition():
    """Python 3.6: Function definition"""
    parse_and_compare("""
def foo(x, y):
    return x + y

def bar(a, b=10, *args, **kwargs):
    pass
""")


def test_class_definition():
    """Python 3.6: Class definition"""
    parse_and_compare("""
class MyClass:
    def __init__(self, value):
        self.value = value

    def get_value(self):
        return self.value
""")


def test_comprehensions():
    """Python 3.6: List/dict/set comprehensions"""
    parse_and_compare("""
squares = [x**2 for x in range(10)]
evens = [x for x in range(10) if x % 2 == 0]
pairs = {x: x**2 for x in range(5)}
unique = {x for x in [1, 2, 2, 3, 3, 3]}
""")


def test_lambda():
    """Python 3.6: Lambda expressions"""
    parse_and_compare("""
f = lambda x: x * 2
g = lambda x, y: x + y
h = lambda: 42
""")


def test_decorators():
    """Python 3.6: Decorators"""
    parse_and_compare("""
@decorator
def foo():
    pass

@dec1
@dec2(arg)
def bar():
    pass

@property
def value(self):
    return self._value
""")


def test_context_managers():
    """Python 3.6: Context managers (with statement)"""
    parse_and_compare("""
with open('file.txt') as f:
    data = f.read()

with open('in.txt') as fin, open('out.txt') as fout:
    fout.write(fin.read())
""")


def test_exception_handling():
    """Python 3.6: Try-except-finally"""
    parse_and_compare("""
try:
    risky_operation()
except ValueError as e:
    handle_value_error(e)
except (TypeError, KeyError):
    handle_type_or_key_error()
except Exception:
    handle_other()
finally:
    cleanup()
""")


def test_async_await():
    """Python 3.6: Async/await"""
    parse_and_compare("""
async def fetch_data():
    result = await get_data()
    return result

async def process():
    async with resource() as r:
        data = await r.read()
    async for item in async_iter():
        process_item(item)
""")


def test_fstrings():
    """Python 3.6: F-strings"""
    parse_and_compare("""
name = "World"
message = f"Hello, {name}!"
value = 42
formatted = f"Value is {value:05d}"
""")


def test_type_annotations():
    """Python 3.6: Type annotations"""
    parse_and_compare("""
def greet(name: str) -> str:
    return f"Hello, {name}"

x: int = 42
y: list[int] = [1, 2, 3]

def process(items: list[str]) -> dict[str, int]:
    return {item: len(item) for item in items}
""")


def test_walrus_operator():
    """Python 3.8: Walrus operator (:=)"""
    parse_and_compare("""
if (n := len(data)) > 10:
    print(f"Large dataset: {n}")

while (line := file.readline()) != '':
    process(line)
""")


def test_positional_only_params():
    """Python 3.8: Positional-only parameters"""
    parse_and_compare("""
def func(a, b, /, c, d):
    return a + b + c + d

def pow(x, y, z=None, /):
    return x ** y if z is None else (x ** y) % z
""")
