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
#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>
#include "Structure.h"
using namespace std;

int value(int nmb, deque<array<int, 1000>>& dq)
{
    int i, j, x, y, k;
    k = 0;
    std::array<int, 1000> a;
    for (i = 0; i < 1000; i++)
    {
        a[i] = -1;
    }
    for (i = 0; i < nmb; i++)
    {
        dq.push_front(a);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand() % 2000;
            y = rand() % 1000;
            if (dq.at(x)[y] != -1)
            {
                k++;
            }
            else
            {
                dq[x][y] = rand() % 1000;
            }
        }
    }
    return k;
};

int value2(int nmb, deque< unordered_map<int, int>>& dq)
{
    int i, j, x, y, k;
    k = 0;
    unordered_map<int, int> umap1;
    for (i = 0; i < nmb; i++)
    {
        dq.push_front(umap1);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand() % 2000;
            y = rand() % 1000;
            if (dq.at(x).find(y) == dq.at(x).cend())
            {
                dq[x][y] = rand() % 1000;
            }
            else
            {
                k++;
            } 
        }
    }
    return k;
};

int value3(int nmb, deque<vector<int>>& dq)
{
    int i, j, x, y, k;
    k = 0;
    vector<int> vect1;
    for (i = 0; i < 1000; i++)
    {
        vect1.push_back(-1);
    }
    for (i = 0; i < nmb; i++)
    {
        dq.push_front(vect1);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand() % 2000;
            y = rand() % 1000;
            if (dq.at(x)[y] != -1)
            {
                k++;
            }
            else
            {
                dq[x][y] = rand() % 1000;
            }
        }
    }
    return k;
};

int value4(int nmb, deque<Vertix_Const>& dq)
{
    int i, j, x, y, k;
    k = 0;
    for (i = 0; i < nmb; i++)
    {
        Vertix_Const ver(32);
        dq.push_front(ver);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand() % 2000;
            y = rand() % 1024;
            if (dq.at(x).get(y) != -1)
            {
                k++;
            }
        }
    }
    return k;
}

int value5(int nmb, unordered_map<pair<int, int>, int, hash_pair> umap2)
{
    int i, j, x, y, k;
    k = 0;
    for (i = 0; i < nmb; i++)
    {
        for (j = 0; j < 10; j++)
        {
            x = rand() % 2000;
            y = rand() % 1000;
            pair<int, int> p1(x, y);
            if (umap2.find(p1) == umap2.cend())
            {
                umap2[p1] = rand() % 1000;
            }
            else
            {
                k++;
            }            
        }   
    }
    return k;
};

int main()
{
    using namespace std::chrono;

    int nmb;
    cout << "Give number of iterations:" << endl;
    cin >> nmb;
    cout << endl;

    for (int i = 0; i < 2000; i++)
    {
        Vertix_Const ver(32);
        dq4.push_front(ver);
    }

    for (int i = 0; i < 2000; i++)
    {
        dq2.push_front(umap);
    };

    a3.fill(-1);

    for (int i = 0; i < 2000; i++)
    {
        dq1.push_front(a3);
    };

    resize(1100, DQ_SIZE, dq3);


    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    int answer1 = value(nmb, dq1);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Time for value for array: " << time_span.count() << " seconds." << endl;

    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    int answer2 = value2(nmb, dq2);
    high_resolution_clock::time_point t4 = high_resolution_clock::now();
    duration<double> time_span2 = duration_cast<duration<double>>(t4 - t3);
    std::cout << "Time for value for unordered map: " << time_span2.count() << " seconds." << endl;

    high_resolution_clock::time_point t7 = high_resolution_clock::now();
    int answer3;
    answer3 = value3(nmb, dq3);
    high_resolution_clock::time_point t8 = high_resolution_clock::now();
    duration<double> time_span4 = duration_cast<duration<double>>(t8 - t7);
    std::cout << "Time for value for deque of integers: " << time_span4.count() << " seconds." << endl;

    high_resolution_clock::time_point t9 = high_resolution_clock::now();
    int answer4;
    answer4 = value4(nmb, dq4);
    high_resolution_clock::time_point t10 = high_resolution_clock::now();
    duration<double> time_span5 = duration_cast<duration<double>>(t10 - t9);
    std::cout << "Time for value for deque of Vertix_MAP: " << time_span5.count() << " seconds." << endl;

    for (int i = 0; i < nmb; i++)
    {
        int x, y;
        x = rand() % 2000;
        y = rand() % 1000;
        pair<int, int> p1(x, y);
        umap2[p1] = rand() % 1000;
    }

    high_resolution_clock::time_point t11 = high_resolution_clock::now();
    int answer5;
    answer5 = value5(nmb, umap2);
    high_resolution_clock::time_point t12 = high_resolution_clock::now();
    duration<double> time_span6 = duration_cast<duration<double>>(t12 - t11);
    std::cout << "Time for value for big unordered map" << time_span6.count() << " seconds." << endl;
}