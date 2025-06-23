# Winsock WSAEventSelect 모델을 이용한 네트워크 이벤트 처리

## 1. 개요

`WSAEventSelect` 모델은 Winsock에서 제공하는 I/O 모델 중 하나로, 특정 소켓에 대한 네트워크 이벤트(예: 데이터 수신, 연결 요청) 발생 시 지정된 **이벤트 객체(Event Object)**를 signaled 상태로 만들어 애플리케이션에 통지하는 방식입니다.

기존의 `select` 모델과 비교했을 때, `WSAEventSelect`는 다음과 같은 장점을 가질 수 있습니다:
-   **명시적인 이벤트 객체 사용:** 각 소켓 또는 소켓 그룹에 대한 이벤트를 개별 이벤트 객체로 관리할 수 있어 코드 구조가 명확해질 수 있습니다.
-   **Windows 이벤트 메커니즘 활용:** Windows의 표준 이벤트 동기화 기능을 사용하므로, `WSAWaitForMultipleEvents`와 같은 함수를 통해 여러 이벤트를 효율적으로 대기할 수 있습니다.
-   **자동 논블로킹 모드 설정:** `WSAEventSelect`를 호출하면 해당 소켓은 자동으로 논블로킹 모드로 전환됩니다.

## 2. 핵심 함수 및 개념

`WSAEventSelect` 모델을 이해하기 위한 주요 함수와 개념은 다음과 같습니다.

### 2.1. `WSAEVENT` (이벤트 객체 핸들)
-   네트워크 이벤트 발생을 알리는 데 사용되는 Windows 이벤트 객체의 핸들입니다.
-   `WSACreateEvent()` 함수를 사용하여 생성하고, 사용 후에는 `WSACloseEvent()` 함수로 닫아야 합니다.

### 2.2. `WSACreateEvent()`
-   새로운 `WSAEVENT` 객체를 생성하고 핸들을 반환합니다.
-   성공 시 이벤트 객체 핸들, 실패 시 `WSA_INVALID_EVENT`를 반환합니다.

```cpp
WSAEVENT hEvent = WSACreateEvent();
if (hEvent == WSA_INVALID_EVENT) {
    // 오류 처리
}
```

### 2.3. `WSAEventSelect(SOCKET s, WSAEVENT hEventObject, long lNetworkEvents)`
-   지정된 소켓 `s`에 대해, `lNetworkEvents`로 명시된 네트워크 이벤트들이 발생했을 때 `hEventObject` 이벤트 객체를 signaled 상태로 만들도록 설정합니다.
-   **`s`**: 대상 소켓 디스크립터.
-   **`hEventObject`**: 소켓 `s`와 연결할 이벤트 객체의 핸들.
-   **`lNetworkEvents`**: 관심 있는 네트워크 이벤트들의 비트마스크 조합. 주요 이벤트는 다음과 같습니다:
    -   `FD_ACCEPT`: 리스닝 소켓에 새로운 연결 요청이 있을 때.
    -   `FD_READ`: 수신할 데이터가 있거나, 연결이 정상적으로 종료(0바이트 수신)되었을 때.
    -   `FD_WRITE`: 데이터를 보낼 수 있을 때 (논블로킹 소켓의 송신 버퍼가 비었을 때). `connect` 또는 `accept` 성공 직후에도 발생.
    -   `FD_CLOSE`: 소켓 연결이 종료되었을 때 (정상 종료 또는 비정상 종료 모두 포함).
    -   `FD_CONNECT`: `connect` 함수의 비동기 호출이 완료되었을 때 (성공 또는 실패).
-   성공 시 0, 실패 시 `SOCKET_ERROR`를 반환합니다.
-   이 함수를 호출하면 소켓 `s`는 **자동으로 논블로킹 모드**로 설정됩니다.
-   하나의 소켓에 `FD_READ`와 `FD_WRITE`를 모두 감지하고 싶다면 `FD_READ | FD_WRITE`와 같이 비트 OR 연산으로 조합합니다.
-   **주의:** 한 소켓에 대해 `WSAEventSelect`를 여러 번 호출하면, 마지막 호출이 이전 설정을 덮어씁니다. 즉, 한 소켓에는 하나의 `WSAEVENT` 객체만 연결할 수 있으며, 이 객체는 여러 유형의 네트워크 이벤트를 감시할 수 있습니다.
-   연결을 취소하고 싶으면 `lNetworkEvents`를 0으로 설정하여 호출합니다.

