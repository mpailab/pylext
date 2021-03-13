#pragma once
#include <utility>

struct Hash64 {
    inline size_t operator()(uint64_t v) const noexcept {
        size_t x = 14695981039346656037ULL;
        for (size_t i = 0; i < 64; i += 8) {
            x ^= static_cast<size_t>((v >> i) & 255);
            x *= 1099511628211ULL;
        }
        return x;
    }
    inline size_t operator()(uint32_t v) const noexcept {
        size_t x = 14695981039346656037ULL;
        for (size_t i = 0; i < 32; i += 8) {
            x ^= static_cast<size_t>((v >> i) & 255);
            x *= 1099511628211ULL;
        }
        return x;
    }
    inline size_t operator()(int v) const noexcept {
        return (*this)(uint32_t(v));
    }
};


struct HashPair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const
    {
        std::size_t seed = 0;
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
    size_t operator()(const std::pair<int, int>& p) const {
        return Hash64{}((uint64_t(p.first)<<32) | p.second);
    }
};
