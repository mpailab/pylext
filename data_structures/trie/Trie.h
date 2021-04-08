#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <list>
#include <utility>
#include <iostream>
#include <deque>    
#include <iterator>     
#include <string>  
#include <array>
#include <unordered_map>
#include <chrono>
#include <utility>
#include <cstdint>
#include <cmath>
#include <vector>
#include <random>
#include <immintrin.h>
#include <stdio.h>
#include <algorithm>

int popcnt(uint64_t x) {
	return (int)_mm_popcnt_u64(x);
}


using namespace std;

template<class V, int SIZE_OF_VERTIX>
class Vertix_Const
{
private:
	std::array<uint64_t, SIZE_OF_VERTIX> mask;
	std::array<int, SIZE_OF_VERTIX> counts;
	//std::vector<V> values;
	//V _default;
	int BIT_SIZE = sizeof(uint64_t) * 8;
public:
	std::vector<V> values;
	Vertix_Const() { mask.fill(uint64_t(0)); }
	Vertix_Const(V def) { _default = def; mask.fill(uint64_t(0)); }
	Vertix_Const(std::vector<pair<int, V>> dict, V def) {
		_default = def;
		mask.fill(uint64_t(0));
		sort(dict.begin(), dict.end(), [](pair<int, V> x, pair<int, V> y) { return x.first < y.first; });
		for (int i = 0; i < dict.size(); i++) {
			int d;
			bool res = true;
			for (int j = 0; j < i; j++) {
				if (dict[j].first == dict[i].second) {
					res = false;
				}
			}
			if (res) {
				d = dict[i].first / BIT_SIZE;
				mask[d] = mask[d] | uint64_t(1) << (dict[i].first % BIT_SIZE);
				values.push_back(dict[i].second);
			}
		}
		int sum = 0;
		for (int i = 0; i < SIZE_OF_VERTIX; i++) {
			int x = popcnt(mask[i]);
			counts[i] = sum;
			sum += x;
		}
	}

	int hash(int n) {
		int cnts;
		uint64_t m, t;
		cnts = counts[n / BIT_SIZE];
		m = mask[n / BIT_SIZE];
		t = uint64_t(1) << (n % BIT_SIZE);
		if ((t & m) == 0)
			return -1;
		return cnts + popcnt(m & (t - 1));
	}

	V& get(int n) {
		int x = hash(n);
		if (x == -1)
			return values[0];
		else
			return values[x];
	}

	void add(int n, V value) {
		typename std::vector<V>::iterator it;
		int x = hash(n), d;
		if (x != -1) {
			values[x] = value;
		}
		else {
			d = n / BIT_SIZE;
			mask[d] = mask[d] | uint64_t(1) << (n % BIT_SIZE);
			int sum = 0;
			for (int i = 0; i < SIZE_OF_VERTIX; i++) {
				int x = popcnt(mask[i]);
				counts[i] = sum;
				sum += x;
			}
			values.insert(std::next(values.begin(), hash(n)), std::move(value));
		}

	}

	void del(int n) {
		typename std::vector<V>::iterator it;
		int x = hash(n), d;
		if (x != -1) {
			d = n / BIT_SIZE;
			mask[d] = mask[d] & ~(uint64_t(1) << (n % BIT_SIZE));
			int sum = 0;
			for (int i = 0; i < SIZE_OF_VERTIX; i++) {
				int x = popcnt(mask[i]);
				counts[i] = sum;
				sum += x;
			}
			int i = 0;
			int c = hash(n);
			for (it = values.begin(); it != values.end();) {
				if (i == c)
					break;
				i++;
			}
			values.erase(it);
		}
	}

};

using namespace std;
/// Префиксное дерево, как множество
struct Trie {
	bool final = false;
	vector<Trie> next;
	bool operator[](const char *m)const {
		const Trie* curr = this;
		for (; *m; m++) {
			if (next.empty())return false;
			curr = &next[(unsigned char)m[0]];
		}
		return curr->final;
	}
	Trie& operator<<(const char *str) {
		Trie *c = this;
		for (; *str; str++) {
			if (next.empty())next.resize(256);
			c = &next[(unsigned char)str[0]];
		}
		c->final = true;
		return *this;
	}
};