```cpp
// ListenSocket에 FD_ACCEPT 이벤트가 발생하면 listenEvent를 signaled 상태로 만듦
if (WSAEventSelect(ListenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
    // 오류 처리
}
```

### 2.4. `WSAWaitForMultipleEvents(DWORD cEvents, const WSAEVENT *lphEvents, BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable)`
-   `lphEvents` 배열에 있는 `cEvents` 개의 이벤트 객체들 중 하나 또는 전부가 signaled 상태가 될 때까지 `dwTimeout` 시간 동안 대기합니다.
-   **`cEvents`**: 대기할 이벤트 객체의 수. (`WSA_MAXIMUM_WAIT_EVENTS`로 최대 개수 제한)
-   **`lphEvents`**: `WSAEVENT` 핸들 배열의 포인터.
-   **`fWaitAll`**: `TRUE`이면 모든 이벤트 객체가 signaled 될 때까지 대기, `FALSE`이면 하나라도 signaled 되면 반환. 일반적으로 `FALSE`를 사용합니다.
-   **`dwTimeout`**: 대기 시간 (밀리초). `WSA_INFINITE`로 설정하면 무한 대기.
-   **`fAlertable`**: `TRUE`이면 I/O 완료 루틴 또는 APC(Asynchronous Procedure Call)로 인해 스레드가 깨어날 수 있음을 나타냅니다. 일반적으로 `FALSE`를 사용합니다.
-   **반환 값:**
    -   성공 시: Signaled 된 이벤트 객체의 배열 인덱스. `fWaitAll`이 `FALSE`일 경우, `WSA_WAIT_EVENT_0` (즉, 0)부터 `WSA_WAIT_EVENT_0 + cEvents - 1` 사이의 값을 가집니다.
    -   `WSA_WAIT_TIMEOUT`: `dwTimeout` 시간 초과.
    -   `WSA_WAIT_IO_COMPLETION`: `fAlertable`이 `TRUE`이고 I/O 완료 루틴/APC에 의해 반환.
    -   `WSA_WAIT_FAILED`: 함수 실패. `WSAGetLastError()`로 오류 코드 확인.

### 2.5. `WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents)`
-   `WSAWaitForMultipleEvents`를 통해 특정 이벤트 객체(`hEventObject`)가 signaled 된 것을 확인한 후, 해당 소켓 `s`에서 실제로 어떤 네트워크 이벤트들이 발생했는지 상세 정보를 `lpNetworkEvents` 구조체에 채워줍니다.
-   **`lpNetworkEvents`**: `WSANETWORKEVENTS` 구조체의 포인터.
    -   `lpNetworkEvents->lNetworkEvents`: 실제로 발생한 네트워크 이벤트들의 비트마스크. (예: `FD_READ`, `FD_CLOSE`)
    -   `lpNetworkEvents->iErrorCode[FD_XXX_BIT]`: 각 네트워크 이벤트(`FD_ACCEPT_BIT`, `FD_READ_BIT` 등)에 대한 오류 코드. 0이 아니면 해당 이벤트 처리 중 오류 발생.
-   이 함수를 호출하면, 인자로 전달된 `hEventObject` 이벤트 객체는 **자동으로 non-signaled (reset) 상태**로 변경됩니다. 또한, 소켓과 관련된 내부 네트워크 이벤트 기록도 초기화됩니다.

```cpp
WSANETWORKEVENTS networkEvents;
int index = WSAWaitForMultipleEvents(eventCount, eventArray, FALSE, WSA_INFINITE, FALSE);

// signaled된 이벤트 객체와 연결된 소켓 찾기 (구현에 따라 다름)
SOCKET currentSocket = socketArray[index - WSA_WAIT_EVENT_0]; // 예시
WSAEVENT currentEvent = eventArray[index - WSA_WAIT_EVENT_0]; // 예시

if (WSAEnumNetworkEvents(currentSocket, currentEvent, &networkEvents) == SOCKET_ERROR) {
    // 오류 처리
}

if (networkEvents.lNetworkEvents & FD_ACCEPT) {
    if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
        // FD_ACCEPT 오류 처리
    }
    // accept 처리 로직
}
if (networkEvents.lNetworkEvents & FD_READ) {
    // FD_READ 처리 로직
}
// ... 기타 이벤트 처리
```

### 2.6. `WSACloseEvent(WSAEVENT hEvent)`
-   `WSACreateEvent()`로 생성한 이벤트 객체의 핸들을 닫고 관련 리소스를 해제합니다.

## 3. GameServer.cpp (가상) 적용 흐름

