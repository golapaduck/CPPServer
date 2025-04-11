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

			// ������ ����
			size_t size() const
			{
				return sizeof(message_header<T>) + body.size();
			}

			// std::cout �������̵�
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
			{
				os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}

			// �޼��� ���ۿ� ������ push (template<Ÿ��>�� ���� Ÿ������ ����)
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				// ������ Ÿ�� üũ (is_standard_layout���� POD-like���� üũ)
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// ���� ���� ������ ĳ��
				size_t i = msg.body.size(); 

				// ���� ������ �缳��
				msg.body.resize(msg.body.size() + sizeof(DataType));

				// ���� ���� ���� ������ ������ copy (Physically copy)
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// �޼��� ������ �ٽ� ���
				msg.header.size = msg.size();

				// �޼��� return ("chained" �� �����ϴٸ�)
				return msg;
			}

			// �޼��� ���ۿ��� ������ pull
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data)
			{
				// ������ Ÿ�� üũ (is_standard_layout���� POD-like���� üũ)
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

				// ������ ���� �κ� ĳ��
				size_t i = msg.body.size() - sizeof(DataType);
				
				// ���� -> ������ ������ ī��
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// ���Ϳ��� ���� �����͸�ŭ ������ ���̱�
				msg.body.resize(i);

				// �޼��� return ("chained" �� �����ϴٸ�)
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