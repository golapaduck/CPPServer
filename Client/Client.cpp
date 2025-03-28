#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main() {

	// 家南 檬扁拳
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup failed" << endl;
		return 0;
	}

	// 家南 积己
	SOCKET CistenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (CistenSocket == INVALID_SOCKET) {
		cout << "socket failed" << endl;
		return 0;
	}

	cout << "Client Start" << endl;

	// 家南 楷搬 夸没
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = htons(9000);

	if (::connect(CistenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "connect failed" << endl;
		return 0;
	}

	// 单捞磐 烹脚
	while (true) {
		char sendBuffer[256] = "Test Msg";
		int sendLen = ::send(CistenSocket, sendBuffer, sizeof(sendBuffer), 0);
		if (sendLen == SOCKET_ERROR) {
			cout << "send failed" << endl;
			break;
		}

		char recvBuffer[256];
		int recvLen = ::recv(CistenSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvLen == SOCKET_ERROR) {
			cout << "recv failed" << endl;
			break;
		}
		else if (recvLen == 0) {
			cout << "server disconnected" << endl;
			break;
		}
		cout << "recv : " << recvBuffer << endl;

	}

}