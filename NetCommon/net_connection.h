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
			// Connection별로 고유의 소켓 생성
			asio::ip::tcp::socket m_socket;

			// 모든 Asio Instance와 공유되는 Context
			asio::io_context& m_asioContext;

			// 이 Connection에서 보낼 모든 메세지가 저장되는 Queue
			tsqueue<message<T>> m_qMessagesOut;

			// 이 Connection에서 받을 모든 메세지가 저장되는 Queue
			tsqueue<owned_message>& m_qMessageIn;


		};
	}
}