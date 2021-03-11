#include <unordered_map>
#include <stdexcept>
//#include <concepts>
#include "Common.h"

class AssertionFailed: public std::runtime_error {
public:
    AssertionFailed(const std::string& s):std::runtime_error(s){}
};

#ifdef _DEBUG
#define ASSERT(x) { if(!(x)) throw AssertionFailed(#x __FILE__ __LINE__); }
#else
#define ASSERT(x)
#endif
/*
template<class Ref, class V>
concept IReference = requires (const Ref &r, const V& rval, V& lval) {
    r = rval;
    lval = r;
};

template<class PM, class V>
concept IPositionMap =
        requires { PM{}; } &&
        requires (const V& default_val) { PM{default_val}; } &&
        requires (const PM &pm, int pos, int nt) { { pm.find(pos, nt) } -> std::convertible_to<const V*>; } &&
        requires (const PM &pm, int pos, int nt) {
            { pm(pos, nt) } -> IReference<V>;
            { pm.forget_before(pos); }
        };

template<class V>
class IPositionMap {
public:
    explicit IPositionMap(V default_val={});
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const;
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) const;
    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void forget_before(int pos);
};*/

template<class V>
class PositionMapUM {
    int erased = 0;
    V _default;
    std::unordered_map<std::pair<int,int>, V, HashPair> _data;
public:
    explicit PositionMapUM(V default_val={}): _default(default_val){}
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const {
        ASSERT(pos>=erased);
        auto it = _data.find(std::make_pair(pos, nt));
        return (it==_data.end()) ? nullptr : &it->second;
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        ASSERT(pos>=erased);
        return _data.insert(std::make_pair(std::make_pair(pos, nt), _default)).first->second;
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos){ erased = std::max(pos, erased); }
};
