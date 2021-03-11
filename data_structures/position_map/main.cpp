#include "SpeedTest.h"
#include "PositionMap.h"

int main(){
    speed_test<PositionMapUM<int>>("big unordered map", 1000000,10);
    return 0;
}
