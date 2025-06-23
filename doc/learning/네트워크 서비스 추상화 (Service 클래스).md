# 네트워크 서비스 추상화 (Service 클래스)

이 문서에서는 CPPServer 프로젝트에 도입된 네트워크 서비스 추상화 계층, 특히 `Service` 기본 클래스와 이를 상속하는 `ClientService` 및 `ServerService`에 대해 설명합니다.

## 1. Service 클래스 (기본 서비스)

`Service` 클래스는 클라이언트와 서버 네트워킹 로직의 공통 기반을 제공하는 추상 클래스입니다. IOCP 모델을 사용하여 비동기 네트워크 이벤트를 처리하고, 세션 생성 및 관리를 담당합니다.

### 주요 구성 요소

- **`ServiceType _type`**: 서비스의 종류 (서버 또는 클라이언트)를 나타냅니다.
  ```cpp
  enum class ServiceType : uint8
  {
  	Server,
  	Client
  };
  ```
- **`NetAddress _netAddress`**: 서비스가 바인딩하거나 연결할 네트워크 주소 정보입니다.
- **`IocpCoreRef _iocpCore`**: IOCP 객체에 대한 참조로, 비동기 I/O 작업을 처리합니다.
- **`SessionFactory _sessionFactory`**: 세션 객체를 생성하는 셔널입니다. 이를 통해 `Service` 클래스는 구체적인 세션 타입에 의존하지 않고 세션을 생성할 수 있습니다.
  ```cpp
  using SessionFactory = function<SessionRef(void)>;
  ```
- **`Set<SessionRef> _sessions`**: 현재 활성화된 세션들을 관리하는 컨테이너입니다.
- **`int32 _sessionCount`**: 현재 활성화된 세션의 수입니다.
- **`int32 _maxSessionCount`**: 서비스가 최대로 관리할 수 있는 세션의 수입니다.
- **`_iocpHandle`**: IOCP (Input/Output Completion Port) 핸들입니다. 비동기 I/O 작업의 완료 알림을 받는 데 사용됩니다. 운영체제는 완료된 I/O 작업을 이 핸들과 연결된 큐에 넣고, 작업 스레드는 이 큐에서 완료된 작업을 가져와 처리합니다.
- **`_threadCount`**: 서비스에서 사용할 작업 스레드의 수입니다. 일반적으로 CPU 코어 수의 두 배로 설정하여 최적의 성능을 내도록 합니다. 이 스레드들은 `GetQueuedCompletionStatus` 함수를 호출하며 IOCP 완료 큐를 감시합니다.

### 주요 멤버 함수

- **`Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1)`**: 생성자. 서비스 타입, 주소, IOCP 코어, 세션 팩토리, 최대 세션 수를 초기화합니다.
- **`virtual bool Start() abstract`**: 서비스를 시작하는 순수 가상 함수입니다. 하위 클래스에서 구체적인 시작 로직을 구현해야 합니다.
- **`bool CanStart()`**: 세션 팩토리가 설정되어 있는지 확인하여 서비스 시작 가능 여부를 반환합니다.
- **`virtual void CloseService()`**: 서비스를 종료하는 가상 함수입니다. (구현은 TODO로 남아있음)
- **`void SetSessionFactory(SessionFactory func)`**: 세션 팩토리 함수를 설정합니다.
- **`SessionRef CreateSession()`**: `_sessionFactory`를 사용하여 새로운 세션 객체를 생성하고 반환합니다. 이를 통해 `Service`는 특정 세션 구현에 종속되지 않고 유연하게 다양한 타입의 세션을 사용할 수 있습니다.
- **`void AddSession(SessionRef session)`**: 생성된 세션을 내부 세션 목록(`_sessions`)에 추가하고, 해당 세션의 소켓을 IOCP에 등록합니다. 이 과정에서 `PER_SOCKET_CONTEXT`와 유사한 형태로 세션별 데이터를 관리할 수 있습니다.
- **`void ReleaseSession(SessionRef session)`**: 세션 목록에서 특정 세션을 제거합니다.
- **`Start()`**: (순수 가상 함수) 파생 클래스에서 서비스 시작 로직을 구현해야 합니다. 예를 들어, `ServerService`는 리슨 소켓을 생성하고 `accept` 준비를 하며, `ClientService`는 서버에 연결을 시도합니다.
- **`CloseService()`**: 서비스 종료 시 모든 세션을 정리하고 IOCP 핸들을 닫는 등의 리소스 해제 작업을 수행합니다.
- **`Dispatch(uint32 timeoutMs)`**: 작업 스레드에서 호출되며, `GetQueuedCompletionStatus` 함수를 사용하여 IOCP 완료 큐에서 완료된 I/O 이벤트를 가져와 처리합니다. 가져온 이벤트의 종류(연결, 수신, 송신, 종료 등)에 따라 해당 세션의 적절한 콜백 함수(`OnConnected`, `OnRecv`, `OnSend`, `OnDisconnected`)를 호출합니다.

