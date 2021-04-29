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
#include "Common.h"


using namespace std;
/// Префиксное дерево, как множество
struct Trie {
	bool final = false;
	vector<Trie> next;
	bool operator[](const char* m)const {
		const Trie* curr = this;
		for (; *m; m++) {
			if (next.empty())return false;
			curr = &next[(unsigned char)m[0]];
		}
		return curr->final;
	}
	Trie& operator<<(const char* str) {
		Trie* c = this;
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
	Vertix_Const<TrieV<T>, 256> next{};
	int used_memory() const {
		int res = (int)sizeof(TrieV<T>);
		for (auto& x : next.values)
			res += x.used_memory();
		//res += (next.capacity() - next.size()) * (int)sizeof(TrieM<T>);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m) {
		TrieV<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.hash((unsigned char)m[0]) == -1) return 0;
			curr = &curr->next.get((unsigned char)m[0]);
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
		TrieV<T>* curr = this;
		TrieV<T> buf;
		for (; *m; m++) {
			if (curr->next.hash((unsigned char)m[0]) == -1) curr->next.add((unsigned char)m[0], buf);
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
			if (curr->next.hash((unsigned char)m[p1]) == -1)return res;
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
		for (auto& x : next)
			res += x.used_memory();
		res += (next.capacity() - next.size()) * (int)sizeof(TrieM<T>);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m)const {
		const TrieM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			curr = &curr->next[(unsigned char)m[0]];
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
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
	const T* operator()(const char* m, int& pos)const {
		const T* res = 0;
		const TrieM<T>* curr = this;
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
	const T* operator()(const char* m) {
		TrieUM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			curr = &curr->next[(unsigned char)m[0]];
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
		TrieUM<T>* curr = this;
		for (; *m; m++) {
			//if (curr->next.empty())curr->next.resize(256);
			curr = &curr->next[(unsigned char)m[0]];
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
struct TrieVP {
	bool final = false;
	int _size = 0;
	T val{};
	vector<pair<int, TrieVP<T>>> next;
	int used_memory() const {
		int res = (int)sizeof(TrieM<T>);
		for (auto& x : next)
			res += x.second.used_memory() + sizeof(int) + sizeof(void*);
		res += (next.capacity() - next.size()) * (int)sizeof(TrieM<T>);
		return res;
	}
	int size()const { return _size; }
	const T* operator()(const char* m)const {
		const TrieVP<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())return 0;
			for (auto& x : curr->next) {
				if (x.first == (unsigned char)m[0]) {
					curr = &x.second;
				}
			}
			//curr = &curr->next[(unsigned char)m[0]].second;
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char* m) {
		TrieVP<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())curr->next.resize(256);
			int i = 0;
			for (auto& x : curr->next) {
				x.first = i;
				i++;
			}
			for (auto& x : curr->next) {
				if (x.first == (unsigned char)m[0]) {
					curr = &x.second;
				}
			}
			//curr = &curr->next[(unsigned char)m[0]].second;
		}
		_size += !curr->final;
		curr->final = true;
		return curr->val;
	}
	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}
	const T* operator()(const char* m, int& pos)const {
		const T* res = 0;
		const TrieVP<T>* curr = this;
		int p1 = pos;
		for (; m[p1]; p1++) {
			if (curr->next.empty())return res;
			for (auto& x : curr->next) {
				if (x.first == (unsigned char)m[p1]) {
					curr = &x.second;
				}
			}
			//curr = &curr->next[(unsigned char)m[p1]].second;
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
		}
		return res;// curr->final ? &curr->val : 0;
	}
};


template<class T>
struct TrieCUM {
	bool final = false;
	string edgelabel;
	int _size = 0;
	T val{};
	unordered_map<char, TrieCUM<T>> next;
	int used_memory() const {
		int res = (int)(sizeof(TrieCUM<T>));
		for (auto& x : next)
			res += x.second.used_memory() + sizeof(int) + sizeof(void*);
		//res += (next.capacity() - next.size()) * (int)sizeof(TrieUM<T>);
		return res;
	}
	int size()const { return _size; }

	const T* operator()(const char* word) {
		TrieCUM<T>* curr = this;
		size_t i = 0;
		if (curr->next.find(word[0]) == curr->next.end()) {
			return 0;
		}
		bool flag = false;
		curr = &curr->next[word[0]];
		while (word[i]) {
			int j = 0;
			auto& match_word = curr->edgelabel;
			for (size_t j = 0; j < match_word.size(); j++) {
				if (word[i + j] != match_word[j])
					return nullptr;
			}
			i += match_word.size();
			if (!word[i]) {
				return curr->final ? &curr->val : nullptr;
			}
			if (curr->next.find(word[i]) == curr->next.end()) {
				return nullptr;
			}
			else {
				curr = &curr->next[word[i]];
			}
			continue;
			////////////////////////////////////
			/*while (word[i] && (j < match_word.size()) && (word[i] == match_word[j])) {
				i++;
				j++;
			}
			if (i == strlen(word)) {
				if ((j == strlen(match_word)) && curr->final) {
					return &curr->val;
				}
				else return nullptr;
			}
			else {
				if (j == match_word.size()) {
					if (curr->next.find(word[i]) == curr->next.end()) {
						return nullptr;
					}
					else {
						curr = &curr->next[word[i]];
					}
				}
				else
				{
					flag = false;
					break;
				}
			}*/
		}
		return flag ? &curr->val : 0;
	}

	T& operator[](const char* word) {
		TrieCUM<T>* curr = this;
		if (curr->next.find(word[0]) == curr->next.end()) {
			curr = &curr->next[word[0]];
			curr->edgelabel = word;
			_size += !curr->final;
			curr->final = true;
			return curr->val;
		}
		else {
			size_t i = 0, j = 0;
			//const char* cmp_word;
			curr = &curr->next[word[0]];
			while (true) {
				auto& cmp_word = curr->edgelabel;
				j = 0;
				while ((word[i]) && (j < cmp_word.size()) && (word[i] == cmp_word[j])) {
					i++;
					j++;
				}
				//Случай 1: слово, которое хотим добавить полностью содержится в слове, приписанном одному из рёбер.
				if (!word[i]) {
					//Они полностью совпадают.
					if (j == cmp_word.size()) {
						_size += !curr->final;
						curr->final = true;
						return curr->val;
					}
					// Слово на ребре больше, тогда добавляем вершину, соответствующую слову cmp_word.
					else {
						string remain = cmp_word.substr(j);
						TrieCUM<T> newnode;
						newnode.edgelabel = remain;
						newnode.final = curr->final;
						newnode.next = move(curr->next);
						newnode.val = curr->val;

						curr->edgelabel.resize(j);
						_size += !curr->final;
						curr->final = true;
						curr->next.clear();
						curr->next[remain[0]] = move(newnode);
						return curr->val;
					}
				}
				else {
					// Случай 2: Слово cmp_word на ребре является частью слова word, которое добавляем.
					if (j == cmp_word.size()) {
						if (curr->next.find(word[i]) == curr->next.end()) {
							//curr->edgelabel.resize(j);
							curr = &curr->next[word[i]];
							curr->edgelabel = word + i; //substr(word, i, strlen(word));
							_size += 1;
							curr->final = true;
							return curr->val;
						}
						else {
							curr = &curr->next[word[i]];
						}
					}
					// Случай 3: Ни одно из слов не является подмножеством другого, но у них есть совпадающие части. Делаем разветвление.
					else {
						string remain_i = word + i;
						string remain_j = cmp_word.substr(j);
						string match(cmp_word, 0, j);

						TrieCUM<T> newnode;
						newnode.final = curr->final;
						newnode.next = move(curr->next);
						newnode.edgelabel = remain_j;
						newnode.val = curr->val;
						curr->next.clear();


						curr->next[remain_j[0]] = move(newnode);
						TrieCUM<T>& newnode2 = curr->next[remain_i[0]];
						_size += curr->final;
						newnode2.final = true;
						newnode2.edgelabel = remain_i;

						curr->final = false;
						curr->edgelabel = match;
						return newnode2.val;
					}
				}
			}
		}
	}

	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}

	const T* operator()(const char* word, int& pos) {
		TrieCUM<T>* curr = this;
		int p1 = pos;
		const T* res = nullptr;
		if (curr->next.find(word[p1]) == curr->next.end()) {
			return nullptr;
		}
		bool flag = false;
		curr = &curr->next[word[p1]];
		while (word[p1]) {
			int j = 0;
			auto& match_word = curr->edgelabel;
			for (size_t j = 0; j < match_word.size(); j++) {
				if (word[p1 + j] != match_word[j])
					return res;
			}
			p1 += match_word.size();
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
			if (!word[p1]) {
				return res;
			}
			if (curr->next.find(word[p1]) == curr->next.end()) {
				return res;
			}
			else {
				curr = &curr->next[word[p1]];
			}
			continue;
			////////////////////////////////////
			/*while (word[i] && (j < match_word.size()) && (word[i] == match_word[j])) {
				i++;
				j++;
			}
			if (i == strlen(word)) {
				if ((j == strlen(match_word)) && curr->final) {
					return &curr->val;
				}
				else return nullptr;
			}
			else {
				if (j == match_word.size()) {
					if (curr->next.find(word[i]) == curr->next.end()) {
						return nullptr;
					}
					else {
						curr = &curr->next[word[i]];
					}
				}
				else
				{
					flag = false;
					break;
				}
			}*/
		}
		return res;
	}
};

template<class T>
struct TrieCV {
	bool final = false;
	string edgelabel;
	int _size = 0;
	T val{};
	Vertix_Const<TrieCV<T>, 256> next{};
	//unordered_map<char, TrieCUM<T>> next;
	int used_memory() const {
		int res = (int)(sizeof(TrieCV<T>));
		for (auto& x : next.values)
			res += x.used_memory();
		//res += (next.capacity() - next.size()) * (int)sizeof(TrieUM<T>);
		return res;
	}
	int size()const { return _size; }

