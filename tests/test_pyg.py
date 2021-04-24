import sys
import pylext


# pylext.core.wrap.set_debug(0xFF)
# pylext.importer._dbg_statements=True


def test1():
    # Импортируем модуль с расширениями
    import macros.match as m

    # Запускаем тест
    res = [m.test1(x) for x in range(len(m.validation))]

    # Печатаем результат
    print(f'result  = {res}')
    print(f'correct = {m.validation}')
    print("Test1: " + ("ok" if res == m.validation else "failed"))


def test2():
    # Импортируем модуль с расширениями
    import macros.match as m

    data = [[1, 2, 3], 4.5, 6 + 6j, '456', b'467']
    print("Test2: ", end="")
    if m.serialize(data) == m.serialize_match(data):
        print("ok")
    else:
        print("failed")


def test3():
    from macros.range_op import test, validation
    res = test(0, 1)
    vld = validation(0, 1)
    print(f'result  = {res}')
    print(f'correct = {vld}')
    print("Test3: " + ("ok" if res == vld else "failed"))


def test4():
    from macros.simplefunc import listpow, check
    lx = [1, 2, 4]
    ly = [0, 1, 3]
    res = listpow(lx, ly)
    vld = check(lx, ly)
    print(f'result  = {res}')
    print(f'correct = {vld}')
    print("Test4: " + ("ok" if res == vld else "failed"))


test1()
test2()
test3()
test4()
