#pragma once
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
//#include <intrin.h>
#include <stdio.h>
#include <algorithm>

struct Hash64 {
    inline size_t operator()(uint64_t v) const noexcept {
        size_t x = 14695981039346656037ULL;
        for (size_t i = 0; i < 64; i += 8) {
            x ^= static_cast<size_t>((v >> i) & 255);
            x *= 1099511628211ULL;
        }
        return x;
    }
    inline size_t operator()(uint32_t v) const noexcept {
        size_t x = 14695981039346656037ULL;
        for (size_t i = 0; i < 32; i += 8) {
            x ^= static_cast<size_t>((v >> i) & 255);
            x *= 1099511628211ULL;
        }
        return x;
    }
    inline size_t operator()(int v) const noexcept {
        return (*this)(uint32_t(v));
    }
};


struct HashPair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const
    {
        std::size_t seed = 0;
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
    size_t operator()(const std::pair<int, int>& p) const {
        return Hash64{}((uint64_t(p.first)<<32) | p.second);
    }
};

int popcnt(uint64_t x) { 
    return (int)_mm_popcnt_u64(x);
}

constexpr int SIZE_OF_VERTIX = 16, BIT_SIZE = 64;


template<class V>
class Vertix_Const
{
private:
    std::array<uint64_t, SIZE_OF_VERTIX> mask;
    std::array<int, SIZE_OF_VERTIX> counts;
    std::vector<V> values;
    V _default;
public:
    /*Vertix_Const(int n) {
        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 g1(seed1);
        mask.fill(uint64_t(0));
        for (int i = 0; i < n; i++) {
            int r;
            r = g1() % SIZE_OF_VERTIX * BIT_SIZE;
            mask[r / BIT_SIZE] = mask[r / BIT_SIZE] | (uint64_t(1) << (r % BIT_SIZE));
        }
        int sum = 0;
        for (int i = 0; i < SIZE_OF_VERTIX; i++) {
            int x;
            x = popcnt(mask[i]);
            counts[i] = sum;
            sum += x;
        }
        for (int i = 0; i < sum; i++) {
            values.push_back(g1());
        }
    }*/
    Vertix_Const(V def) { _default = def; mask.fill(uint64_t(0)); }
    Vertix_Const(vector<pair<int, V>> dict,V def) {
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
            return _default;
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
            /*int i = 0;
            int c = hash(n);
            //cout << c << endl;
            //if (c <= values.size() - 1) {
                for (it = values.begin(); it <= values.end();) {
                    if (i == c)
                        break;
                    i++;
                }
                cout << c << " "  << i <<  " " << endl;
                values.insert(it, value);
                //cout << "*";
            //}
            //else {
            //    values.push_back(value);
            //}*/
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
