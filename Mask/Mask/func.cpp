#include <immintrin.h>
#include <stdio.h>
#include <time.h> 
#include <intrin.h>
#include "cls.h"
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cmath>
#include <chrono>
#include <iostream>  
#include <iterator>     
#include <string>
#include <array>  
using namespace std;
//using namespace std::chrono;

void Complex_Test(void);
bool Correct_Test(int n,int m);
void Ver_Time_Calc();
void Map_Time_Calc();
void Mas_Time_Calc();

int main()
{
	system("pause");
	Complex_Test();
	Ver_Time_Calc();
	Map_Time_Calc();
	Mas_Time_Calc();

}


bool Correct_Test(int n,int m)
{
	bool res;
	int *vals,*index,*vales;
	int r;
	index = new int[n];
	vals = new int[1024];
	vales = new int[n];
	res = true;
	for (int i = 0; i < 1024; i++)
		vals[i] = -1;
	for (int i = 0; i < n; i++)
		{
			r = rand() % 1024;
			index[i] = r;
			vales[i] = r;
			vals [r] = r;
		}
	Vertix_Const ver1(index,vales,n);
	for (int i = 0; i < m; i++)
		{
			r = rand() % 1024;
			if (!(ver1.ret(r) == vals[r]))
			{
				res = false;
			}
		}
	delete [] vals;
	delete [] index;
	delete [] vales;

	return res;
}

void Complex_Test(void)
{
	bool res,h;
	int n,m;
	res = true;
	for (int i = 0; i < 10000; i++)
	{
		n = rand() % 1024;
		m = rand() % 100;
		h = Correct_Test(n,m);
		res = res && h;
	}
	if (res) 
		printf("Algorithm works correct\n");
	else
		printf("Algorithm works incorrect\n");
}

void Ver_Time_Calc(void)
{
	std::cout << "Start testing Vertix time :" << endl;
	using namespace std::chrono;
    srand ( time(NULL) );
	int n,*req,x,r;
	n = 10;
	Vertix_Const *verts[1024];
	for (int i = 0; i < 1024; i++)
	{
		verts[i] = new Vertix_Const(32);
	}

	for (int i = 0; i < 8; i++)
	{
		req = new int[n];
		for (int j = 0; j < n; j++)
			req[j] = rand() % 1024;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		for (int j = 0; j < n; j++)
		{
			x += verts[req[j]]->ret(req[j]);
		}	
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		std::cout << "Number of requests " << n << ". Time of working: " << time_span.count() << " seconds." << " and some x for O3 " << x  << endl;
		delete [] req;
		n = n * 10;
	}
	for (int i = 0; i < 1024; i++)
	{
		delete verts[i];
	}


}


void Map_Time_Calc(void)
{
	std::cout << "Start testing Map time :" << endl;
	using namespace std::chrono;
    srand ( time(NULL) );
	int n,*req,x;
	unordered_map<int,int> maps;
	for (int i = 0; i < 32; i++)
	{
		maps[rand()] = rand() % 100; 
	}
	n = 10;
	for (int i = 0; i < 8; i++)
	{
		req = new int[n];
		for (int j = 0; j < n; j++)
			req[j] = rand() % 1024;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		for (int j = 0; j < n; j++)
			x += maps[req[j]];
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		std::cout << "Number of requests " << n << ". Time of working: " << time_span.count() << " seconds." << " and some x for O3 " << x << endl;
		delete [] req;
		n = n * 10;
	}

}

void Mas_Time_Calc(void)
{
	std::cout << "Start testing Massive time :" << endl;
	using namespace std::chrono;
    srand ( time(NULL) );
	int n,*req,x,index[1024];
	for (int i = 0; i < 1024; i++)
	{
		index[i] = -1; 
	}
	for (int i = 0; i < 32; i++)
	{
		index[rand() % 1024] = rand() % 100; 
	}
	n = 10;
	for (int i = 0; i < 8; i++)
	{
		req = new int[n];
		for (int j = 0; j < n; j++)
			req[j] = rand() % 1024;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		for (int j = 0; j < n; j++)
			x += index[req[j]];	
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		std::cout << "Number of requests " << n << ". Time of working: " << time_span.count() << " seconds." << " and some x for O3 " << x << endl;
		delete [] req;
		n = n * 10;
	}

}