template<class T>
struct TrieV {
	bool final = false;
	int _size = 0;
	T val{};
	//TrieV<T> buf;
	Vertix_Const<TrieV<T>, 4> next{};
	int used_memory() const {
		int res = (int)sizeof(TrieM<T>);
		for (auto& x : next.values)
			res += x.used_memory();
		//res += (next.capacity() - next.size()) * (int)sizeof(TrieM<T>);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m)const {
		const TrieV<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.hash((unsigned char)m[0] == -1)) return 0;
			curr = &curr->next.get((unsigned char)m[0]);
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
		TrieV<T>* curr = this;
		TrieV<T> buf;
		for (; *m; m++) {
			if (curr->next.hash((unsigned char)m[0] == -1)) curr->next.add((unsigned char)m[0], buf);
			curr = &curr->next.get((unsigned char)m[0]);
		}
		_size += !curr->final;
		curr->final = true;
		return curr->val;
	}
	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}
	const T* operator()(const char* m, int& pos) {
		const T* res = 0;
		TrieV<T>* curr = this;
		int p1 = pos;
		for (; m[p1]; p1++) {
			if (curr->next.hash((unsigned char)m[p1] == -1))return res;
			curr = &curr->next.get((unsigned char)m[p1]);
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
		}
		return res;// curr->final ? &curr->val : 0;
	}
};


/// Префиксное дерево, как map
template<class T>
struct TrieM {
	bool final = false;
	int _size = 0;
	T val{};
	vector<TrieM<T>> next;
	int used_memory() const {
	    int res = (int)sizeof(TrieM<T>);
        for(auto &x: next)
            res += x.used_memory();
        res += (next.capacity()-next.size())*(int)sizeof(TrieM<T>);
        return res;
	}
	int size()const { return _size; }
	const T* operator()(const char *m)const {
		const TrieM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			curr = &curr->next[(unsigned char)m[0]];
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char *m) {
		TrieM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())curr->next.resize(256);
			curr = &curr->next[(unsigned char)m[0]];
		}
		_size += !curr->final;
		curr->final = true;
		return curr->val;
	}
	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}
	const T*operator()(const char *m, int &pos)const {
		const T*res = 0;
		const TrieM<T>* curr = this;
		int p1 = pos;
		for (; m[p1]; p1++) {
			if (curr->next.empty())return res;
			curr = &curr->next[(unsigned char)m[p1]];
			if (curr->final) {
				res = &curr->val;
				pos = p1+1;
			}
		}
		return res;// curr->final ? &curr->val : 0;
	}
};


template<class T>
struct TrieUM {
	bool final = false;
	int _size = 0;
	T val{};
	unordered_map<int, TrieUM<T>> next;
	int used_memory() const {
		int res = (int)(sizeof(TrieUM<T>));
		for (auto& x : next)
			res += x.second.used_memory() + sizeof(int) + sizeof(void*);
		//res += (next.capacity() - next.size()) * (int)sizeof(TrieUM<T>);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m)const {
		const TrieUM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			curr = &curr->next[(unsigned char)m[0]];
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m){
		TrieUM<T>* curr = this;
		for (; *m; m++) {
			//if (curr->next.empty())curr->next.resize(256);
			curr = &curr->next[(unsigned char)m[0]];
		}
		_size += !curr->final;
		curr->final = true;
		return curr->val;
	}
	T& operator[](const std::string& m){
		return (*this)[m.c_str()];
	}
	
	const T* operator()(const char* m, int& pos){
		const T* res = 0;
		TrieUM<T>* curr = this;
		int p1 = pos;
		for (; m[p1]; p1++) {
			if (curr->next.empty())return res;
			curr = &curr->next[(unsigned char)m[p1]];
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
		}
		return res;// curr->final ? &curr->val : 0;
	}
};

template<class T>
struct TrieL {
	bool final = false;
	int _size = 0;
	T val{};
	list<pair<int,TrieL<T>>> next;
	int used_memory() const {
		int res = (int)sizeof(TrieL<T>);
		for (auto& x : next)
			res += x.second.used_memory() + sizeof(int) + 2*sizeof(void*);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m)const {
		TrieL<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			list<pair<int, TrieL<T>>>::iterator it = curr->next.begin();
			for (list<pair<int, TrieL<T>>>::iterator it = curr->next.begin(); it != curr->next.end(); ++it)
			{
				if ((*it).first == (unsigned char)m[0])
				{
					curr = &(*it).second;
					break;
				}
			}
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
		TrieL<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())curr->next.resize(256);
			for (list<pair<int, TrieL<T>>>::iterator it = curr->next.begin(); it != curr->next.end(); ++it)
			{
				if ((*it).first == (unsigned char)m[0])
				{
					curr = &(*it).second;
					break;
				}
			}
		}
		_size += !curr->final;
		curr->final = true;
		return curr->val;
	}
	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}
	const T* operator()(const char* m, int& pos){
		const T* res = 0;
		TrieL<T>* curr = this;
		int p1 = pos;
		for (; m[p1]; p1++) {
			if (curr->next.empty())return res;
			for (list<pair<int, TrieL<T>>>::iterator it = curr->next.begin(); it != curr->next.end(); ++it)
				{
					if ((*it).first == (unsigned char)m[p1])
					{
						curr = &(*it).second;
						break;
					}
				}
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
		}
		return res;// curr->final ? &curr->val : 0;
	}
};


