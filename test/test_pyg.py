import sys
import pymacros


def test1():
    # Импортируем модуль с расширениями
    import macros.match as m

    # Запускаем тест
    res = [m.test(x) for x in range(len(m.validation))]

    # Печатаем результат
    print(f'result  = {res}')
    print(f'correct = {m.validation}')
    print("ok" if res == m.validation else "failed")


def test2():
    from macros.operator import test, validation
    res = test(0, 1)
    vld = validation(0, 1)
    print(f'result = {res}')
    print(f'result = {vld}')
    print("ok" if res == vld else "failed")


test1()
test2()
