#pragma once

#if defined(__cplusplus) && __cplusplus >= 202002
#include <bit>
using std::countr_zero;
using std::countl_zero;
using std::popcount;
#elif 0 || defined(NO_INTRINSICS)
inline int countr_zero(uint64_t x) {
    int r = 0;
    uint32_t y;
    if(!(x&0xFFFFFFFF))r=32, y = uint32_t(x>>32);
    else y = uint32_t(x);
    if(!(y&0xFFFF))r+=16, y>>=16;
    if(!(y&0xFF))r+=8, y>>=8;
    if(!(y&0xF))r+=4, y>>=4;
    if(!(y&0x3))r+=2, y>>=2;
    if(!(y&0x1))r+=1, y>>=1;
    return r + !(y&1);
}
inline int countl_zero(uint64_t x) {
    int r = 0;
    uint32_t y;
    if(x&0xFFFFFFFF00000000) y = uint32_t(x>>32);
    else y = uint32_t(x>>32), r=32;
    if(y&0xFFFF0000) y>>=16; else r+=16;
    if(y&0xFF00) y>>=8; else r+=8;
    if(y&0xF0) y>>=4; else r+=4;
    if(y&0xC) y>>=2; else r+=2;
    if(y&0x2) y>>=1; else r+=1;
    return r + !(y&1);
}
inline int popcount(uint64_t x){
    constexpr uint64_t mask1 = 0x5555555555555555UL;
    constexpr uint64_t mask2 = 0x3333333333333333UL;
    constexpr uint64_t mask3 = 0x0F0F0F0F0F0F0F0FUL;
    x = (x&mask1) + (x>>1)&mask1;
    x = (x&mask2) + (x>>2)&mask2;
    x = (x+(x>>4))&mask3;
    uint32_t y = uint32_t(x) + uint32_t(x>>32);
    y += (y >> 8);
    return (y += (y>>16)) & 0xFF;
}
#else
inline int countl_lero(uint64_t x) {
    return (int)_lzcnt_u64(x);
}
inline int countr_zero(uint64_t x) {
    return (int)_tzcnt_u64(x);
}
inline int popcount(uint64_t x){
    return (int)_mm_popcnt_u64(x);
}
#endif
