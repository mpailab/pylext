import pylext
from pylext import exec_macros

def test_exception():
    res = 0
    try:
        exec_macros("""
def f(x)):
    return
""", {})
    except RuntimeError as err:
        res = 1
    assert res == 1

def test_simple_import():
    import macros.romb_import as m
    res = m.run_test(len(m.validation)-1)
    assert res == m.validation

def test_simple_gimport():
    import macros.romb_import as m
    m.test_from_match()

def test_romb_gimport():
    import macros.romb_import as m
    m.test_romb_1()
    m.test_romb_2()
    m.test_romb_3()
