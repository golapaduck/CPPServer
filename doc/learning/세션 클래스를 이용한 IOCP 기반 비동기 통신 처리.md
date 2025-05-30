# 세션(Session) 클래스를 이용한 IOCP 기반 비동기 통신 처리

## 1. 개요

`Session` 클래스는 서버에 접속하는 개별 클라이언트와의 통신을 담당하는 핵심 컴포넌트입니다. 각 클라이언트 연결마다 하나의 `Session` 객체가 생성되어 데이터 송수신, 연결 관리 등 모든 네트워크 상호작용을 처리합니다. 이 문서는 `Session` 클래스가 IOCP(Input/Output Completion Port) 모델을 기반으로 어떻게 비동기적으로 네트워크 이벤트를 처리하는지 설명합니다.

## 2. Session 클래스의 주요 역할

-   **연결 관리**: 클라이언트와의 연결 설정(Connect), 유지, 종료(Disconnect)를 담당합니다.
-   **데이터 송수신**: 비동기적으로 데이터를 수신(Receive)하고 송신(Send)합니다.
-   **이벤트 처리**: IOCP로부터 완료된 I/O 작업을 통지받아 해당 작업을 처리합니다.
-   **버퍼 관리**: 수신 버퍼(`_recvBuffer`)와 송신 버퍼 큐(`_sendQueue`)를 관리합니다.

## 3. IOCP와 비동기 I/O

IOCP는 고성능 네트워크 프로그래밍을 위한 Windows API로, 적은 수의 스레드로 다수의 클라이언트 연결을 효율적으로 관리할 수 있게 해줍니다. `Session` 클래스는 다음과 같은 주요 Winsock 함수들과 함께 IOCP를 활용하여 비동기 I/O 작업을 등록하고 완료 알림을 받습니다.

-   `ConnectEx`: 비동기 연결 시도
-   `DisconnectEx`: 비동기 연결 종료
-   `WSARecv`: 비동기 데이터 수신
-   `WSASend`: 비동기 데이터 송신

각 비동기 작업은 `OVERLAPPED` 구조체를 확장한 `IocpEvent` 객체와 함께 IOCP에 등록됩니다. 작업이 완료되면 IOCP는 해당 `IocpEvent`를 완료 큐에 넣고, 작업자 스레드는 이를 가져와 처리합니다.

이러한 비동기 함수들은 호출 시 즉시 반환하며, 작업이 성공적으로 시작되었는지는 반환값과 WSAGetLastError() 함수의 결과를 통해 확인합니다. 예를 들어, 함수가 실패를 나타내는 값(ConnectEx/DisconnectEx의 경우 FALSE, WSARecv/WSASend의 경우 SOCKET_ERROR)을 반환했을 때, WSAGetLastError()가 ERROR_IO_PENDING (또는 WSA_IO_PENDING)을 반환하면 비동기 작업이 성공적으로 등록된 것입니다. 그 외의 오류 코드는 작업 시작에 실패했음을 의미합니다.

## 4. Session의 주요 비동기 작업 처리 흐름

### 4.1. 연결 (Connect)

1.  **`Session::Connect()`**: 클라이언트로서 서버에 연결을 시도할 때 호출됩니다.
2.  **`Session::RegisterConnect()`**:
    *   소켓이 재사용 가능하도록 설정 (`SocketUtils::SetReuseAddress`).
    *   임의의 로컬 주소와 포트에 소켓을 바인딩 (`SocketUtils::BindAnyAddress`).
    *   `_connectEvent` (타입: `ConnectEvent`)를 초기화하고 `Session` 자신을 `owner`로 설정합니다.
    *   `SocketUtils::ConnectEx` 함수를 호출하여 비동기 연결을 요청합니다. `_connectEvent`가 `OVERLAPPED` 구조체로 전달됩니다.
    *   ConnectEx 함수 호출이 FALSE를 반환하고 WSAGetLastError() 결과가 ERROR_IO_PENDING이면 비동기 연결 작업이 성공적으로 등록된 것입니다. (ConnectEx가 TRUE를 반환하며 즉시 성공하는 경우도 있지만, 일반적인 비동기 시나리오에서는 IO PENDING 상태를 기대합니다.) 다른 오류 코드가 반환되면 연결 시작에 실패한 것입니다.