## 2. ClientService 클래스

`ClientService`는 `Service` 클래스를 상속받아 클라이언트 측 네트워킹 기능을 구현합니다.

### 주요 특징

- 생성자 `ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1)`: 서비스 타입을 `ServiceType::Client`로 설정하고, 연결 대상 주소를 받습니다.
- **`virtual bool Start() override`**: `Service::Start()`를 재정의합니다. 설정된 `_maxSessionCount`만큼 세션을 생성하고, 각 세션에 대해 `Connect()`를 호출하여 서버에 연결을 시도합니다.

```cpp
bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}
```

## 3. ServerService 클래스

`ServerService`는 `Service` 클래스를 상속받아 서버 측 네트워킹 기능을 구현합니다.

### 주요 특징

- 생성자 `ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1)`: 서비스 타입을 `ServiceType::Server`로 설정하고, 리스닝할 주소를 받습니다.
- **`ListenerRef _listener`**: 클라이언트의 연결 요청을 수신 대기하는 `Listener` 객체에 대한 참조입니다.
- **`virtual bool Start() override`**: `Service::Start()`를 재정의합니다. `Listener` 객체를 생성하고, `_listener->StartAccept(service)`를 호출하여 클라이언트 연결 수신을 시작합니다. `StartAccept`에는 `ServerService` 자신의 `shared_ptr`이 전달되어, 연결이 수락될 때 세션을 생성하고 관리할 수 있도록 합니다.

```cpp
bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}
```
- **`virtual void CloseService() override`**: `Service::CloseService()`를 재정의합니다. (현재는 기본 `Service::CloseService()`만 호출하며, 추가적인 리스너 종료 로직 등은 TODO로 남아있을 수 있음)

## 4. IOCP (Input/Output Completion Port) 와의 연동

`Service` 클래스는 IOCP를 핵심 메커니즘으로 사용하여 고성능 비동기 네트워킹을 구현합니다.

- **IOCP 생성**: `Service` 생성자 또는 초기화 단계에서 `CreateIoCompletionPort` API를 사용하여 IOCP 핸들(`_iocpHandle`)을 생성합니다.
- **소켓과 IOCP 연결**: 새로운 세션이 생성되거나(`ServerService`에서 클라이언트 연결 수락 시, `ClientService`에서 서버 연결 성공 시) 해당 세션의 소켓 핸들을 `CreateIoCompletionPort` API를 다시 호출하여 기존 IOCP 핸들과 연결합니다. 이때, 각 소켓(세션)을 식별할 수 있는 키(예: `Session` 객체의 포인터 또는 ID)를 함께 전달합니다. 이 키는 `GetQueuedCompletionStatus` 호출 시 반환되어 어떤 소켓에서 I/O가 완료되었는지 식별하는 데 사용됩니다 (이는 `PER_SOCKET_CONTEXT`의 역할과 유사합니다).
- **비동기 I/O 작업 요청**: 데이터 송수신 시 `WSASend`, `WSARecv` 등의 비동기 함수를 `OVERLAPPED` 구조체와 함께 호출합니다. `OVERLAPPED` 구조체에는 I/O 작업별 컨텍스트(`PER_IO_CONTEXT`와 유사)를 저장할 수 있으며, 이 정보 또한 `GetQueuedCompletionStatus`를 통해 전달됩니다.
- **작업 스레드와 `Dispatch`**: `Service`는 여러 작업 스레드를 생성하고, 각 스레드는 `Dispatch` 함수 내에서 `GetQueuedCompletionStatus`를 호출하여 IOCP 완료 큐를 대기합니다. I/O 작업이 완료되면 `GetQueuedCompletionStatus`는 완료된 작업의 정보(전송된 바이트 수, 소켓 식별 키, `OVERLAPPED` 구조체 포인터 등)를 반환합니다.
- **이벤트 처리**: `Dispatch` 함수는 `GetQueuedCompletionStatus`로부터 받은 정보를 바탕으로 어떤 세션에서 어떤 종류의 I/O(수신, 송신, 연결 등)가 완료되었는지 판단하고, 해당 `Session` 객체의 적절한 콜백 함수(`OnRecv`, `OnSend`, `OnConnected` 등)를 호출하여 후속 처리를 위임합니다.

