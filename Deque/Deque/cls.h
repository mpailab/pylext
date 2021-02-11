#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>
#include <time.h> 
#include <intrin.h>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cmath>
#include <chrono>
#include <iostream>  
#include <iterator>     
#include <string>
#include <array>  

class Vertix_Const
{
public:
	Vertix_Const(int n) {
		uint64_t d;
		int r, x, sum = 0;
		for (int i = 0; i < 16; i++)
		{
			mask[i] = 0;
		}
		for (int i = 0; i < n; i++)
		{
			r = rand() % 1024;
			d = 1;
			d = d << (r % 64);
			x = r / 64;
			mask[x] = mask[x] | d;
		}
		for (int i = 0; i < 16; i++)
		{
			x = __popcnt64(mask[i]);
			counts[i] = sum;
			sum += x;
		}
		values = new int[sum];
		for (int i = 0; i < sum; i++)
		{
			r = rand() % 100;
			values[i] = r;
		}
	}

	Vertix_Const(int* index, int* vals, int n)
	{
		uint64_t y;
		int d, x, sum = 0, temp;
		bool res;
		for (int i = 0; i < 16; i++)
		{
			mask[i] = 0;
		}
		for (int i = 0; i < n - 1; i++) {
			for (int j = 0; j < n - i - 1; j++) {
				if (index[j] > index[j + 1]) {
					temp = index[j];
					index[j] = index[j + 1];
					index[j + 1] = temp;
					temp = vals[j];
					vals[j] = vals[j + 1];
					vals[j + 1] = temp;
				}
			}
		}
		values = new int[n];
		x = 0;

		for (int i = 0; i < n; i++)
		{
			res = true;
			for (int j = 0; j < i; j++)
			{
				if (index[j] == index[i])
				{
					res = false;
				}
			}
			if (res)
			{
				d = index[i] / 64;
				y = 1;
				y = y << (index[i] % 64);
				mask[d] = mask[d] | y;
				values[x] = vals[i];
				x++;
			}
		}
		for (int i = 0; i < 16; i++)
		{
			x = __popcnt64(mask[i]);
			counts[i] = sum;
			sum += x;
		}
	}

	int hash(int n)
	{
		int i, cnts, d, cnts1;
		uint64_t m, t = 1;
		i = n / 64;
		d = n % 64;
		cnts = counts[i];
		m = mask[i];
		t = t << d;
		if ((t & m) == 0)
			return -1;
		return cnts + __popcnt64(m & (t - 1));
	}

	int ret(int n)
	{
		int x;
		x = hash(n);
		if (x == -1)
			return -1;
		else
			return values[x];

	}

	uint64_t mask[16];
	int counts[16];
	int* values;

	~Vertix_Const()
	{
		delete[] values;
	}
};
