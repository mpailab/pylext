#pragma once
#include <chrono>

class Timer {
    std::chrono::high_resolution_clock::time_point _start;
public:
    Timer(){ reset();}
    void reset() {
        _start = std::chrono::high_resolution_clock::now();
    }
    [[nodiscard]] double time() const {
        using namespace std::chrono;
        return duration_cast<duration<double>>(high_resolution_clock::now() - _start).count();
    }
};


