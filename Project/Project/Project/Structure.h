#include <iostream>
#include <deque>    
#include <iterator>     
#include <string>  
#include <array>
#include <unordered_map>
#include <chrono>
#include <utility>
#include <stdint.h>
#include <cmath>
#include <vector>
#include <random>
#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>
#include <algorithm>

using namespace std;

constexpr int SIZE_OF_VERTIX = 16, BIT_SIZE = 64;
constexpr int DQ_SIZE = 2000;


class Vertix_Const
{
private:
    array<uint64_t, SIZE_OF_VERTIX> mask;
    array<int, SIZE_OF_VERTIX> counts;
    vector<int> values;
public:
    Vertix_Const(int n) {
        mask.fill(uint64_t(0));
        for (int i = 0; i < n; i++) {
            int r;
            r = rand() % SIZE_OF_VERTIX * BIT_SIZE;
            mask[r / BIT_SIZE] = mask[r / BIT_SIZE] | (uint64_t(1) << (r % BIT_SIZE));
        }
        int sum = 0;
        for (int i = 0; i < SIZE_OF_VERTIX; i++) {
            int x;
            x = __popcnt64(mask[i]);
            counts[i] = sum;
            sum += x;
        }
        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 g1(seed1);
        for (int i = 0; i < sum; i++) {
            values.push_back(g1());
        }
    }

    Vertix_Const(const vector<pair<int,int>> &dict) {
        mask.fill(uint64_t(0));
        //sort(dict.begin(), dict.end(), [](pair<int, int> x, pair<int, int> y) { return x.first < y.first; });
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
        for (int i = 0; i < SIZE_OF_VERTIX; i++)
        {
            int x = __popcnt64(mask[i]);
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
        return cnts + __popcnt64(m & (t - 1));
    }

    int get(int n) {
        int x = hash(n);
        if (x == -1)
            return -1;
        else
            return values[x];
    }
    
    void add(int n,int value) {
        int x = hash(n), d;
        if (x != -1) {
            values[x] = value;
        } else {
            d = n / BIT_SIZE;
            mask[d] = mask[d] | uint64_t(1) << (n % BIT_SIZE);
            int sum = 0;
            for (int i = 0; i < SIZE_OF_VERTIX; i++)
            {
                int x = __popcnt64(mask[i]);
                counts[i] = sum;
                sum += x;
            }
            values.insert(values.begin() + hash(n),value);
        }
        
    }

    void del(int n) {
        int x = hash(n), d;
        if (x != -1) {
            d = n / BIT_SIZE;
            mask[d] = mask[d] | ~(uint64_t(1) << (n % BIT_SIZE));
            int sum = 0;
            for (int i = 0; i < SIZE_OF_VERTIX; i++)
            {
                int x = __popcnt64(mask[i]);
                counts[i] = sum;
                sum += x;
            }
            values.erase(values.begin() + hash(n));
        }
    }
  
}; 

void resize(int size, int dq_size, deque<vector<int>>& dq)
{
    int i, j;
    if (dq.size() == 0)
    {
    std:vector<int> vect;
        for (i = 0; i < size; i++)
        {
            vect.push_back(-1);
        }
        for (i = 0; i < dq_size; i++)
        {
            dq.push_front(vect);
        }
    }
    else
    {
        for (i = 0; i < size; i++)
        {
            int sz = dq[i].size();
            dq[i].resize(size);
            for (j = 0; j < dq[i].size() - sz; j++)
            {
                dq[i][j] = -1;
            }
        }
    }
};


struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const pair<T1, T2>& p) const
    {
        std::size_t seed = 0;
        auto hash1 = hash<T1>{}(p.first);
        auto hash2 = hash<T2>{}(p.second);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

deque<array<int, 1000>> dq1;
array<int, 1000> a3;
deque<unordered_map<int, int>> dq2;
deque<vector<int>> dq3;
unordered_map<int, int> umap;
unordered_map<pair<int, int>, int, hash_pair> umap2;

deque<Vertix_Const> dq4;