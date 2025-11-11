"""Test parsing of Python 3.11+ syntax features"""

import sys
import pytest
from conftest import parse_and_compare

# Skip all tests if Python < 3.11
pytestmark = pytest.mark.skipif(
    sys.version_info < (3, 11),
    reason="Requires Python 3.11+"
)


def test_exception_groups():
    """Python 3.11: Exception groups with except*"""
    parse_and_compare("""
def handle_errors():
    try:
        raise ExceptionGroup("errors", [ValueError("v"), TypeError("t")])
    except* ValueError as e:
        print(f"ValueError: {e}")
    except* TypeError as e:
        print(f"TypeError: {e}")
""")


def test_exception_groups_multiple():
    """Python 3.11: Multiple exception groups"""
    parse_and_compare("""
def process():
    try:
        risky_operation()
    except* ValueError as e:
        handle_value_errors(e)
    except* (TypeError, KeyError) as e:
        handle_type_key_errors(e)
    except* Exception as e:
        handle_other_errors(e)
""")
