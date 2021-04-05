import sys
import cfgexts 

# Добавляем директорию для поиска модулей с расширениями
sys.path.append("../pymacros")

# Импортируем модуль с расширениями
import match_macro
from match_macro import validation as vld

# Запускаем тест
res = [match_macro.test(x) for x in range(10)]

# Печатаем результат
print(res)
print(vld)
print("ok" if all ( res[i] == vld[i] for i in range(10) ) else "failed")