	const T* operator()(const char* word) {
		TrieCV<T>* curr = this;
		size_t i = 0;
		if (curr->next.hash(word[0]) == -1) {
			return 0;
		}
		bool flag = false;
		curr = &curr->next.get(word[0]);
		while (word[i]) {
			int j = 0;
			auto& match_word = curr->edgelabel;
			for (size_t j = 0; j < match_word.size(); j++) {
				if (word[i + j] != match_word[j])
					return nullptr;
			}
			i += match_word.size();
			if (!word[i]) {
				return curr->final ? &curr->val : nullptr;
			}
			if (curr->next.hash(word[i]) == -1) {
				return nullptr;
			}
			else {
				curr = &curr->next.get(word[i]);
			}
			continue;
			////////////////////////////////////
			/*while (word[i] && (j < match_word.size()) && (word[i] == match_word[j])) {
				i++;
				j++;
			}
			if (i == strlen(word)) {
				if ((j == strlen(match_word)) && curr->final) {
					return &curr->val;
				}
				else return nullptr;
			}
			else {
				if (j == match_word.size()) {
					if (curr->next.find(word[i]) == curr->next.end()) {
						return nullptr;
					}
					else {
						curr = &curr->next[word[i]];
					}
				}
				else
				{
					flag = false;
					break;
				}
			}*/
		}
		return flag ? &curr->val : 0;
	}

