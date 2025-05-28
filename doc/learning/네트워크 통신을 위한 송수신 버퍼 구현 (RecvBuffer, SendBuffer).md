# 네트워크 통신을 위한 송수신 버퍼 구현 (RecvBuffer, SendBuffer)

고성능 네트워크 애플리케이션을 개발할 때, 데이터 송수신 처리는 매우 중요한 부분입니다. `RecvBuffer`와 `SendBuffer` 클래스는 TCP/IP 통신과 같은 스트림 기반 프로토콜에서 효율적으로 데이터를 주고받기 위해 설계된 버퍼 관리 클래스입니다.

## 1. RecvBuffer (수신 버퍼)

`RecvBuffer` 클래스는 네트워크로부터 데이터를 수신하고, 애플리케이션이 이 데이터를 처리할 수 있도록 관리하는 역할을 합니다. 주로 원형 버퍼(Circular Buffer)와 유사한 개념을 사용하여 메모리를 효율적으로 재사용합니다.

### 1.1. 주요 구성 요소

-   `_buffer (Vector<BYTE>)`: 실제 데이터가 저장되는 바이트 벡터입니다.
-   `_capacity (int32)`: 버퍼의 전체 용량입니다. 생성자에서 `bufferSize * BUFFER_COUNT`로 설정됩니다.
-   `_bufferSize (int32)`: 단일 읽기/쓰기 작업의 기본 단위 크기입니다.
-   `_readPos (int32)`: 읽기 시작 위치를 나타내는 인덱스입니다. 애플리케이션이 데이터를 읽어가면 이 위치가 이동합니다.
-   `_writePos (int32)`: 쓰기 시작 위치를 나타내는 인덱스입니다. 네트워크로부터 새로운 데이터를 수신하면 이 위치에 데이터가 기록되고 이동합니다.

### 1.2. 주요 메서드

-   **`RecvBuffer(int32 bufferSize)` (생성자)**
    -   지정된 `bufferSize`를 기반으로 버퍼의 전체 용량(`_capacity`)을 계산하고, `_buffer` 벡터의 크기를 할당합니다.
-   **`~RecvBuffer()` (소멸자)**
    -   특별한 정리 작업은 없습니다.
-   **`Clean()`**
    -   버퍼를 정리하여 추가 공간을 확보하는 메서드입니다.
    -   만약 처리할 데이터가 없다면 (`DataSize() == 0`), `_readPos`와 `_writePos`를 모두 0으로 초기화합니다.
    -   처리할 데이터가 있고 남은 공간(`FreeSize()`)이 `_bufferSize`보다 작다면, 아직 처리되지 않은 데이터를 버퍼의 시작 부분으로 복사(압축)합니다. 이를 통해 `_writePos` 뒤쪽에 연속된 빈 공간을 만듭니다.
-   **`OnRead(int32 numOfBytes)`**
    -   애플리케이션이 `numOfBytes`만큼의 데이터를 성공적으로 처리(읽음)했을 때 호출됩니다.
    -   `_readPos`를 `numOfBytes`만큼 증가시킵니다.
    -   읽으려는 바이트 수가 현재 버퍼에 있는 데이터 크기(`DataSize()`)보다 크면 `false`를 반환합니다.
-   **`OnWrite(int32 numOfBytes)`**
    -   네트워크로부터 `numOfBytes`만큼의 데이터를 성공적으로 수신하여 버퍼에 기록했을 때 호출됩니다.
    -   `_writePos`를 `numOfBytes`만큼 증가시킵니다.
    -   **주의**: 제공된 코드에서 `if (numOfBytes > DataSize()) return false;` 부분은 논리적으로 `if (numOfBytes > FreeSize()) return false;` (쓰려는 바이트가 남은 공간보다 크면 실패)가 더 적합해 보입니다. 현재 코드는 `DataSize()`를 사용하고, 성공 여부와 관계없이 `false`를 반환하고 있어 수정이 필요할 수 있습니다.
-   **`ReadPos()`**: `_buffer[_readPos]`의 포인터, 즉 읽을 데이터의 시작 위치를 반환합니다.
-   **`WritePos()`**: `_buffer[_writePos]`의 포인터, 즉 새로 데이터를 쓸 위치를 반환합니다.
-   **`DataSize()`**: `_writePos - _readPos`, 즉 현재 버퍼에 처리해야 할 데이터의 크기를 반환합니다.
-   **`FreeSize()`**: `_capacity - _writePos`, 즉 버퍼에 남은 사용 가능한 공간의 크기를 반환합니다.

### 1.3. 작동 원리

