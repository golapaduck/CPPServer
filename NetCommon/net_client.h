#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace olc
{
	namespace net
	{
		template <typename T>
		class client_interface
		{
		public:
			client_interface()
			{}

			virtual ~client_interface()
			{
				// Client가 종료되면, 항상 서버와 disconnect 시도
				Disconnect();
			}

		public:
			// 서버에 Connection (hostname/ip-주소, port)
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Conneciton 생성
					m_connection = std::make_unique<connection<T>>(); // TODO

					// hostname/ip-주소 변환
					asio::ip::tcp::resolver resolver(m_context);
					asio::ip::tcp::resolver::results_type m_endpoints = resolver.resolve(host, std::to_string(port));


					m_connection->ConnectToServer(m_endpoints);
					
					// Context Thread 시작
					thrContext = std::thread([this]{ m_context.run(); });
					
				}
				catch (std::exception& e)
				{
					std::cerr << "Client Exception: " << e.what() << "\n";
					return false;
				}


				return false;
			}

			// Disconnect
			void Disconnect()
			{
				if (IsConnected())
				{
					m_connection->DisConnect();
				}

				// Asio Context와 thread 종료
				m_context.stop();

				if (thrContext.joinable())
					thrContext.join();

				// Connection 종료
				m_connection.release();
			}

			// 서버 연결여부 확인
			bool IsConnected()
			{
				if (m_connection)
					return m_connection->IsConnected();
				else
					return false;
			}
			
			// 서버로 메세지 전송
			void Send(const message<T>& msg)
			{
				if (IsConnected())
					m_connection->Send(msg);
			}

			// 서버로부터 들어오는 메세지 Queue 검색
			tsqueue<owned_message<T>>& Incoming()
			{
				return m_qMessageIn;
			}

		protected:
			// Asio Context로 데이터 이동
			asio::io_context m_context;
			// 작업들이 이루어질 Thread 필요
			std::thread thrContext;
			// 서버와 연결된 소켓
			asio::ip::tcp::socket m_socket;
			// 클라이언트는 Connection 오브젝트에 대한 단일 Instance를 가지고 있음
			std::unique_ptr<connection<T>> m_connection;


		private:
			// 서버로부터 들어오는 메세지에 대한 Thread Safe Queue
			tsqueue<owned_message<T>> m_qMessageIn;

		};
	}
}