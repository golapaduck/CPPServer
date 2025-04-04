#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

int main() 
{
	asio::error_code ec;

	// context ����
	asio::io_context context;

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

	if (socket.is_open()) {
		
		// ���Ͽ� �����͸� ����
		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n";

		socket.write_some(asio::buffer(sRequest.data(),sRequest.size()), ec);

		// read �� ������ ���
		socket.wait(socket.wait_read);

		// ���Ͽ��� �����͸� �б�
		size_t bytes = socket.available();
		std::cout << "Bytes available: " << bytes << std::endl;

		if (bytes > 0) {
			std::vector<char> vbuffer(bytes);
			socket.read_some(asio::buffer(vbuffer.data(), vbuffer.size()), ec);

			for (auto c : vbuffer) {
				std::cout << c;
			}
		}

	}

	system("pause");
	return 0;
}