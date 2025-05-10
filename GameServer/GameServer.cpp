#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <thread>
#include <atomic>
#include <mutex>

//될 때까지 대기, 상호배타적 lock / 상대방이 빨리 lock을 푼다면? good / 아니면 무한대기
//cpu 점유율이 높아짐
class SpinLock
{
public:
	void lock()
	{
		// CAS (Compare-And-Swap)
		bool expected = false;
		bool desired = true;

		// expected일 경우, desired로 변경 (아니면 false 리턴)
		while (_locked.compare_exchange_strong(expected, desired) == false)
		{
			// expected는 성공 유무와 관계없이 _locked로 반환되니 항상 false로 변경해야함
			expected = false;
		}

	}
	
	void unlock()
	{
		_locked.store(false);
	}

private:
	atomic<bool> _locked = false;
};

int32 sum = 0;
mutex m;
SpinLock spinLock;

int main()
{


}