	T& operator[](const char* word) {

		TrieCV<T>* curr = this;
		TrieCV<T> buf;
		Vertix_Const<TrieCV<T>, 256> buf1{};
		if (curr->next.hash(word[0]) == -1) {
			curr->next.add(word[0], buf);
			curr = &curr->next.get(word[0]);
			curr->edgelabel = word;
			_size += !curr->final;
			curr->final = true;
			return curr->val;
		}
		else {
			size_t i = 0, j = 0;
			//const char* cmp_word;
			curr = &curr->next.get(word[0]);
			while (true) {
				auto& cmp_word = curr->edgelabel;
				j = 0;
				while ((word[i]) && (j < cmp_word.size()) && (word[i] == cmp_word[j])) {
					i++;
					j++;
				}
				//Случай 1: слово, которое хотим добавить полностью содержится в слове, приписанном одному из рёбер.
				if (!word[i]) {
					//Они полностью совпадают.
					if (j == cmp_word.size()) {
						_size += !curr->final;
						curr->final = true;
						return curr->val;
					}
					// Слово на ребре больше, тогда добавляем вершину, соответствующую слову cmp_word.
					else {
						string remain = cmp_word.substr(j);
						TrieCV<T> newnode;
						newnode.edgelabel = remain;
						newnode.final = curr->final;
						newnode.next = move(curr->next);
						newnode.val = curr->val;

						curr->edgelabel.resize(j);
						_size += !curr->final;
						curr->final = true;
						curr->next.clear();
						curr->next.add(remain[0], newnode);
						//curr->next[remain[0]] = move(newnode);
						return curr->val;
					}
				}
				else {
					// Случай 2: Слово cmp_word на ребре является частью слова word, которое добавляем.
					if (j == cmp_word.size())
					{
						if (curr->next.hash(word[i]) == -1)
						{
							//curr->edgelabel.resize(j);
							curr->next.add(word[i], buf);
							curr = &curr->next.get(word[i]);
							//curr = &curr->next[word[i]];
							curr->edgelabel = word + i; //substr(word, i, strlen(word));
							_size += 1;
							curr->final = true;
							return curr->val;
						}
						else {
							curr = &curr->next.get(word[i]);
						}
					}
					// Случай 3: Ни одно из слов не является подмножеством другого, но у них есть совпадающие части. Делаем разветвление.
					else {
						string remain_i = word + i;
						string remain_j = cmp_word.substr(j);
						string match(cmp_word, 0, j);

						TrieCV<T> newnode;
						newnode.final = curr->final;
						newnode.next = move(curr->next);
						newnode.edgelabel = remain_j;
						newnode.val = curr->val;
						curr->next.clear();


						curr->next.add(remain_j[0], newnode);
						//curr->next[remain_j[0]] = move(newnode);
						curr->next.add(remain_i[0], buf);
						TrieCV<T>& newnode2 = curr->next.get(remain_i[0]);
						_size += curr->final;
						newnode2.final = true;
						newnode2.edgelabel = remain_i;

						curr->final = false;
						curr->edgelabel = match;
						return newnode2.val;
					}
				}
			}
		}
	}

