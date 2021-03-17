#include "Structure.h"
using namespace std;

struct hash_pair {
    template <class T1, class T2>
    /*size_t operator()(const pair<int, int>& p) const 
    {
        return hash<uint64_t>{}((uint64_t(p.first)«32) | uint64_t(p.second));
    }*/
    
    size_t operator()(const pair<T1, T2>& p) const
    {
        std::size_t seed = 0;
        auto hash1 = hash<T1>{}(p.first);
        auto hash2 = hash<T2>{}(p.second);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
    size_t operator()(const std::pair<int, int>& p) const
    {
        return std::hash<uint64_t>{}((uint64_t(p.first) << 32) | p.second);
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


int value_dqarr(int nmb, deque<array<int, 1000>>& dq, vector<int> rand1, vector<int> rand2, vector<int> rand3)
{
    int i, j, x, y, k, s;
    k = 0;
    s = 0;
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
            x = rand1[s];
            y = rand2[s];
            if (dq.at(x)[y] != -1)
            {
                k++;
            }
            else
            {
                dq[x][y] = rand3[s];
            }
            s++;
        }
    }
    return k;
};

int value_dqumap(int nmb, deque< unordered_map<int, int>>& dq, vector<int> rand1, vector<int> rand2, vector<int> rand3)
{
    int i, j, x, y, k, s;
    k = 0;
    s = 0;
    unordered_map<int, int> umap1;
    cout << "umap1 size: " << umap1.bucket_count() * sizeof(void*) + umap1.size() * (sizeof(int) + sizeof(int) + sizeof(void*)) + sizeof(umap1) << endl;
    for (i = 0; i < nmb; i++)
    {
        dq.push_front(umap1);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand1[s];
            y = rand2[s];
            if (dq.at(x).find(y) == dq.at(x).cend())
            {
                dq[x][y] = rand3[s];
            }
            else
            {
                k++;
            }
            s++;
        } 
    }
    return k;
};

int value_dqvect(int nmb, deque<vector<int>>& dq, vector<int> rand1, vector<int> rand2, vector<int> rand3)
{
    int i, j, x, y, k, s;
    k = 0;
    s = 0;
    vector<int> vect1;
    for (i = 0; i < 1000; i++)
    {
        vect1.push_back(-1);
    }
    cout << "vect1 size: " << sizeof(int) * vect1.capacity() << endl;
    for (i = 0; i < nmb; i++)
    {
        dq.push_front(vect1);
        dq.pop_back();
        for (j = 0; j < 10; j++)
        {
            x = rand1[s];
            y = rand2[s];
            if (dq.at(x)[y] != -1)
            {
                k++;
            }
            else
            {
                dq[x][y] = rand3[s];
            }
            s++;
        }
    }
    return k;
};

int value_dqvertix(int nmb, deque<Vertix_Const>& dq)
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

int value_bigumap(int nmb, unordered_map<pair<int, int>, int, hash_pair> umap2, vector<int> rand1, vector<int> rand2, vector<int> rand3)
{
    int i, j, x, y, k, s;
    k = 0;
    s = 0;
    for (i = 0; i < nmb; i++)
    {
        for (j = 0; j < 10; j++)
        {
            x = rand1[s];
            y = rand2[s];
            pair<int, int> p1(x, y);
            if (umap2.find(p1) == umap2.cend())
            {
                umap2[p1] = rand3[s];
            }
            else
            {
                k++;
            }
            s++;
        }
    }
    cout << "big_umap size: " << umap2.bucket_count() * sizeof(void*) + umap2.size() * (2*sizeof(int) + sizeof(int) + 2*sizeof(void*)) + sizeof(umap2) << endl;
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
        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 g1(seed1);

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

        vector<int> rand1;
        vector<int> rand2;
        vector<int> rand3;

        for (int i = 0; i < nmb*10; i++)
        {
            rand1.push_back(g1() % 2000);
        }
        for (int i = 0; i < nmb * 10; i++)
        {
            rand2.push_back(g1() % 1000);
        }
        for (int i = 0; i < nmb * 10; i++)
        {
            rand3.push_back(g1() % 1000);
        }


        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        int answer1 = value_dqarr(nmb, dq1, rand1, rand2, rand3);
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        std::cout << "Time for value for deque of array: " << time_span.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
        int answer2 = value_dqumap(nmb, dq2, rand1, rand2, rand3);
        std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3);
        std::cout << "Time for value for deque of unordered map: " << time_span2.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();
        int answer3;
        answer3 = value_dqvect(nmb, dq3, rand1, rand2, rand3);
        std::chrono::high_resolution_clock::time_point t6 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span4 = std::chrono::duration_cast<std::chrono::duration<double>>(t6 - t5);
        std::cout << "Time for value for deque of integers: " << time_span4.count() << " seconds." << endl;

        std::chrono::high_resolution_clock::time_point t7 = std::chrono::high_resolution_clock::now();
        int answer4;
        answer4 = value_dqvertix(nmb, dq4);
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
        answer5 = value_bigumap(nmb, umap2, rand1, rand2, rand3);
        std::chrono::high_resolution_clock::time_point t10 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span6 = std::chrono::duration_cast<std::chrono::duration<double>>(t10 - t9);
        std::cout << "Time for value for big unordered map: " << time_span6.count() << " seconds." << endl;
        return 0;
    }