이러한 IOCP 기반 모델은 소수의 스레드로 다수의 클라이언트 연결을 효율적으로 처리할 수 있게 하며, 서버의 확장성과 성능을 크게 향상시킵니다.

## 5. 세션 팩토리 (`SessionFactory`) 활용

`Service` 클래스는 `SessionFactory`를 통해 `Session` 객체를 생성합니다. 이는 팩토리 디자인 패턴의 일종으로, 다음과 같은 이점을 제공합니다:

- **유연성 및 확장성**: `Service` 클래스는 구체적인 `Session` 파생 클래스에 직접 의존하지 않습니다. 대신 `SessionFactory`에게 세션 생성을 위임함으로써, 다양한 종류의 세션 로직(예: 게임 세션, 채팅 세션 등)을 쉽게 추가하거나 교체할 수 있습니다. 새로운 세션 타입을 지원하기 위해 `Service` 코드를 직접 수정할 필요 없이, 새로운 `Session` 파생 클래스와 이를 생성하는 로직을 `SessionFactory`에 추가하면 됩니다.
- **객체 생성 로직 중앙화**: 세션 객체 생성과 관련된 복잡한 로직(예: 초기화 파라미터 설정, 리소스 할당 등)을 `SessionFactory` 내에 캡슐화하여 코드의 가독성과 유지보수성을 높입니다.

`Service`는 생성자에서 `SessionFactory`의 `shared_ptr`를 인자로 받아 멤버 변수 `_sessionFactory`에 저장합니다. 이후 새로운 세션이 필요할 때마다 `_sessionFactory->Create()`와 같이 호출하여 `SessionRef`(즉, `shared_ptr<Session>`)를 얻고, 이를 사용하여 통신을 관리합니다.

## 6. 주요 기능

### 3.1. `ClientService`

`ClientService`는 클라이언트 측 네트워크 서비스를 구현합니다. `Service` 클래스를 상속받아 클라이언트로서 서버에 연결하고 데이터를 송수신하는 기능을 담당합니다.

**주요 기능:**

-   **서버 연결**: 지정된 서버 주소(`NetAddress`)로 TCP 연결을 시도합니다. `connect` 함수 호출 후 비동기적으로 연결 결과를 처리합니다.
-   **세션 관리**: 서버와의 연결이 성공하면 `SessionFactory`를 통해 `Session` 객체를 생성하고, 이 세션을 통해 데이터 송수신을 관리합니다. `Service`의 `AddSession`을 호출하여 IOCP에 등록하고 I/O 작업을 시작합니다.
-   **데이터 송수신**: 연결된 세션을 통해 서버와 데이터를 주고받습니다. `WSASend` 및 `WSARecv` (또는 이와 유사한 비동기 I/O 함수)를 사용합니다.

**`Start()` 함수 주요 로직 (예상):**

1.  연결할 서버의 `NetAddress`를 설정합니다.
2.  `connect` 함수를 호출하여 서버에 비동기 연결을 요청합니다.
3.  연결 성공 시, `SessionFactory`를 통해 `Session`을 생성합니다.
4.  생성된 `Session`을 `AddSession`을 통해 IOCP에 등록하고, 초기 데이터 수신(`WSARecv`)을 요청합니다.
5.  지정된 수의 작업 스레드를 생성하여 `Dispatch` 함수를 실행, IOCP 이벤트를 처리하도록 합니다.

### 3.2. `ServerService`

`ServerService`는 서버 측 네트워크 서비스를 구현합니다. `Service` 클래스를 상속받아 클라이언트의 연결 요청을 수락하고, 각 클라이언트와 독립적인 세션을 통해 통신하는 기능을 담당합니다.

**주요 기능:**

-   **클라이언트 연결 수락**: 지정된 포트에서 클라이언트의 연결 요청을 기다립니다 (`listen`). 연결 요청이 오면 `WSAAccept` (또는 `accept`) 함수를 통해 수락하고, 클라이언트와 통신할 새로운 소켓을 생성합니다.
-   **세션 생성 및 관리**: 새로운 클라이언트 연결이 수락될 때마다 `SessionFactory`를 통해 해당 클라이언트와 통신할 `Session` 객체를 생성합니다. 생성된 세션은 `Service`의 `AddSession`을 통해 IOCP에 등록되고 관리됩니다. 각 세션은 독립적으로 클라이언트와 데이터를 주고받습니다.
-   **데이터 송수신**: 각 클라이언트 세션을 통해 데이터를 주고받습니다. `WSASend` 및 `WSARecv` (또는 이와 유사한 비동기 I/O 함수)를 사용합니다.

**`Start()` 함수 주요 로직 (예상):**

