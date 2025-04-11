#include <iostream>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

std::vector<char> vBuffer(1 * 1024); // 1KB ����

void GrabSomeData(asio::ip::tcp::socket& socket) 
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
		[&](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				std::cout << "\n\nRead " << length << " bytes\n\n";

				for (int i = 0; i < length; i++)
					std::cout << vBuffer[i];

				GrabSomeData(socket);
			}

		}
	);
}

int main() 
{
	asio::error_code ec;

	// context ����
	asio::io_context context;

	//fake task
	asio::io_context::work idleWork(context);

	std::thread thrContext = std::thread([&]() {context.run();});

	// ��������Ʈ ���� (make_address�� ���ڿ� �ּҸ� asio ���信 ���缭 ��������)
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

	// ���� ����
	asio::ip::tcp::socket socket(context);

	// ������ ��������Ʈ�� ����
	socket.connect(endpoint, ec);

	if (!ec) {
		std::cout << "Connected!" << std::endl;
	}
	else {
		std::cout << "Failed to connect to address: " << ec.message() << std::endl;
	}

	if (socket.is_open()) 
	{

		GrabSomeData(socket);
		// ���Ͽ� �����͸� ����
		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n";

		socket.write_some(asio::buffer(sRequest.data(),sRequest.size()), ec);

		
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2000ms);

		context.stop();
		if (thrContext.joinable())thrContext.join();
	}

	system("pause");
	return 0;
}