`GameServer.cpp`에서 `WSAEventSelect` 모델을 적용한다면 다음과 같은 흐름을 예상할 수 있습니다. (실제 코드는 `git diff` 결과를 기반으로 추론)

1.  **서버 초기화:**
    *   `WSAStartup()` 호출.
    *   Listen 소켓 생성, `bind()`, `listen()`.
    *   Listen 소켓용 `WSAEVENT` 객체(`listenEvent`)를 `WSACreateEvent()`로 생성.
    *   `WSAEventSelect(ListenSocket, listenEvent, FD_ACCEPT | FD_CLOSE)` 호출하여 `listenEvent`와 연결. (주의: `FD_READ`, `FD_WRITE` 등 다른 이벤트도 Listen 소켓에 설정하면, `accept`된 소켓이 이를 상속받고 **동일한 `listenEvent`를 사용하게 됩니다.** 만약 `accept`된 각 클라이언트 소켓마다 독립적인 이벤트 객체와 이벤트 타입을 원한다면, Listen 소켓에는 `FD_ACCEPT`와 `FD_CLOSE` 정도만 설정하고, `accept` 후 새로 생성된 클라이언트 소켓에 대해 별도의 `WSAEVENT` 객체를 생성하고 `WSAEventSelect`를 호출해야 합니다.)
    *   이벤트 객체들을 담을 배열 (`WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS]`)과 각 이벤트 객체에 해당하는 소켓 정보 (또는 세션 정보)를 담을 배열 (`YourSessionType* SessionArray[WSA_MAXIMUM_WAIT_EVENTS]`) 준비. `EventArray[0]`에 `listenEvent`, `SessionArray[0]`에 Listen 소켓 정보 저장. `EventTotal = 1`.

2.  **메인 루프 (`while(true)`):**
    *   `DWORD index = WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE);` 호출하여 이벤트 발생 대기.
    *   `index` 값을 확인하여 어떤 이벤트 객체가 signaled 되었는지 식별 (`signaledEventIndex = index - WSA_WAIT_EVENT_0`).
    *   `SOCKET currentSocket = SessionArray[signaledEventIndex]->socket;` (또는 유사한 방식으로 소켓 가져오기)
    *   `WSAEVENT hSignaledEvent = EventArray[signaledEventIndex];`
    *   `WSANETWORKEVENTS networkEvents;` 선언.
    *   `WSAEnumNetworkEvents(currentSocket, hSignaledEvent, &networkEvents)` 호출하여 발생한 이벤트 상세 정보 얻기.

    *   **`FD_ACCEPT` 처리 (if `currentSocket == ListenSocket` and `networkEvents.lNetworkEvents & FD_ACCEPT`):**
        *   오류 체크: `networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0`.
        *   `SOCKET clientSocket = accept(ListenSocket, ...);`.
        *   **만약 `accept`된 소켓에 대해 리스닝 소켓과는 다른 이벤트 객체나 네트워크 이벤트 설정을 원한다면 (권장되는 방식):**
            *   새로운 클라이언트 소켓용 `WSAEVENT` 객체(`clientEvent`) 생성.
            *   `WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE)` 호출. (또는 필요한 이벤트만 설정)
            *   `EventArray`와 `SessionArray`에 `clientEvent`와 클라이언트 세션 정보 추가. `EventTotal` 증가. (배열 크기 및 인덱스 관리 주의)
        *   **만약 리스닝 소켓의 `WSAEventSelect` 설정을 그대로 사용한다면, 별도의 `WSAEventSelect` 호출은 필요 없습니다.** 그러나 이 경우 모든 `accept`된 소켓이 리스닝 소켓과 동일한 `WSAEVENT` 객체(`listenEvent`)를 공유하게 되므로, `FD_READ`, `FD_WRITE` 등의 이벤트 처리가 복잡해질 수 있습니다. 일반적으로는 각 클라이언트 소켓마다 별도의 `WSAEVENT`를 할당합니다.
    *   **`FD_READ` 처리 (if `networkEvents.lNetworkEvents & FD_READ`):**
        *   오류 체크: `networkEvents.iErrorCode[FD_READ_BIT] != 0`.
        *   `char recvBuffer[BUFSIZE]; int recvLen = recv(currentSocket, recvBuffer, BUFSIZE, 0);`
        *   `recvLen == 0`: 상대방이 정상 종료. `FD_CLOSE` 이벤트 처리로 유도하거나 여기서 바로 정리.
        *   `recvLen == SOCKET_ERROR`:
            *   `WSAGetLastError() == WSAEWOULDBLOCK`: 무시 (다음에 다시 시도).
            *   기타 오류: 연결 종료 처리.
        *   `recvLen > 0`: 데이터 수신 성공. 데이터 처리.

    *   **`FD_WRITE` 처리 (if `networkEvents.lNetworkEvents & FD_WRITE`):**
        *   오류 체크: `networkEvents.iErrorCode[FD_WRITE_BIT] != 0`.
        *   보낼 데이터가 있다면: `int sendLen = send(currentSocket, sendBuffer, dataSize, 0);`
        *   `sendLen == SOCKET_ERROR`:
            *   `WSAGetLastError() == WSAEWOULDBLOCK`: 무시 (버퍼가 차 있으므로 다음에 다시 시도).
            *   기타 오류: 연결 종료 처리.
        *   송신 성공 시 관련 처리.

    *   **`FD_CLOSE` 처리 (if `networkEvents.lNetworkEvents & FD_CLOSE`):**
        *   오류 체크: `networkEvents.iErrorCode[FD_CLOSE_BIT] != 0`. (일반적으로 비정상 종료 시 오류 코드 포함)
        *   해당 소켓(`currentSocket`)과 연결된 세션 정리.
        *   `closesocket(currentSocket)`.
        *   `WSACloseEvent(hSignaledEvent)`.
        *   `EventArray`와 `SessionArray`에서 해당 이벤트 객체와 세션 정보 제거. `EventTotal` 감소. (배열 요소 재정렬 필요)