3.  **`Session::ProcessConnect()`**: 연결 작업이 완료되면 IOCP를 통해 호출됩니다.
    *   소켓의 `SO_UPDATE_CONNECT_CONTEXT` 옵션을 설정합니다.
    *   연결된 서비스(`_service`)에 `Session`을 등록합니다.
    *   `OnConnected()` 콜백 함수를 호출하여 연결 성공 후 로직을 수행합니다 (예: `GameSession`의 경우 `GSessionManager`에 추가).
    *   데이터 수신을 위해 `RegisterRecv()`를 호출합니다.

### 4.2. 연결 종료 (Disconnect)

1.  **`Session::Disconnect(cause)`**: 연결을 종료할 때 호출됩니다.
2.  **`Session::RegisterDisonnect()`**:
    *   `_disconnectEvent` (타입: `DisconnectEvent`)를 초기화하고 `owner`를 설정합니다.
    *   SocketUtils::DisconnectEx 함수를 호출하여 비동기 연결 종료를 요청합니다 (TF_REUSE_SOCKET 플래그 사용). 이 호출이 FALSE를 반환하고 WSAGetLastError()가 ERROR_IO_PENDING을 나타내면 작업이 성공적으로 등록된 것입니다.
3.  **`Session::ProcessDisconnect()`**: 연결 종료 작업이 완료되면 호출됩니다.
    *   연결된 서비스에서 `Session`을 제거합니다.
    *   `OnDisconnected()` 콜백 함수를 호출합니다 (예: `GameSession`의 경우 `GSessionManager`에서 제거).
    *   `_connectEvent`, `_disconnectEvent`, `_recvEvent`, `_sendEvent`의 `owner`를 `nullptr`로 설정하여 참조 카운트를 정리합니다.

### 4.3. 데이터 수신 (Receive)

1.  **`Session::RegisterRecv()`**: 데이터 수신을 준비합니다. `ProcessConnect` 성공 후 또는 이전 수신 작업 완료 후 호출됩니다.
    *   `_recvEvent` (타입: `RecvEvent`)를 초기화하고 `owner`를 설정합니다.
    *   `_recvBuffer`에서 데이터를 쓸 위치와 가능한 크기를 가져와 `WSABUF` 구조체를 설정합니다.
    *   WSARecv 함수를 호출하여 비동기 데이터 수신을 요청합니다. 이 호출이 SOCKET_ERROR를 반환하고 WSAGetLastError()가 WSA_IO_PENDING을 나타내면 작업이 성공적으로 등록된 것입니다. (드물게 즉시 완료되어 0을 반환할 수도 있습니다.)
2.  **`Session::ProcessRecv(numOfBytes)`**: 데이터 수신 작업이 완료되면 호출됩니다. `numOfBytes`는 수신된 데이터의 크기입니다.
    *   수신된 데이터가 없으면(`numOfBytes == 0`), 연결이 종료된 것으로 간주하고 `Disconnect()`를 호출합니다.
    *   `_recvBuffer`에 수신된 만큼 쓰기 커서를 이동시킵니다 (`OnRead(numOfBytes)`).
    *   `OnRecvPacket()` (또는 `PacketSession`의 경우 `OnRecv()`) 콜백 함수를 호출하여 수신된 데이터를 처리합니다. 이 함수는 처리한 바이트 수를 반환해야 합니다.
    *   처리 후 남은 데이터가 있다면 버퍼를 정리하고(`Clean()`), 다시 `RegisterRecv()`를 호출하여 다음 데이터 수신을 준비합니다.

### 4.4. 데이터 송신 (Send)

