# 커밋 상세: NetAddress 및 SocketUtils 클래스 추가 (YYYY-MM-DD)

**날짜:** YYYY-MM-DD

**커밋 메시지:**
```
feat: NetAddress 및 SocketUtils 클래스 추가를 통한 네트워크 기능 확장

- NetAddress 클래스 추가:
  - IP 주소 및 포트 번호 관리를 위한 SOCKADDR_IN 래퍼 클래스
  - 다양한 생성자 및 주소 정보 접근자 제공
  - Ip2Address 유틸리티 함수 포함 (InetPtonW 사용)

- SocketUtils 클래스 추가:
  - Winsock 초기화(WSAStartup) 및 정리(WSACleanup) 기능
  - TCP 소켓 생성 (WSASocket) 및 닫기 기능
  - 다양한 소켓 옵션 설정 함수 (SO_LINGER, SO_REUSEADDR, SO_RCVBUF, SO_SNDBUF, TCP_NODELAY, SO_UPDATE_ACCEPT_CONTEXT)
  - 주소 바인딩(bind, INADDR_ANY 지원) 및 리슨(listen) 기능
  - Winsock 확장 함수(ConnectEx, DisconnectEx, AcceptEx) 로딩 및 관리 (WSAIoctl, SIO_GET_EXTENSION_FUNCTION_POINTER 사용)
  - 소켓 옵션 설정을 위한 제네릭 템플릿 함수 SetSockOpt 제공

- 기존 코드 수정:
  - CoreGlobal: SocketUtils::Init() 및 Clear() 호출 추가
  - GameServer: 새로운 네트워크 클래스 사용을 위한 준비 (향후 실제 사용 예정)
  - Types.h, CorePch.h: 새 클래스 및 헤더 포함
  - .vcxproj, .vcxproj.filters: 새 소스 파일 추가
```

**주요 변경 C++ 파일:**
*   `ServerCore/NetAddress.h` (추가)
*   `ServerCore/NetAddress.cpp` (추가)
*   `ServerCore/SocketUtils.h` (추가)
*   `ServerCore/SocketUtils.cpp` (추가)
*   `ServerCore/CoreGlobal.cpp` (수정)
*   `ServerCore/CorePch.h` (수정)
*   `ServerCore/Types.h` (수정)
*   `GameServer/GameServer.cpp` (수정)
*   `ServerCore/ServerCore.vcxproj` (수정)
*   `ServerCore/ServerCore.vcxproj.filters` (수정)

**상세 변경 내용:**

1.  **`NetAddress` 클래스 신규 추가 (`ServerCore/NetAddress.h`, `ServerCore/NetAddress.cpp`):**
    *   네트워크 주소 정보(IPv4 주소 및 포트)를 캡슐화하고 관리합니다.
    *   `SOCKADDR_IN` 구조체를 기반으로 하며, 사용 편의성을 위한 생성자(기본, `SOCKADDR_IN` 복사, IP/Port 문자열)와 멤버 함수(`GetSockAddr`, `GetIpAddress`, `GetPort`)를 제공합니다.
    *   `Ip2Address` 정적 유틸리티 함수를 통해 와이드 문자열 IP를 `IN_ADDR` 구조체로 변환합니다 (`InetPtonW` 사용).

2.  **`SocketUtils` 클래스 신규 추가 (`ServerCore/SocketUtils.h`, `ServerCore/SocketUtils.cpp`):**
    *   Winsock API 사용을 위한 정적 유틸리티 함수 모음입니다.
    *   **초기화/정리**: `Init()` (WSAStartup, 확장 함수 로딩), `Clear()` (WSACleanup).
    *   **확장 함수 로딩**: `BindWindowsFunction()` (WSAIoctl, SIO_GET_EXTENSION_FUNCTION_POINTER 사용) - `ConnectEx`, `DisconnectEx`, `AcceptEx` 함수 포인터 로드.
    *   **소켓 관리**: `CreateSocket()` (TCP 소켓 생성), `Close()` (소켓 닫기).
    *   **소켓 옵션**: `SetLinger()`, `SetReuseAddress()`, `SetRecvBufferSize()`, `SetSendBufferSize()`, `SetTcpNoDelay()`, `SetUpdateAcceptSocket()`. 내부적으로 제네릭 `SetSockOpt()` 템플릿 함수 활용.
    *   **소켓 작업**: `Bind()` (주소 바인딩), `BindAnyAddress()` (INADDR_ANY 바인딩), `Listen()`.

3.  **기존 핵심 파일 수정:**
    *   `ServerCore/CoreGlobal.cpp`: 전역 초기화/정리 시 `SocketUtils::Init()` 및 `SocketUtils::Clear()` 호출 로직 추가.
    *   `ServerCore/CorePch.h`, `ServerCore/Types.h`: 새로운 클래스(`NetAddress`, `SocketUtils`) 및 관련 헤더 파일(`WinSock2.h`, `MSWSock.h`, `WS2tcpip.h`) 포함, 필요한 using 선언 추가.
    *   `GameServer/GameServer.cpp`: 향후 `NetAddress`, `SocketUtils` 사용을 위한 기본적인 초기화 호출은 `CoreGlobal`에서 처리되도록 수정. (직접적인 사용 로직은 아직 미반영)

4.  **프로젝트 파일 수정:**
    *   `ServerCore/ServerCore.vcxproj`, `ServerCore/ServerCore.vcxproj.filters`: 새로 추가된 `NetAddress.cpp`, `NetAddress.h`, `SocketUtils.cpp`, `SocketUtils.h` 파일들을 프로젝트에 포함.
