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

	// context 생성
	asio::io_context context;

	// 엔드포인트 생성 (make_address는 문자열 주소를 asio 포멧에 맞춰서 변경해줌)
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

	// 소켓 생성
	asio::ip::tcp::socket socket(context);

	// 소켓을 엔드포인트에 연결
	socket.connect(endpoint, ec);

	if (!ec) {
		std::cout << "Connected!" << std::endl;
	}
	else {
		std::cout << "Failed to connect to address: " << ec.message() << std::endl;
	}

	if (socket.is_open()) {
		
		// 소켓에 데이터를 쓰기
		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n";

		socket.write_some(asio::buffer(sRequest.data(),sRequest.size()), ec);

		// read 될 때까지 대기
		socket.wait(socket.wait_read);

		// 소켓에서 데이터를 읽기
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