1.  리슨 소켓을 생성하고, 지정된 `NetAddress` (IP, 포트)에 바인딩(`bind`)합니다.
2.  리슨 소켓을 통해 클라이언트 연결 요청을 기다리기 시작합니다 (`listen`).
3.  지정된 수의 작업 스레드를 생성하여 `Dispatch` 함수를 실행, IOCP 이벤트를 처리하도록 합니다. 이 스레드들은 `GetQueuedCompletionStatus`를 통해 완료된 I/O 작업을 가져옵니다.
4.  주 스레드 또는 별도의 accept 스레드에서 반복적으로 `WSAAccept`를 호출하여 클라이언트 연결을 수락합니다.
5.  새로운 연결이 수락되면:
    a.  `SessionFactory`를 통해 해당 클라이언트를 위한 `Session`을 생성합니다.
    b.  생성된 `Session`을 `AddSession`을 통해 IOCP에 등록하고, `PER_SOCKET_CONTEXT`와 유사한 정보를 연결합니다.
    c.  해당 `Session`에 대해 초기 데이터 수신(`WSARecv`)을 요청합니다.

## 7. IOCP와 함께 사용되는 주요 Winsock 함수

IOCP 모델에서 사용되는 주요 Winsock 함수는 다음과 같습니다.

### AcceptEx

`AcceptEx`는 클라이언트의 연결 요청을 비동기적으로 수락하는 함수입니다. 이 함수는 IOCP와 함께 사용될 때 높은 성능을 제공합니다.

```cpp
BOOL WINAPI AcceptEx(
  _In_     SOCKET sListenSocket,
  _In_     SOCKET sAcceptSocket,
  _Inout_  PVOID lpOutputBuffer,
  _In_     DWORD dwReceiveDataLength,
  _In_     DWORD dwLocalAddressLength,
  _In_     DWORD dwRemoteAddressLength,
  _Out_    LPDWORD lpdwBytesReceived,
  _In_     LPOVERLAPPED lpOverlapped
);
```

### WSARecv

`WSARecv`는 데이터를 비동기적으로 수신하는 함수입니다.

```cpp
int WSARecv(
  _In_   SOCKET s,
  _Inout_ LPWSABUF lpBuffers,
  _In_   DWORD dwBufferCount,
  _Out_  LPDWORD lpNumberOfBytesRecvd,
  _Inout_ LPDWORD lpFlags,
  _In_   LPWSAOVERLAPPED lpOverlapped,
  _In_   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
```

### WSASend

`WSASend`는 데이터를 비동기적으로 송신하는 함수입니다.

```cpp
int WSASend(
  _In_   SOCKET s,
  _Inout_ LPWSABUF lpBuffers,
  _In_   DWORD dwBufferCount,
  _Out_  LPDWORD lpNumberOfBytesSent,
  _In_   DWORD dwFlags,
  _In_   LPWSAOVERLAPPED lpOverlapped,
  _In_   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
```

### DisconnectEx

`DisconnectEx`는 연결을 비동기적으로 종료하는 함수입니다.

```cpp
BOOL WINAPI DisconnectEx(
  _In_     SOCKET s,
  _In_     DWORD dwFlags,
  _In_     DWORD_PTR dwReserved
);
```

이러한 함수들은 IOCP 모델에서 비동기적으로 네트워크 이벤트를 처리하는 데 사용됩니다.

## 8. OVERLAPPED 구조체와 CompletionKey의 활용

IOCP 모델에서 `OVERLAPPED` 구조체와 `CompletionKey`는 비동기 작업의 컨텍스트를 관리하고, 완료된 작업을 식별하는 데 핵심적인 역할을 합니다.

### OVERLAPPED 구조체

`OVERLAPPED` 구조체는 비동기 작업의 상태를 관리하는 데 사용됩니다. 이 구조체에는 작업의 종류, 데이터 버퍼, 세션 참조 등이 포함될 수 있습니다.

```cpp
typedef struct _OVERLAPPED {
  ULONG_PTR Internal;
  ULONG_PTR InternalHigh;
  DWORD Offset;
  DWORD OffsetHigh;
  HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;
```

### CompletionKey

`CompletionKey`는 IOCP에 등록된 소켓을 식별하는 데 사용됩니다. 이 키는 `CreateIoCompletionPort` 함수를 통해 등록되며, 완료된 작업을 식별하는 데 사용됩니다.

```cpp
HANDLE WINAPI CreateIoCompletionPort(
  _In_  HANDLE FileHandle,
  _In_  HANDLE ExistingCompletionPort,
  _In_  ULONG_PTR CompletionKey,
  _In_  DWORD NumberOfConcurrentThreads
);
