#pragma once
//#include <unordered_map>
#include <vector>
#include <utility>

template<class T, template<class ...> class Set, class ... Args>
struct Enumerator {
	Set<T, int, Args...> _m;
	std::vector<T> _i;
	Enumerator() = default;
	template<class T1, class ... Ts>
	Enumerator(const T1 &x1, const Ts& ... x) :_m(x1,x...) {}
	const T& operator[](int i)const { return _i[i]; }
	int operator[](const T& x) {
		auto p = _m.insert(std::make_pair(x, (int)_i.size()));
		if (p.second)_i.push_back(x);
		return p.first->second;
	}
	int num(const T& x) const {
		auto it = _m.find(x);
		if (it == _m.end())return -1;
		return it->second;
	}
};

template<class T>
inline int len(T &x) {
	return (int)x.size();
}
