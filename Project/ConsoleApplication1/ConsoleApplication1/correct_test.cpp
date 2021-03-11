#include "Structure.h"


constexpr int CHECK_ITER = 10000;
using namespace std;

void correct_test() {
	unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 g1(seed1);
	array<int, BIT_SIZE* SIZE_OF_VERTIX> buf;
	buf.fill(-1);
	vector<pair<int, int>> di;
	Vertix_Const ver(di);
	for (int i = 0; i < CHECK_ITER; i++) {
		int ind, val;
		ind = g1() % BIT_SIZE * SIZE_OF_VERTIX;
		val = g1();
		buf[ind] = val;
		ver.add(ind, val);
		ind = g1() % BIT_SIZE * SIZE_OF_VERTIX;
		buf[ind] = -1;
		ver.del(ind);
	}
	bool res = true;
	for (int i = 0; i < BIT_SIZE * SIZE_OF_VERTIX; i++) {
		if (buf[i] != ver.get(i))
			res = false;
	}
	if (res)
		cout << "All correct\n";
	else
		cout << "Something wrong\n";

}