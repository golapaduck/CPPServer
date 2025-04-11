#pragma once
#include "net_common.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class tsqueue
		{
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete;
			virtual ~tsqueue() { clear(); }

		public:
			// Queue ���� ���� item�� �����ϰ�, ����
			const T& front()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.front();
			}

			// Queue ���� ���� item�� �����ϰ�, ����
			const T& back()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.back();
			}

			// Queue ���� �ڿ� item �߰�
			void push_back(const T& item)
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_back(std::move(item));
			}

			// Queue�� item�� ���� ��� true ����
			bool empty()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}
			
			// Queue�� item�� ���� ��� true ����
			size_t count()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.size();
			}

			// Queue Ŭ����
			void clear()
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.clear();
			}

			// Queue ���� ���� item�� pop�ϰ�, ����
			T pop_front()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.front());
				deqQueue.pop_front();
				return t;
			}

			// Queue ���� ���� item�� pop�ϰ�, ����
			T pop_back()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.back());
				deqQueue.pop_back();
				return t;
			}

		protected:
			std::mutex muxQueue;
			std::deque<T> deqQueue;

		};
	}
}