1.  **`Session::Send(sendBuffer)`**: 데이터를 송신하고자 할 때 호출됩니다. `SendBufferRef` 타입의 버퍼를 인자로 받습니다.
    *   송신할 데이터를 `_sendQueue` (쓰기 락으로 보호)에 추가합니다.
    *   `_sendRegistered` 플래그를 `true`로 설정하고, 이전 값이 `false`였다면 (즉, 현재 등록된 송신 작업이 없다면) `RegisterSend()`를 호출합니다.
2.  **`Session::RegisterSend()`**: 실제 비동기 송신 작업을 IOCP에 등록합니다.
    *   `_sendEvent` (타입: `SendEvent`)를 초기화하고 `owner`를 설정합니다.
    *   `_sendQueue` (쓰기 락으로 보호)에서 모든 `SendBuffer`를 꺼내 `_sendEvent.sendBuffers`에 추가합니다.
    *   `_sendEvent.sendBuffers`에 있는 각 `SendBuffer`에 대해 `WSABUF` 구조체를 생성합니다.
    *   WSASend 함수를 호출하여 비동기 데이터 송신을 요청합니다. 이 호출이 SOCKET_ERROR를 반환하고 WSAGetLastError()가 WSA_IO_PENDING을 나타내면 작업이 성공적으로 등록된 것입니다. (드물게 즉시 완료되어 0을 반환할 수도 있습니다.)
3.  **`Session::ProcessSend(numOfBytes)`**: 데이터 송신 작업이 완료되면 호출됩니다.
    *   `_sendEvent.sendBuffers`를 비웁니다 (참조 카운트 감소).
    *   `OnSend(numOfBytes)` 콜백 함수를 호출합니다.
    *   `_sendQueue`에 아직 송신할 데이터가 남아있다면 다시 `RegisterSend()`를 호출합니다. 그렇지 않으면 `_sendRegistered` 플래그를 `false`로 설정합니다.

## 5. 이벤트 디스패치

**`Session::Dispatch(iocpEvent, numOfBytes)`**: IOCP 작업자 스레드가 완료된 I/O 이벤트를 가져오면 호출하는 함수입니다.
`iocpEvent->eventType`에 따라 `ProcessConnect`, `ProcessDisconnect`, `ProcessRecv`, `ProcessSend` 중 적절한 함수를 호출하여 작업을 처리합니다.

## 6. PacketSession

`PacketSession`은 `Session` 클래스를 상속받아 패킷 기반 통신을 처리합니다.
**`PacketSession::OnRecv(buffer, len)`**: `Session::ProcessRecv` 내부에서 호출되며, 수신된 `buffer`에서 패킷 헤더를 읽고 완전한 패킷이 도착했는지 확인합니다. 완전한 패킷이 있다면 `OnRecvPacket(packetBuffer, packetLen)`을 호출하여 실제 패킷 처리 로직을 수행합니다.

## 7. 주요 멤버 변수

-   `_socket`: 세션의 소켓 핸들.
-   `_service`: 세션이 속한 `Service` 객체의 참조.
-   `_recvBuffer`: 수신 버퍼.
-   `_sendQueue`: 송신 대기 중인 `SendBuffer`들을 담는 큐.
-   `_connected`: 연결 상태 플래그 (atomic).
-   `_sendRegistered`: 송신 작업 등록 여부 플래그 (atomic).
-   `_connectEvent`, `_disconnectEvent`, `_recvEvent`, `_sendEvent`: 각 I/O 작업에 사용되는 `IocpEvent` 객체들.

## 8. 결론

`Session` 클래스는 IOCP를 활용하여 비동기적으로 네트워크 통신을 효율적으로 관리합니다. 각 I/O 작업(연결, 종료, 송수신)은 별도의 이벤트 객체와 함께 IOCP에 등록되며, 완료 시 해당 이벤트를 처리하는 방식으로 동작합니다. 이를 통해 서버는 높은 동시성과 성능을 달성할 수 있습니다.
