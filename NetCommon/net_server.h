#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		class server_interface
		{
		public:
			server_interface(uint16_t port)
				: m_asioAcceptor(m_asioContext,asio::ip::tcp::endpoint(asio::ip::tcp::v4(),port))
			{

			}

			virtual ~server_interface()
			{
				Stop();
			}
			
			bool Start()
			{
				try
				{
					WaitForClientConnection();

					m_threadContext = std::thread([this]() { m_asioContext.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "[SERVER] Exception: " << e.what() << "\n";
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			void Stop()
			{
				// Context에 종료 요청
				m_asioContext.stop();

				// Context Thread 종료
				if (m_threadContext.joinable()) m_threadContext.join();

				std::cout << "[SERVER] Stopped! \n";
			} 

			// 비동기 방식
			void WaitForClientConnection()
			{
				m_asioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{

						if (!ec)
						{
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
							
							//std::shared_ptr<connection<T>> newconn =
							//	std::make_shared<connection<T>>(
							//		connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);
							//
							//// 서버에서 연결을 거절할 수 있게 구성
							//if (OnClientConnect(newconn))
							//{
							//	m_deqConnections.push_back(std::move(newconn));

							//	m_deqConnections.back()->ConnectToClient(nIDCounter++);

							//	std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
							//}
							//else
							//{
							//	std::cout << "[-----] Connection Denied\n";
							//}
						}
						else
						{
							std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
						}

						// 다음 연결
						WaitForClientConnection();
					});
			}
			
			// 특정 클라이언트에 메세지 전송
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				if (client && client->IsConnected())
				{
					client->Send(msg);
				}
				else
				{
					OnClientDisconnect(client);
					client.reset();
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
				}
			}
			// 모든 클라이언트에 메세지 전송 (특정 클라이언트 제외 가능)
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;

				for (auto& client : m_deqConnections)
				{
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
							client->Send(msg);
					}
					else
					{
						OnClientDisconnect(client);
						client.reset();
						bInvalidClientExists = true;
					}
				}
				if (bInvalidClientExists)
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
			}

			void Update(size_t nMaxMessages = -1)
			{
				size_t nMessageCount = 0;
				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
				{
					auto msg = m_qMessagesIn.pop_front();

					OnMessage(msg.remote, msg.msg);

					nMessageCount++;
					 
				}
			}
		protected:
			// 클라이언트가 연결되었을 때 호출, false를 리턴해서 거절 가능
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// 클라이언트가 연결 해제되었을 때 호출
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{

			}

			// 메세지를 받았을 때, 호출
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
			{

			}
		protected:
			// 들어오는 메세지 패킷을 위한 Thread Sasfe Queue
			tsqueue<owned_message<T>> m_qMessagesIn;

			// 인증 완료된 connection들의 컨테이너
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			// 순서 중요
			asio::io_context m_asioContext;
			std::thread m_threadContext;

			// Asio Context 필요
			asio::ip::tcp::acceptor m_asioAcceptor;

			// 클라이언트의 고유 ID 배정을 위한 카운터
			uint32_t nIDCounter = 10000;
		};
	}
}