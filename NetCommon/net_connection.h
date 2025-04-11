#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			connection()
			{ }

			virtual ~connection()
			{ }

		public:
			bool ConnectToServer();
			bool Disconnect();
			bool IsConnected() const;

		public:
			bool Send(const message<T>& msg);

		protected:
			// Connection���� ������ ���� ����
			asio::ip::tcp::socket m_socket;

			// ��� Asio Instance�� �����Ǵ� Context
			asio::io_context& m_asioContext;

			// �� Connection���� ���� ��� �޼����� ����Ǵ� Queue
			tsqueue<message<T>> m_qMessagesOut;

			// �� Connection���� ���� ��� �޼����� ����Ǵ� Queue
			tsqueue<owned_message>& m_qMessageIn;


		};
	}
}