#include <unordered_map>
#include <deque>  
#include <array>
#include <stdexcept>
#include <string>
#include <numeric>
#include "Common.h"

class AssertionFailed: public std::runtime_error {
public:
    AssertionFailed(const std::string& s):std::runtime_error(s){}
};


#ifdef _DEBUG
#define ASSERT(x) { if(!(x)) throw AssertionFailed("Assertion " #x " failed at " _FILE_ ", line "+std::to_string(__LINE__)); }
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
        //ASSERT(pos>=erased);
        auto it = _data.find(std::make_pair(pos, nt));
        return (it==_data.end()) ? nullptr : &it->second;
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        //ASSERT(pos>=erased);
        return _data.insert(std::make_pair(std::make_pair(pos, nt), _default)).first->second;
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos){ erased = std::max(pos, erased); }
};




template<class V,int SIZE>
class PositionMapDA {
    int erased = 0;
    V _default;
    std::deque<array<V,SIZE>> _data;
public:
    explicit PositionMapDA(V default_val = {}) : _default(default_val) {}
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        return ((pos - erased) > len) ? nullptr : _data[pos - erased][nt];
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        if ((pos - erased) > len) {
            _data.resize(pos - erased + 1, std::array<V, SIZE>{_default});
        }
        return _data[(pos - erased)][nt];
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos) {
        for (int i = 0; i < pos - erased; i++)
            _data.pop_back();
        erased = std::max(pos, erased); 
    }
};


template<class V>
class PositionMapDVC {
    int erased = 0;
    V _default;
    std::deque<Vertix_Const<V>> _data;
public:
    explicit PositionMapDVC(V default_val = {}) : _default(default_val) {}
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        if ((pos - erased) > len) {
            return nullptr;
        } else {
            if (_data[(pos - erased)].hash(nt) == -1)
                return nullptr;
            else
                return _data[(pos - erased)].get(nt);
        }
        //return ((pos - erased) > len) ? nullptr : &(_data[pos - erased][nt]);
        //auto it = _data.find(std::make_pair(pos, nt));
        //return (it == _data.end()) ? nullptr : &it->second;
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        if ((pos - erased) > len) {
            //std::array<V, 1024> _buf;
            //_buf.fill(_default);
            Vertix_Const<V> buf(_default);
            int nb = (pos - erased) - len;
            for (int i = 0; i < nb; i++)
                _data.push_back(buf);
        }
        if (_data[(pos - erased)].hash(nt) == -1) {
            _data[(pos - erased)].add(nt,_default);
            return _data[(pos - erased)].get(nt);

        } else {
            return  _data[(pos - erased)].get(nt);
        }
        //return _data[(pos - erased)][nt];
        //return _data.insert(std::make_pair(std::make_pair(pos, nt), _default)).first->second;
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos) {
        for (int i = 0; i < pos - erased; i++)
            _data.pop_back();
        erased = std::max(pos, erased);
    }
};



template<class V>
class PositionMapDV {
    int erased = 0;
    V _default;
    std::deque<std::vector<V>> _data;
public:
    explicit PositionMapDV(V default_val = {}) : _default(default_val) {}
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        return ((pos - erased) > len) ? nullptr : &(_data[pos - erased][nt]);
        //auto it = _data.find(std::make_pair(pos, nt));
        //return (it == _data.end()) ? nullptr : &it->second;
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        if ((pos - erased) > len) {
            std::vector<V> buf(1024);
            std::iota(std::begin(buf), std::end(buf), _default);
            int nb = (pos - erased) - len;
            for (int i = 0; i < nb; i++)
                _data.push_back(buf);
        }
        if (nt > _data[(pos - erased)].size())
        {
            for (int i = 0; i < nt - _data[(pos - erased)].size(); i++)
                _data[(pos - erased)].push_back(-1);
        }
        return _data[(pos - erased)][nt];
        //return _data.insert(std::make_pair(std::make_pair(pos, nt), _default)).first->second;
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos) {
        for (int i = 0; i < pos - erased; i++)
            _data.pop_back();
        erased = std::max(pos, erased);
    }
};


template<class V>
class PositionMapDUM {
    int erased = 0;
    V _default;
    std::deque<std::unordered_map<int, V>> _data;
public:
    explicit PositionMapDUM(V default_val = {}) : _default(default_val) {}
    // Ищет в структуре данных ключ (pos, nt) и возвращает 0, если не найдено и указатель на значение, если надено
    const V* find(int pos, int nt) const {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        return ((pos - erased) > len) ? nullptr : &(_data[pos - erased][nt]);
        //auto it = _data.find(std::make_pair(pos, nt));
        //return (it == _data.end()) ? nullptr : &it->second;
    }
    // Делает то же самое, но если ключ (pos, nt) не найден, то добавляется значение V{} с таким ключом и возвращается ссылка на него
    V& operator()(int pos, int nt) {
        //ASSERT(pos >= erased);
        int len = int(_data.size() - 1);
        if ((pos - erased) > len) {
            std::unordered_map<int, V> _buf;
            //std::iota(std::begin(_buf), std::end(_buf), _default);
            int nb = (pos - erased) - len;
            for (int i = 0; i < nb; i++)
                _data.push_back(_buf);
        }
        return _data[(pos - erased)][nt];
        //return _data.insert(std::make_pair(std::make_pair(pos, nt), _default)).first->second;
    }

    // Функция сообщает структуре данных, что можно стереть все ключи (p, nt) с p < pos
    void erase_before(int pos) {
        for (int i = 0; i < pos - erased; i++)
            _data.pop_back();
        erased = std::max(pos, erased);
    }
};