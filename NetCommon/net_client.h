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
				// Client�� ����Ǹ�, �׻� ������ disconnect �õ�
				Disconnect();
			}

		public:
			// ������ Connection (hostname/ip-�ּ�, port)
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Conneciton ����
					m_connection = std::make_unique<connection<T>>(); // TODO

					// hostname/ip-�ּ� ��ȯ
					asio::ip::tcp::resolver resolver(m_context);
					asio::ip::tcp::resolver::results_type m_endpoints = resolver.resolve(host, std::to_string(port));


					m_connection->ConnectToServer(m_endpoints);
					
					// Context Thread ����
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

				// Asio Context�� thread ����
				m_context.stop();

				if (thrContext.joinable())
					thrContext.join();

				// Connection ����
				m_connection.release();
			}

			// ���� ���Ῡ�� Ȯ��
			bool IsConnected()
			{
				if (m_connection)
					return m_connection->IsConnected();
				else
					return false;
			}
			
			// ������ �޼��� ����
			void Send(const message<T>& msg)
			{
				if (IsConnected())
					m_connection->Send(msg);
			}

			// �����κ��� ������ �޼��� Queue �˻�
			tsqueue<owned_message<T>>& Incoming()
			{
				return m_qMessageIn;
			}

		protected:
			// Asio Context�� ������ �̵�
			asio::io_context m_context;
			// �۾����� �̷���� Thread �ʿ�
			std::thread thrContext;
			// ������ ����� ����
			asio::ip::tcp::socket m_socket;
			// Ŭ���̾�Ʈ�� Connection ������Ʈ�� ���� ���� Instance�� ������ ����
			std::unique_ptr<connection<T>> m_connection;


		private:
			// �����κ��� ������ �޼����� ���� Thread Safe Queue
			tsqueue<owned_message<T>> m_qMessageIn;

		};
	}
}