#pragma once
#include <cstdlib>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Timer.h"

void next_tok(const char* text, int &pos);
std::string read_file(const std::string& filename);
std::vector<std::string> read_words(const std::string& filename);

template<class Trie>
void fill_trie(Trie& tr, const std::vector<std::string>& words){
    int nw = 0;
    for (auto &w : words)
        tr[w] = ++nw;
}

template<class Trie>
double test_trie_find_speed(const std::vector<std::string>& kwords, int num_tries, const char* text, double timeout) {
    std::vector<Trie> tr(num_tries);
    for(auto &t : tr)
        fill_trie(t, kwords);

    uint64_t ncalls = 0;
    Timer tm;
    while (tm.time()<timeout) {
        for (int pos = 0, n=0; text[pos]; ncalls++, n++) {
            if(n >= num_tries) n = 0;
            if (!tr[n](text, pos)) {
                next_tok(text, pos);
            }
        }
    }
    return double(ncalls)/tm.time(); // Возвращаем число вызовов в секунду
}

template<class Trie>
int test_trie_copy_speed(const std::vector<std::string>& kwords, int num_tries, double timeout) {
    std::vector<Trie> tr(num_tries), tr2 = tr, *p1 = &tr, *p2 = &tr2;
    for(auto &t : tr)
        fill_trie(t, kwords);

    uint64_t ncopy = 0;
    Timer tm;
    for(; tm.time() < timeout; ncopy++) {
        *p2 = *p1;
        std::swap(p1, p2);
    }
    return double(ncopy)/tm.time();
}

template<class Trie>
double test_trie_memory(const std::vector<std::string>& words) {
    Trie tr;
    fill_trie(tr, words);
    size_t words_memory = 0;
    for(auto &w : words)
        words_memory += w.size();
    return double(tr.used_memory())/double(words_memory);
}

template<class Trie>
int test_trie_correct(const std::vector<std::string>& kwords, const char* text) {
    Trie tr1;
    TrieM<int> tr2;
    fill_trie(tr1, kwords);
    fill_trie(tr2, kwords);
    int flag = 1;
    for (auto& w : kwords) {
        if (tr1[w] != tr2[w]){
            flag = 0;
            cout << "tr1[w] = " << tr1[w] << " tr2[w] = " << tr2[w] << endl;
            break;
        }
    } 
    
    for (int pos = 0, n = 0; text[pos]; n++) {
        int pos1 = pos;
        int pos2 = pos;
        int pos3 = pos;
        int pos4 = pos;
        if ((((tr1(text, pos1) != nullptr) && (tr2(text, pos2)) == nullptr)) || ((tr1(text, pos3) == nullptr) && (tr2(text, pos4) != nullptr))) {
            flag = 0;
            pos1 = pos;
            pos2 = pos;
            cout << "Sanya " << tr1(text, pos1) <<  " " << tr2(text, pos2) << endl;
            break;
        }
        pos1 = pos;
        pos2 = pos;
        pos3 = pos;
        pos4 = pos;
            if ((tr1(text, pos1) != nullptr) && (tr2(text, pos2) != nullptr)) {
            if (*tr1(text, pos3) != *tr2(text, pos4)) {
                flag = 0;
                pos1 = pos;
                pos2 = pos;
                cout << "tr1(text, pos1) = " << *tr1(text, pos1) << " tr2(text, pos2) = " << *tr2(text, pos2) << endl;
                cout << "pos1 = " << pos1 << " pos2 = " << pos2 << " pos = " << pos << endl;
                break;
            }
        }
        if (!tr1(text, pos)) {
            next_tok(text, pos);
        }
    }
    for (auto& w : kwords) {
        if (*tr1(w.c_str()) != *tr2(w.c_str())) {
            flag = 0;
            cout << "tr1(w) = " << tr1(w.c_str()) << " tr2(w) = " << tr2(w.c_str()) << " w = " << w <<endl;
            break;
        }
    }

    return flag;
};

template<class Trie>
void test_trie(const std::string& name, const std::vector<std::string>& kwords, const std::string& text, int num_tries = 100, double timeout = 1) {
    double bytes_per_symbol = test_trie_memory<Trie>(kwords);
    std::cout << name << " memory :\t" << bytes_per_symbol << " bytes/symbol" << std::endl;

    double find_speed = test_trie_find_speed<Trie>(kwords, num_tries, text.c_str(), timeout);
    std::cout << name << " find  :\t" << int(find_speed) << " calls/s" << std::endl;

    double copy_speed = test_trie_copy_speed<Trie>(kwords, num_tries, timeout);
    std::cout << name << " copy  :\t" << int(copy_speed) << " calls/s" << std::endl;

    int corr = test_trie_correct<Trie>(kwords, text.c_str());
        if (corr == 1) { std::cout << name << " correct " << std::endl;}
        else { std::cout << name << " incorrect " << std::endl; }
}


template<class Trie>
void test_trie(const std::string& name, const std::string& kwfile, const std::string& file, int num_tries = 100, double timeout = 1) {
    auto kw = read_words(kwfile);
    auto text = read_file(file);
    test_trie<Trie>(name, kw, text, num_tries, timeout);
}


