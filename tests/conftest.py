import ast
import pytest
from pylext.core.parse import ParseContext, parse


def parse_and_compare(code):
    """
    Test if code is parsed correctly by comparing with standard Python AST.

    1. Parse with Python's ast module -> ref_ast
    2. Parse with pylext parser -> result
    3. Convert back to text using ast_to_text
    4. Parse result text with Python's ast -> result_ast
    5. Compare ref_ast and result_ast using ast.unparse
    """
    # Step 1: Parse with standard Python parser
    try:
        ref_ast = ast.parse(code)
        ref_code = ast.unparse(ref_ast)
    except SyntaxError as e:
        pytest.fail(f"Standard Python parser failed: {e}")

    # Step 2-3: Parse with pylext and convert back to text
    try:
        with ParseContext({}) as px:
            result_parts = []
            for stmt_ast in parse(code, px):
                stmt_text = px.ast_to_text(stmt_ast)
                result_parts.append(stmt_text)
            result_code = ''.join(result_parts)
    except Exception as e:
        pytest.fail(f"PyLExt parser failed: {e}")

    # Step 4: Parse result with standard Python parser
    try:
        result_ast = ast.parse(result_code)
        result_unparsed = ast.unparse(result_ast)
    except SyntaxError as e:
        pytest.fail(f"Result code is not valid Python: {e}\nResult code:\n{result_code}")

    # Step 5: Compare by unparsing both
    assert ref_code == result_unparsed, f"AST mismatch:\nExpected:\n{ref_code}\n\nGot:\n{result_unparsed}"


def pytest_addoption(parser):
    parser.addoption('--longrun', action='store_true', dest="longrun",
        default=False, help="enable longrundecorated tests")

def pytest_configure(config):
    config.addinivalue_line("markers", "long: mark test as long to run")

def pytest_collection_modifyitems(config, items):
    if config.getoption("--longrun"):
        return
    skip_longrun = pytest.mark.skip(reason="need --longrun option to run")
    for item in items:
        if "long" in item.keywords:
            item.add_marker(skip_longrun)