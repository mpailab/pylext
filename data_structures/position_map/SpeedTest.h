#pragma once
#include <chrono>
#include <random>
#include <iostream>

template<class PM>
int run_posmap_speed_test(int nmb, int calls_per_shift=10) //requires IPositionMap<PM, int>
{
    unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 g1(seed1);
    int k = 0;
    PM umap{-1};
    for (int i = 0; i < nmb; i++) {
        umap.erase_before(i);
        for (int j = 0; j < calls_per_shift; j++) {
            int x = g1() % 2048;
            int y = g1() % 1024;
            int &r = umap(i + x, y);
            if (r == -1) {
                r = g1() % 1024;
            } else {
                k++;
            }
        }
    }
    return k;
}

template<class PM>
void speed_test(std::string name, int nmb, int calls_per_shift=10) {
    using namespace std;
    using namespace chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    run_posmap_speed_test<PM>(nmb, calls_per_shift);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << name << " time: " << time_span.count() << " seconds." << endl;
}

