import sys
import pylext

def test1():
    # Импортируем модуль с расширениями
    import macros.match as m

    # Запускаем тест
    res = [m.test1(x) for x in range(len(m.validation))]

    # Печатаем результат
    print(f'result  = {res}')
    print(f'correct = {m.validation}')
    print("Test1: "+("ok" if res == m.validation else "failed"))

def test2():
    # Импортируем модуль с расширениями
    import macros.match as m

    data = [[1,2,3],4.5,6+6j,'456',b'467']
    print("Test2: ", end="")
    if m.serialize(data) == m.serialize_match(data):
        print("ok")
    else:
        print("failed")


def test3():
    from macros.operator import test, validation
    res = test(0, 1)
    vld = validation(0, 1)
    print(f'result = {res}')
    print(f'result = {vld}')
    print("Test3: "+("ok" if res == vld else "failed"))


test1()
test2()
test3()
