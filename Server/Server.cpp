#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main() {

	//家南 檬扁拳

	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup failed" << endl;
		return 0;
	}
	
	//家南 积己
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		cout << "socket failed" << endl;
		return 0;
	}

	//家南 林家 汲沥
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9000);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		return 0;
	}

	cout << "Server Start" << endl;

	//Listen
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "listen failed" << endl;
		return 0;
	}

	//Accept
	while (true) {
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int addrLen = sizeof(clientAddr);

		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {
			cout << "accept failed" << endl;
			return 0;
		}

		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
		cout << "client Connect : " << ip << endl;
		
		while (true) {
			char recvBuffer[1024];
			int recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen == 0) {
				cout << "client Disconnect : " << ip << endl;
				break;
			}
			else if (recvLen == SOCKET_ERROR) {
				cout << "recv failed" << endl;
				break;
			}
			cout << "recv : " << recvBuffer << endl;
			
			int result = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (result == SOCKET_ERROR) {
				cout << "send failed" << endl;
				break;
			}
		}
	}
}