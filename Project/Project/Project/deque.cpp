/*#include <iostream>
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



class Vertix_Const
{
public:
    Vertix_Const(int n) {
        uint64_t d;
        int r, x, sum = 0;
        for (int i = 0; i < 16; i++)
        {
            mask[i] = 0;
        }
        for (int i = 0; i < n; i++)
        {
            r = rand() % 1024;
            d = 1;
            d = d << (r % 64);
            x = r / 64;
            mask[x] = mask[x] | d;
        }
        for (int i = 0; i < 16; i++)
        {
            x = __popcnt64(mask[i]);
            counts[i] = sum;
            sum += x;
        }
        values = new int[sum];
        for (int i = 0; i < sum; i++)
        {
            r = rand() % 100;
            values[i] = r;
        }
    }

    Vertix_Const(int* index, int* vals, int n)
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
        uint64_t m, t = 1;
        i = n / 64;
        d = n % 64;
        cnts = counts[i];
        m = mask[i];
        t = t << d;
        if ((t & m) == 0)
            return -1;
        return cnts + __popcnt64(m & (t - 1));
    }

    int ret(int n)
    {
        int x;
        x = hash(n);
        if (x == -1)
            return -1;
        else
            return values[x];

    }

    uint64_t mask[16];
    int counts[16];
    int* values;


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

void shift_right(int s, array<int, 1000> a, deque<array<int, 1000>>& dq)
{
    int j;
    for (j = 0; j < s; j++)
    {
        dq.push_front(a);
        dq.pop_back();
    }
};

void shift_left(int s, array<int, 1000> a, deque<array<int, 1000>>& dq)
{
    int j;
    for (j = 0; j < s; j++)
    {
        dq.push_back(a);
        dq.pop_front();
    }
};

void pop_left(int s, array<int, 1000> a, deque<array<int, 1000>>& dq)
{
    int j;
    for (j = 0; j < s; j++)
    {
        dq.pop_back();
    }
};


int return_value(array<int, 1000> a, deque<array<int, 1000>>& dq, int x, int y)
{
    int j;
    if (x < dq.size())
    {
        return dq.at(x)[y];
    }
    else
    {
        int s = dq.size();
        for (j = 0; j < x - s + 1; j++)
        {
            dq.push_front(a);
        }
        return dq.at(x)[y];
    }
};


void shift_right2(int s, unordered_map<int, int> umap, deque< unordered_map<int, int>>& dq)
{
    int j;
    for (j = 0; j < s; j++)
    {
        dq.push_front(umap);
        dq.pop_back();
    }
};

void shift_left2(int s, unordered_map<int, int> umap, deque< unordered_map<int, int>>& dq)
{
    int j;
    for (j = 0; j < s; j++)
    {
        dq.push_back(umap);
        dq.pop_front();
    }
};


int return_value2(unordered_map<int, int> umap, deque< unordered_map<int, int>>& dq, int x, int y)
{
    int j;
    if (x < dq.size())
    {
        return dq.at(x)[y];
    }
    else
    {
        int s = dq.size();
        for (j = 0; j < x - s + 1; j++)
        {
            dq.push_front(umap);
        }
        return dq.at(x)[y];
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



int main()
{
    using namespace std::chrono;
    srand(time(NULL));
    int i, j;
    int dq_size = 2000;
    double sec = 0;
    std::deque<array<int, 1000>> dq1;
    std::array<int, 1000> a3;
    std::deque<unordered_map<int, int>> dq2;
    std::deque<vector<int>> dq3;

    unordered_map<int, int> umap;

    unordered_map<pair<int, int>, int, hash_pair> umap2;

    std::deque<Vertix_Const> dq4;
    for (i = 0; i < 2000; i++)
    {
        Vertix_Const ver(32);
        dq4.push_front(ver);
    }
    for (i=0;i<1000;i++)
    {
            umap[i] = -1;
    }
    for (i = 0; i < 2000; i++)
    {
        dq2.push_front(umap);
    };

    a3.fill(-1);

    for (i = 0; i < 2000; i++)
    {
        dq1.push_front(a3);
    };

    resize(1000, dq_size, dq3);

    cout << "My deque of array size before value: " << dq1.size() << endl;
    cout << "My deque of unordered map size before value: " << dq2.size() << endl;
    cout << "My deque of vector size before value: " << dq3.size() << endl;
    cout << "My deque of vector size before value: " << dq4.size() << endl;
    int nmb;
    cout << "Give number of iterations:" << endl;
    cin >> nmb;
    cout << endl;


    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    shift_right(400, a3, dq1);
    shift_left(200, a3, dq1);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Time for shift for array: " << time_span.count() << " seconds." << endl;


    t1 = high_resolution_clock::now();
    int answer1 = value(nmb, dq1);
    t2 = high_resolution_clock::now();
    time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Time for value for array: " << time_span.count() << " seconds." << endl;


    t1 = high_resolution_clock::now();
    int ans, x, y;
    for (i = 0; i < 1000; i++)
    {
        x = rand() % 3000;
        y = rand() % 1000;
        ans = return_value(a3, dq1, x, y);
    }
    t2 = high_resolution_clock::now();
    time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Time for return_value for array: " << time_span.count() << " seconds." << endl;
    cout << "k1 = " << answer1 << endl;
    cout << endl;


    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    shift_right2(400, umap, dq2);
    shift_left2(200, umap, dq2);
    high_resolution_clock::time_point t4 = high_resolution_clock::now();
    duration<double> time_span2 = duration_cast<duration<double>>(t4 - t3);
    std::cout << "Time for shift for unordered map: " << time_span2.count() << " seconds." << endl;


    t3 = high_resolution_clock::now();
    int answer2 = value2(nmb, dq2);
    t4 = high_resolution_clock::now();
    time_span2 = duration_cast<duration<double>>(t4 - t3);
    std::cout << "Time for value for unordered map: " << time_span2.count() << " seconds." << endl;


    t3 = high_resolution_clock::now();
    int ans2;
    for (i = 0; i < 1000; i++)
    {
        x = rand() % 3000;
        y = rand() % 1000;
        ans2 = return_value2(umap, dq2, x, y);
    }
    t4 = high_resolution_clock::now();
    time_span2 = duration_cast<duration<double>>(t4 - t3);
    std::cout << "Time for return_value for unordered map: " << time_span2.count() << " seconds." << endl;
    cout << "k2 = " << answer2 << endl;
    cout << endl;


    high_resolution_clock::time_point t5 = high_resolution_clock::now();
    for (i = 0; i < nmb; i++)
    {
        x = rand() % 2000;
        y = rand() % 1000;
        pair<int, int> p1(x, y);
        umap2[p1] = rand() % 1000;
    }
    high_resolution_clock::time_point t6 = high_resolution_clock::now();
    duration<double> time_span3 = duration_cast<duration<double>>(t6 - t5);
    std::cout << "Time to fill big unordered map: " << time_span3.count() << " seconds." << endl << endl;


    high_resolution_clock::time_point t7 = high_resolution_clock::now();
    resize(1100, dq_size, dq3);
    high_resolution_clock::time_point t8 = high_resolution_clock::now();
    duration<double> time_span4 = duration_cast<duration<double>>(t8 - t7);
    std::cout << "Time to resize deque of vector: " << time_span4.count() << " seconds." << endl;


    t7 = high_resolution_clock::now();
    int ans3;
    for (i = 0; i < 1000; i++)
        ans3 = value3(nmb, dq3);
    t8 = high_resolution_clock::now();
    time_span2 = duration_cast<duration<double>>(t8 - t7);
    std::cout << "Time for value for deque of integers: " << time_span4.count() << " seconds." << endl;
    cout << "k3 = " << ans3 << endl;
    cout << endl;



    high_resolution_clock::time_point t9 = high_resolution_clock::now();
    int ans4;
    for (i = 0; i < 1000; i++)
        ans4 = value4(nmb, dq4);
    high_resolution_clock::time_point t10 = high_resolution_clock::now();
    duration<double> time_span5 = duration_cast<duration<double>>(t10 - t9);
    std::cout << "Time for value for deque of Vertix_MAP: " << time_span5.count() << " seconds." << endl;
    cout << "k4 = " << ans4 << endl;
    cout << endl;

    cout << "My deque of array size after value: " << dq1.size() << endl;
    cout << "My deque of unordered map size after value: " << dq2.size() << endl;
    cout << "My deque of vector size after value: " << dq3.size() << endl;
    cout << "My deque of vector size after value: " << dq4.size() << endl;
    system("pause");
    return 0;
}
*/
