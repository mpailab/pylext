"""Test parsing of Python 3.12+ syntax features"""

import sys
import pytest
from conftest import parse_and_compare

# Skip all tests if Python < 3.12
pytestmark = pytest.mark.skipif(
    sys.version_info < (3, 12),
    reason="Requires Python 3.12+"
)


def test_type_parameters_function():
    """Python 3.12: Type parameters for functions"""
    parse_and_compare("""
def identity[T](x: T) -> T:
    return x

def pair[S, T](a: S, b: T) -> tuple[S, T]:
    return (a, b)
""")


def test_type_parameters_class():
    """Python 3.12: Type parameters for classes"""
    parse_and_compare("""
class Container[T]:
    def __init__(self, value: T):
        self.value = value

    def get(self) -> T:
        return self.value

class Pair[S, T]:
    def __init__(self, first: S, second: T):
        self.first = first
        self.second = second
""")


def test_type_statement():
    """Python 3.12: Type statement"""
    parse_and_compare("""
type Point = tuple[float, float]
type IntOrStr = int | str
type ListOfInts = list[int]
""")


def test_type_parameters_with_bounds():
    """Python 3.12: Type parameters with bounds"""
    parse_and_compare("""
def process[T: int | str](value: T) -> T:
    return value

class Numeric[T: (int, float)]:
    def __init__(self, value: T):
        self.value = value
""")
