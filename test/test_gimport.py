import pylext
# pylext.importer._dbg_statements=True
# pylext.core.wrap.set_debug(0xFF)


def test_simple_gimport():
    print('Testing simple gimport')
    import macros.test_match as m

    # Запускаем тест
    res = m.run_test(len(m.validation)-1)

    # Печатаем результат
    print(f'result  = {res}')
    print(f'correct = {m.validation}')
    print("Test simple: "+("ok" if res == m.validation else "failed"))
    m.test_from_match()


test_simple_gimport()
