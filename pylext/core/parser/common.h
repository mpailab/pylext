#pragma once
#include <vector>
#include <utility>

template<class T, template<class ...> class Set, class ... Args>
struct Enumerator {
	Set<T, int, Args...> _m;
	std::vector<T> _i;
	Enumerator() = default;
	template<class T1, class ... Ts>
	explicit Enumerator(const T1 &x1, const Ts& ... x) :_m(x1,x...) {}
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
	bool has(const T& x) const {
		return _m.count(x) > 0;
	}
	[[nodiscard]] int size()const {
		return (int)_i.size();
	}
};

template<class T>
inline int len(T &x) {
	return (int)x.size();
}
