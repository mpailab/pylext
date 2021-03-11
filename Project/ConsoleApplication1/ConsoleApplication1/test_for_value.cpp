#include "Structure.h"
using namespace std;


struct hash_pair {
    template <class T1, class T2>
    /*size_t operator()(const pair<int, int>& p) const 
    {
        return hash<uint64_t>{}((uint64_t(p.first)«32) | uint64_t(p.second));
    }*/
    size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = hash<uint64_t>{}(p.first);
        auto h2 = hash<uint64_t>{}(p.second);
        return h1<<32 | h2;
    }
};

void resize2(int size, int dq_size, deque<vector<int>>& dq)
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




    int test2()
    {
        deque<array<int, 1000>> dq1;
        array<int, 1000> a3;
        deque<unordered_map<int, int>> dq2;
        deque<vector<int>> dq3;
        unordered_map<int, int> umap;
        unordered_map<pair<int, int>, int, hash_pair> umap2;

        deque<Vertix_Const> dq4;
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

        resize2(1100, DQ_SIZE, dq3);


        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int answer1 = value(nmb, dq1);
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        std::cout << "Time for value for array: " << time_span.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        int answer2 = value2(nmb, dq2);
        std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3);
        std::cout << "Time for value for unordered map: " << time_span2.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();
        int answer3;
        answer3 = value3(nmb, dq3);
        std::chrono::high_resolution_clock::time_point t6 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span4 = std::chrono::duration_cast<std::chrono::duration<double>>(t6 - t5);
        std::cout << "Time for value for deque of integers: " << time_span4.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t7 = std::chrono::high_resolution_clock::now();
        int answer4;
        answer4 = value4(nmb, dq4);
        std::chrono::high_resolution_clock::time_point t8 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span5 = std::chrono::duration_cast<std::chrono::duration<double>>(t8 - t7);
        std::cout << "Time for value for deque of Vertix_MAP: " << time_span5.count() << " seconds." << endl;

        for (int i = 0; i < nmb; i++)
        {
            int x, y;
            x = rand() % 2000;
            y = rand() % 1000;
            pair<int, int> p1(x, y);
            umap2[p1] = rand() % 1000;
        }

        std::chrono::high_resolution_clock::time_point t9 = std::chrono::high_resolution_clock::now();
        int answer5;
        answer5 = value5(nmb, umap2);
        std::chrono::high_resolution_clock::time_point t10 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span6 = std::chrono::duration_cast<std::chrono::duration<double>>(t10 - t9);
        std::cout << "Time for value for big unordered map: " << time_span6.count() << " seconds." << endl;
    }