#include <iostream>
#include <deque>    
#include <iterator>     
#include <string>  
#include <array>
#include <unordered_map>
#include <chrono>
#include <utility>
//#include <bits/stdc++.h>
#include <stdint.h>
#include <cmath>
#include <vector>
#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>

using namespace std;

constexpr int SIZE_OF_VERTIX = 16, BIT_SIZE = 64;
constexpr int DQ_SIZE = 2000;


class Vertix_Const
{
private:
    std::array<uint64_t, SIZE_OF_VERTIX> mask;
    std::array<int, SIZE_OF_VERTIX> counts;
    std::vector<int> values;
public:
    Vertix_Const(int n) {
        int sum = 0;
        mask.fill(uint64_t(0));
        for (int i = 0; i < n; i++)
        {
            int r;
            r = rand() % SIZE_OF_VERTIX * BIT_SIZE;
            mask[r / BIT_SIZE] = mask[r / BIT_SIZE] | (uint64_t(1) << (r % BIT_SIZE));
        }
        for (int i = 0; i < SIZE_OF_VERTIX; i++)
        {
            int x;
            x = __popcnt64(mask[i]);
            counts[i] = sum;
            sum += x;
        for (int i = 0; i < sum; i++)
        {
            values.push_back(rand());
        }
    }

    Vertix_Const(array<pair<int,int>, int n> dict)
    {
        uint64_t y;
        int d, x, sum = 0, temp;
        bool res;
        for (int i = 0; i < 16; i++)
        {
            mask[i] = 0;
        }
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                if (index[j] > index[j + 1]) {
                    temp = index[j];
                    index[j] = index[j + 1];
                    index[j + 1] = temp;
                    temp = vals[j];
                    vals[j] = vals[j + 1];
                    vals[j + 1] = temp;
                }
            }
        }
        values = new int[n];
        x = 0;

        for (int i = 0; i < n; i++)
        {
            res = true;
            for (int j = 0; j < i; j++)
            {
                if (index[j] == index[i])
                {
                    res = false;
                }
            }
            if (res)
            {
                d = index[i] / 64;
                y = 1;
                y = y << (index[i] % 64);
                mask[d] = mask[d] | y;
                values[x] = vals[i];
                x++;
            }
        }
        for (int i = 0; i < 16; i++)
        {
            x = __popcnt64(mask[i]);
            counts[i] = sum;
            sum += x;
        }
    }

    int hash(int n)
    {
        int i, cnts, d, cnts1;
        uint64_t m, t;
        i = n / 64;
        d = n % 64;
        cnts = counts[i];
        m = mask[i];
        t = uint64_t(1) << d;
        if ((t & m) == 0)
            return -1;
        return cnts + __popcnt64(m & (t - 1));
    }

    int get(int n)
    {
        int x;
        x = hash(n);
        if (x == -1)
            return -1;
        else
            return values[x];
    }

  



}; 

std::deque<array<int, 1000>> dq1;
std::array<int, 1000> a3;
std::deque<unordered_map<int, int>> dq2;
std::deque<vector<int>> dq3;

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

unordered_map<int, int> umap;
unordered_map<pair<int, int>, int, hash_pair> umap2;

std::deque<Vertix_Const> dq4;