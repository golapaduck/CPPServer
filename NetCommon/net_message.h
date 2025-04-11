#pragma once
#include "net_common.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		struct message_header
		{
			T id{};
			uint32_t size = 0;
		};

		template <typename T>
		struct message
		{
			message_header<T> header{};
			std::vector<uint8_t> body;

			// 사이즈 리턴
			size_t size() const
			{
				return sizeof(message_header<T>) + body.size();
			}

			// std::cout 오버라이드
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
			{
				os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}

			// 메세지 버퍼에 데이터 push (template<타입>에 따라 타입지정 가능)
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				// 데이터 타입 체크 (is_standard_layout으로 POD-like인지 체크)
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// 현재 벡터 사이즈 캐시
				size_t i = msg.body.size(); 

				// 벡터 사이즈 재설정
				msg.body.resize(msg.body.size() + sizeof(DataType));

				// 새로 생긴 벡터 공간에 데이터 copy (Physically copy)
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// 메세지 사이즈 다시 계산
				msg.header.size = msg.size();

				// 메세지 return ("chained" 가 가능하다면)
				return msg;
			}

			// 메세지 버퍼에서 데이터 pull
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data)
			{
				// 데이터 타입 체크 (is_standard_layout으로 POD-like인지 체크)
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// 데이터 시작 부분 캐시
				size_t i = msg.body.size() - sizeof(DataType);
				
				// 벡터 -> 변수로 데이터 카피
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// 벡터에서 읽은 데이터만큼 사이즈 줄이기
				msg.body.resize(i);

				// 메세지 return ("chained" 가 가능하다면)
				return msg;
			}
		};

		template <typename T>
		class connection;

		template <typename T>
		struct owned_message
		{
			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg)
			{
				os << msg.msg;
				return os;
			}
		};
	}
}