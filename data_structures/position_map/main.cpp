#include "SpeedTest.h"
#include "PositionMap.h"

int main(){
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    testpm<PositionMapUM<int>>("big unordered map", 1000000,10,seed);
    testpm<PositionMapDA<int,1024>>("deque of arrays", 1000000, 10,seed);
    testpm<PositionMapDV<int>>("deque of vectors", 1000000, 10,seed);
    testpm<PositionMapDUM<int>>("deque of unordered map", 1000000, 10,seed);
    testpm<PositionMapDVC<int>>("deque of vertix", 1000000, 10,seed);
    correct_test<PositionMapUM<int>, PositionMapDA<int, 1024>>("big unordered map", "deque of arrays", 1024, 1024,seed);
    correct_test<PositionMapUM<int>, PositionMapDV<int>>("big unordered map", "deque of vectors", 1024, 1024,seed);
    correct_test<PositionMapUM<int>, PositionMapDUM<int>>("big unordered map", "deque of unordered map", 1024, 1024,seed);
    correct_test<PositionMapUM<int>, PositionMapDVC<int>>("big unordered map", "deque of vertix", 1024, 1024,seed);

    return 0;
}
