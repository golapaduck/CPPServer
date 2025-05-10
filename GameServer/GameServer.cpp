#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>

atomic<int32> sum = 0;

void Add()
{
	for (int32 i = 0; i < 100'0000; i++)
	{
		sum.fetch_add(1);
	}
}

void Sub()
{
	for (int32 i = 0; i < 100'0000; i++)
	{
		sum.fetch_sub(1);
	}

}
int main()
{
	std::thread t1(Add);
	std::thread t2(Sub);
	t1.join();
	t2.join();
	cout << sum << endl;
}
