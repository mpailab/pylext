"""Test parsing of Python 3.10 syntax features"""

import sys
import pytest
from conftest import parse_and_compare

# Skip all tests if Python < 3.10
pytestmark = pytest.mark.skipif(
    sys.version_info < (3, 10),
    reason="Requires Python 3.10+"
)


def test_pattern_matching_literals():
    """Python 3.10: Pattern matching with literals"""
    parse_and_compare("""
def check(x):
    match x:
        case 0:
            return "zero"
        case 1 | 2:
            return "one or two"
        case _:
            return "other"
""")


def test_pattern_matching_sequences():
    """Python 3.10: Pattern matching with sequence patterns"""
    parse_and_compare("""
def process_point(point):
    match point:
        case [0, 0]:
            return "origin"
        case [x, 0]:
            return f"on x-axis at {x}"
        case [0, y]:
            return f"on y-axis at {y}"
        case [x, y]:
            return f"at ({x}, {y})"
""")


def test_pattern_matching_mappings():
    """Python 3.10: Pattern matching with mapping patterns"""
    parse_and_compare("""
def process_data(data):
    match data:
        case {"name": name, "age": age}:
            return f"{name} is {age}"
        case {"name": name}:
            return f"Name: {name}"
        case _:
            return "Unknown"
""")


def test_pattern_matching_guard():
    """Python 3.10: Pattern matching with guard"""
    parse_and_compare("""
def categorize(value):
    match value:
        case x if x > 0:
            return "positive"
        case x if x < 0:
            return "negative"
        case _:
            return "zero"
""")


def test_pattern_matching_as_pattern():
    """Python 3.10: Pattern matching with as pattern"""
    parse_and_compare("""
def process(data):
    match data:
        case [1, 2] as pair:
            return f"Got pair: {pair}"
        case x as value:
            return f"Got value: {value}"
""")


def test_union_types():
    """Python 3.10: Union types with | operator"""
    parse_and_compare("""
def process(x: int | str) -> int | None:
    if isinstance(x, int):
        return x
    return None

def combine(a: int | float, b: int | float) -> int | float:
    return a + b
""")


def test_parenthesized_context_managers():
    """Python 3.10: Parenthesized context managers"""
    parse_and_compare("""
with (
    open('file1.txt') as f1,
    open('file2.txt') as f2
):
    data = f1.read() + f2.read()

with (
    resource1() as r1,
    resource2() as r2,
    resource3() as r3
):
    process(r1, r2, r3)
""")
