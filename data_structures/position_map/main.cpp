#include "SpeedTest.h"
#include "PositionMap.h"

int main(){
    speed_test<PositionMapUM<int>>("big unordered map", 1000000,10);
    speed_test<PositionMapDA<int>>("deque of arrays", 1000000, 10);
    speed_test<PositionMapDV<int>>("deque of vectors", 1000000, 10);
    speed_test<PositionMapDUM<int>>("deque of unordered map", 1000000, 10);
    speed_test<PositionMapDVC<int>>("deque of vertix", 1000000, 10);
    return 0;
}
