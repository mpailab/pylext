#include "Structure.h"
using namespace std;


void resize1(int size, int dq_size, deque<vector<int>>& dq)
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

int return_value(array<int, 1000> a, deque<array<int, 1000>>& dq, int x, int y)
{
    if (x < dq.size())
    {
        return dq.at(x)[y];
    }
    else
    {
        int s = dq.size();
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.push_front(a);
        }
        return dq.at(x)[y];
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.pop_back();
        }
    }
};

int return_value2(unordered_map<int, int> umap, deque< unordered_map<int, int>>& dq, int x, int y)
{
    if (x < dq.size())
    {
        return dq.at(x)[y];
    }
    else
    {
        int s = dq.size();
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.push_front(umap);
        }
        return dq.at(x)[y];
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.pop_back();
        }
    }
};

int return_value3(vector<int> vect1, deque< vector<int> >& dq, int x, int y)
{
    if (x < dq.size())
    {
        return dq.at(x)[y];
    }
    else
    {
        int s = dq.size();
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.push_front(vect1);
        }
        return dq.at(x)[y];
        for (int j = 0; j < x - s + 1; j++)
        {
            dq.pop_back();
        }
    }
};



    int test1()
    {
        deque<array<int, 1000>> dq1;
        array<int, 1000> a3;
        deque<unordered_map<int, int>> dq2;
        deque<vector<int>> dq3;
        unordered_map<int, int> umap;
        unordered_map<pair<int, int>, int, hash_pair> umap2;
        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        mt19937 g1(seed1);

        deque<Vertix_Const> dq4;
        using namespace std::chrono;
        int nmb;
        cout << "Give number of iterations:" << endl;
        cin >> nmb;
        cout << endl;

        vector<int> rand1;
        vector<int> rand2;

        for (int i = 0; i < nmb; i++)
        {
            rand1.push_back(g1() % 3000);
        }
        for (int i = 0; i < nmb; i++)
        {
            rand2.push_back(g1() % 1000);
        }

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

        resize1(1100, DQ_SIZE, dq3);

        vector<int> vect1;
        for (int i = 0; i < 1000; i++)
        {
            vect1.push_back(-1);
        }


        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        int ans, x, y;
        for (int i = 0; i < nmb; i++)
        {
            x = rand1[i];
            y = rand2[i];
            ans = return_value(a3, dq1, x, y);
        }
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        std::cout << "Time for return_value for array: " << time_span.count() << " seconds." << endl;

        high_resolution_clock::time_point t3 = high_resolution_clock::now();
        int ans2;
        for (int i = 0; i < nmb; i++)
        {
            x = rand1[i];
            y = rand2[i];
            ans2 = return_value2(umap, dq2, x, y);
        }
        high_resolution_clock::time_point t4 = high_resolution_clock::now();
        duration<double> time_span2 = duration_cast<duration<double>>(t4 - t3);
        std::cout << "Time for return_value for unordered map: " << time_span2.count() << " seconds." << endl;

        high_resolution_clock::time_point t5 = high_resolution_clock::now();
        int ans3;
        for (int i = 0; i < nmb; i++)
        {
            x = rand1[i];
            y = rand2[i];
            ans3 = return_value3(vect1, dq3, x, y);
        }
        high_resolution_clock::time_point t6 = high_resolution_clock::now();
        duration<double> time_span3 = duration_cast<duration<double>>(t6 - t5);
        std::cout << "Time for return_value for vector: " << time_span3.count() << " seconds." << endl;

    }