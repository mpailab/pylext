#pragma once
#include <vector>
using namespace std;
/// Префиксное дерево, как множество
struct Trie {
	bool final;
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

/// Префиксное дерево, как map
template<class T>
struct TrieM {
	bool final = false;
	T val{};
	vector<TrieM<T>> next;
	const T* operator()(const char *m)const {
		const TrieM<T>* curr = this;
		for (; *m; m++) {
			if (next.empty())return 0;
			curr = &next[(unsigned char)m[0]];
		}
		return curr->final ? &curr->val : 0;
	}
	T& operator[](const char *m) {
		TrieM<T>* curr = this;
		for (; *m; m++) {
			if (curr->next.empty())curr->next.resize(256);
			curr = &curr->next[(unsigned char)m[0]];
		}
		curr->final = true;
		return curr->val;
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
		return curr->final ? &curr->val : 0;
	}
};