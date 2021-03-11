#include "Structure.h"
int test1();
int test2();
void correct_test();
int main()
{
	int n;
	//correct_test();
	cout << "What test did you want? 1 - value; 2 - return_value" << endl;
	cin >> n;
	if (n == 1)
	{
		return test2();
	}
	if (n ==2)
	{
		return test1();
	}
}