## 4. 주의사항 및 주요 특징 요약
-   `WSAEventSelect` 함수 호출 시, 해당 소켓은 자동으로 **논블로킹 모드**로 설정됩니다.
-   소켓을 다시 **블로킹 모드**로 설정하려면, 먼저 `WSAEventSelect(socket, NULL, 0)`을 호출하여 소켓과 이벤트 객체의 연결을 해제한 후, `ioctlsocket()` 또는 `WSAIoctl()` 함수를 사용하여 소켓의 모드를 변경해야 합니다.
-   `WSAEnumNetworkEvents` 함수를 호출하면, 인자로 전달된 이벤트 객체는 자동으로 **non-signaled (reset) 상태**로 변경됩니다. 또한 소켓과 관련된 내부 네트워크 이벤트 기록도 초기화됩니다.
-   `accept()` 함수로 생성된 새로운 클라이언트 소켓은 **리스닝 소켓에 설정된 `WSAEventSelect` 연결 및 네트워크 이벤트 선택 사항을 상속받습니다.** 즉, 리스닝 소켓이 특정 `WSAEVENT` 객체 및 이벤트 플래그(예: `FD_READ`, `FD_WRITE`)로 설정되어 있었다면, `accept`된 소켓도 동일한 `WSAEVENT` 객체와 플래그를 사용하게 됩니다. 만약 `accept`된 소켓에 대해 다른 `WSAEVENT` 객체나 다른 네트워크 이벤트를 사용하고 싶다면, `accept`된 소켓을 대상으로 `WSAEventSelect`를 **새롭게 호출**해야 합니다. (일반적으로 클라이언트 소켓마다 독립적인 `WSAEVENT` 객체를 사용하는 것이 관리에 용이합니다.)
-   네트워크 이벤트가 발생하여 `WSAWaitForMultipleEvents`가 반환되고, `WSAEnumNetworkEvents`로 실제 이벤트를 확인했더라도, 실제 `recv` 또는 `send` 같은 I/O 함수 호출 시 `WSAEWOULDBLOCK` 오류가 발생할 수 있습니다. 이는 이벤트 발생 시점과 실제 I/O 호출 시점 사이에 네트워크 상태가 변할 수 있기 때문입니다. 따라서 항상 `WSAEWOULDBLOCK` 오류를 적절히 처리해야 합니다.
-   하나의 소켓에는 하나의 `WSAEVENT` 객체만 연결할 수 있습니다. 그러나 하나의 `WSAEVENT` 객체는 여러 종류의 네트워크 이벤트(예: `FD_READ | FD_WRITE | FD_CLOSE`)를 동시에 감시하도록 설정할 수 있습니다.
-   `closesocket()` 함수로 소켓을 닫으면 해당 소켓에 대한 `WSAEventSelect` 설정(네트워크 이벤트 연결)은 자동으로 취소됩니다. 그러나 `WSAEventSelect`에 사용된 `WSAEVENT` 객체 자체는 시스템 리소스이므로, 더 이상 사용하지 않는다면 반드시 `WSACloseEvent()` 함수를 호출하여 명시적으로 닫고 리소스를 해제해야 합니다.