	T& operator[](const std::string& m) {
		return (*this)[m.c_str()];
	}

	const T* operator()(const char* word, int& pos) {
		TrieCV<T>* curr = this;
		int p1 = pos;
		const T* res = nullptr;
		if (curr->next.hash(word[p1]) == -1) {
			return nullptr;
		}
		bool flag = false;
		curr = &curr->next.get(word[p1]);
		while (word[p1]) {
			int j = 0;
			auto& match_word = curr->edgelabel;
			for (size_t j = 0; j < match_word.size(); j++) {
				if (word[p1 + j] != match_word[j])
					return res;
			}
			p1 += match_word.size();
			if (curr->final) {
				res = &curr->val;
				pos = p1 + 1;
			}
			if (!word[p1]) {
				return res;
			}
			if (curr->next.hash(word[p1]) == -1) {
				return res;
			}
			else {
				curr = &curr->next.get(word[p1]);
			}
			continue;
			////////////////////////////////////
			/*while (word[i] && (j < match_word.size()) && (word[i] == match_word[j])) {
				i++;
				j++;
			}
			if (i == strlen(word)) {
				if ((j == strlen(match_word)) && curr->final) {
					return &curr->val;
				}
				else return nullptr;
			}
			else {
				if (j == match_word.size()) {
					if (curr->next.find(word[i]) == curr->next.end()) {
						return nullptr;
					}
					else {
						curr = &curr->next[word[i]];
					}
				}
				else
				{
					flag = false;
					break;
				}
			}*/
		}
		return res;
	}
};