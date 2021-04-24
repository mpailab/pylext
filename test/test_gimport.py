import pylext

# pylext.importer._dbg_statements=True
# pylext.core.wrap.set_debug(0xFF)


def test_simple_gimport():
    print('Testing simple gimport')
    import macros.romb_import as m

    # Запускаем тест
    res = m.run_test(len(m.validation)-1)

    # Печатаем результат
    print(f'result  = {res}')
    print(f'correct = {m.validation}')
    print("Test simple: "+("ok" if res == m.validation else "failed"))
    m.test_from_match()


def test_romb_gimport():
    print('Testing romb gimport')
    import macros.romb_import as m
    m.test_romb_1()
    m.test_romb_2()
    m.test_romb_3()


test_simple_gimport()
test_romb_gimport()