1.  네트워크로부터 데이터가 수신되면, `WritePos()`가 가리키는 위치부터 데이터가 기록되고, `OnWrite()`를 호출하여 `_writePos`를 갱신합니다.
2.  애플리케이션은 `ReadPos()`가 가리키는 위치부터 `DataSize()`만큼의 데이터를 읽어 처리합니다.
3.  데이터 처리가 완료되면 `OnRead()`를 호출하여 `_readPos`를 갱신합니다.
4.  `_writePos`가 버퍼의 끝에 가까워져 공간이 부족해지면, `Clean()` 메서드를 호출하여 버퍼 내의 데이터를 앞으로 당겨 공간을 확보합니다.

## 2. SendBuffer (송신 버퍼)

`SendBuffer` 클래스는 애플리케이션이 전송하고자 하는 데이터를 임시로 저장하고, 네트워크를 통해 이 데이터를 전송할 수 있도록 관리하는 역할을 합니다. `enable_shared_from_this<SendBuffer>`를 상속받는 것으로 보아, 비동기 송신 작업에서 `shared_ptr`로 관리될 가능성이 높습니다.

### 2.1. 주요 구성 요소

-   `_buffer (Vector<BYTE>)`: 전송할 데이터가 저장되는 바이트 벡터입니다.
-   `_writeSize (int32)`: 버퍼에 실제로 기록된, 즉 전송해야 할 데이터의 크기입니다.

### 2.2. 주요 메서드

-   **`SendBuffer(int32 bufferSize)` (생성자)**
    -   지정된 `bufferSize`만큼 `_buffer` 벡터의 크기를 할당합니다.
-   **`~SendBuffer()` (소멸자)**
    -   특별한 정리 작업은 없습니다.
-   **`Buffer()`**: `_buffer.data()`, 즉 전송할 데이터의 시작 위치 포인터를 반환합니다.
-   **`WriteSize()`**: `_writeSize`, 즉 전송할 데이터의 크기를 반환합니다.
-   **`Capacity()`**: `_buffer.size()`, 즉 버퍼의 전체 용량을 반환합니다.
-   **`CopyData(void* data, int32 len)`**
    -   전송할 `data`를 `len`만큼 `_buffer`에 복사합니다.
    -   복사 후 `_writeSize`를 `len`으로 설정합니다.
    -   `ASSERT_CRASH(Capacity() >= len)`를 통해 복사할 데이터의 길이가 버퍼 용량을 초과하지 않는지 확인합니다.

### 2.3. 작동 원리

1.  애플리케이션은 전송할 데이터가 있을 때 `SendBuffer` 객체를 생성(또는 풀에서 가져옴)합니다.
2.  `CopyData()` 메서드를 사용하여 전송할 데이터를 버퍼에 복사합니다.
3.  네트워크 송신 함수(예: `WSASend` 등)에 `Buffer()`가 반환하는 포인터와 `WriteSize()`를 전달하여 데이터를 전송합니다.
4.  비동기 작업의 경우, `SendBuffer` 객체는 `shared_ptr`로 관리되어 송신 작업이 완료될 때까지 안전하게 유지될 수 있습니다.

## 3. 활용 방안

`RecvBuffer`와 `SendBuffer`는 주로 IOCP(Input/Output Completion Port)와 같은 윈도우 환경의 고성능 비동기 I/O 모델이나, Linux의 epoll과 같은 이벤트 기반 모델에서 사용됩니다.

-   **수신 시나리오 (`RecvBuffer`)**:
    -   소켓에 비동기 수신(`WSARecv`)을 요청할 때 `RecvBuffer`의 `WritePos()`와 `FreeSize()`를 사용합니다.
    -   수신 완료 통지를 받으면, `OnWrite()`를 호출하여 `_writePos`를 갱신합니다.
    -   애플리케이션 로직은 `ReadPos()`부터 `DataSize()`만큼 데이터를 파싱하고 처리한 후, `OnRead()`를 호출합니다.
    -   필요에 따라 `Clean()`을 호출하여 버퍼를 정리합니다.

-   **송신 시나리오 (`SendBuffer`)**:
    -   전송할 데이터를 `SendBuffer`에 `CopyData()`로 채웁니다.
    -   비동기 송신(`WSASend`)을 요청할 때 `SendBuffer`의 `Buffer()`와 `WriteSize()`를 사용합니다.
    -   `SendBuffer` 객체 자체를 비동기 작업의 컨텍스트로 함께 전달하여, 송신 완료 시 버퍼를 해제하거나 재사용 풀에 반환할 수 있습니다.

이러한 버퍼 관리 클래스를 사용하면 동적 메모리 할당/해제를 최소화하고, 데이터 복사를 줄이며, 네트워크 처리 로직을 단순화하는 데 도움이 됩니다.
