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
using namespace std;

int main()
{
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
    duration<double> time_span2 = duration_cast<duration<double>>(t8 - t7);
    std::cout << "Time for value for deque of integers: " << time_span4.count() << " seconds." << endl;

    high_resolution_clock::time_point t9 = high_resolution_clock::now();
    int answer4;
    for (i = 0; i < 1000; i++)
        answer4 = value4(nmb, dq4);
    high_resolution_clock::time_point t10 = high_resolution_clock::now();
    duration<double> time_span5 = duration_cast<duration<double>>(t10 - t9);
    std::cout << "Time for value for deque of Vertix_MAP: " << time_span5.count() << " seconds." << endl;
}
