#include "SpeedTest.h"
#include "PositionMap.h"

int main(){
    speed_test<PositionMapUM<int>>("big unordered map", 1000000,10);
    speed_test<PositionMapDA<int,1024>>("deque of arrays", 1000000, 10);
    speed_test<PositionMapDV<int>>("deque of vectors", 1000000, 10);
    speed_test<PositionMapDUM<int>>("deque of unordered map", 1000000, 10);
    speed_test<PositionMapDVC<int>>("deque of vertix", 1000000, 10);
    correct_test<PositionMapUM<int>, PositionMapDA<int, 1024>>("big unordered map", "deque of arrays", 1024, 1024);
    correct_test<PositionMapUM<int>, PositionMapDV<int>>("big unordered map", "deque of vectors", 1024, 1024);
    correct_test<PositionMapUM<int>, PositionMapDUM<int>>("big unordered map", "deque of unordered map", 1024, 1024);
    correct_test<PositionMapUM<int>, PositionMapDVC<int>>("big unordered map", "deque of vertix", 1024, 1024);
    correct_test<PositionMapDA<int, 1024>, PositionMapDV<int>>("deque of arrays", "deque of vectors", 1024, 1024);
    correct_test<PositionMapDA<int, 1024>, PositionMapDUM<int>>("deque of arrays", "deque of unordered map", 1024, 1024);
    correct_test<PositionMapDA<int, 1024>, PositionMapDVC<int>>("deque of arrays", "deque of vertix", 1024, 1024);
    correct_test<PositionMapDV<int>, PositionMapDUM<int>>("deque of vectors", "deque of unordered map", 1024, 1024);
    correct_test<PositionMapDV<int>, PositionMapDVC<int>>("deque of vectors", "deque of vertix", 1024, 1024);
    correct_test<PositionMapDUM<int>, PositionMapDVC<int>>("deque of unordered map", "deque of vertix", 1024, 1024);
    return 0;
}
