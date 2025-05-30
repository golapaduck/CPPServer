# 학습 문서: 스레드 로컬 저장소(TLS)를 활용한 SendBuffer 최적화

## 1. 개요

고성능 네트워크 서버에서 데이터 송신은 매우 빈번하게 발생하는 작업이며, 송신 버퍼를 효율적으로 관리하는 것은 전체 시스템 성능에 큰 영향을 미칩니다. 기존 `SendBufferManager`는 중앙 집중형으로 송신 버퍼 청크를 관리하여 여러 스레드가 동시에 접근할 때 락 경합이 발생할 수 있었습니다. 이러한 문제를 해결하고 성능을 향상시키기 위해 스레드 로컬 저장소(Thread Local Storage, TLS)를 활용하여 `SendBuffer` 할당 로직을 최적화했습니다.

이 문서에서는 TLS의 개념, `SendBufferManager`의 변경된 작동 방식, 그리고 이로 인한 기대 효과에 대해 설명합니다.

## 2. 스레드 로컬 저장소 (TLS)란?

스레드 로컬 저장소(Thread Local Storage, TLS)는 각 스레드가 자신만의 독립적인 저장 공간을 가질 수 있도록 하는 프로그래밍 기법입니다. `thread_local`로 선언된 변수는 외견상 전역 변수나 정적 변수처럼 보일 수 있지만, 실제로는 각 스레드마다 별도의 인스턴스가 생성되어 해당 스레드 내에서만 접근 가능합니다. 이 변수들은 스레드가 시작될 때 생성 및 초기화되고, 스레드가 종료될 때 소멸합니다.

TLS를 사용하면 다음과 같은 이점이 있습니다:
*   **스레드 안전성**: 각 스레드가 자신만의 데이터 복사본을 가지므로, 여러 스레드가 동일한 변수에 동시에 접근할 때 발생할 수 있는 데이터 경합(race condition) 문제를 피할 수 있습니다.
*   **락 불필요**: 스레드별로 데이터가 분리되어 있어, 해당 데이터에 접근하기 위해 락(lock)과 같은 동기화 메커니즘을 사용할 필요가 줄어들어 성능 향상에 기여할 수 있습니다.
*   **상태 유지**: 스레드별로 특정 상태를 유지해야 할 때 유용합니다. 예를 들어, 각 스레드가 고유한 ID, 트랜잭션 컨텍스트, 또는 `LSendBufferChunk`와 같이 재사용 가능한 리소스에 대한 참조를 가질 수 있습니다. 이는 각 스레드가 자신만의 싱글톤(Singleton) 객체 인스턴스를 가지는 것과 유사한 효과를 낼 수 있습니다.

C++11부터 도입된 `thread_local` 저장소 지정자를 사용하여 TLS 변수를 선언할 수 있습니다.

```cpp
// CoreTLS.h
// 다른 소스 파일에서 이 변수를 참조할 수 있도록 extern으로 선언
extern thread_local SendBufferChunkRef LSendBufferChunk;

// CoreTLS.cpp
// 실제 TLS 변수 정의. 각 스레드는 이 변수의 자신만의 인스턴스를 가짐.
thread_local SendBufferChunkRef LSendBufferChunk; 
```

위 코드에서 `LSendBufferChunk`는 각 스레드가 현재 사용 중인 `SendBufferChunk`에 대한 `shared_ptr`를 저장하는 TLS 변수입니다. 이를 통해 각 스레드는 다른 스레드와 독립적으로 자신만의 송신 버퍼 청크를 관리할 수 있게 됩니다.

### 2.1. `thread_local` 사용 규칙 및 주의사항

`thread_local` 지정자를 사용할 때는 다음과 같은 규칙과 주의사항을 고려해야 합니다.

1.  **선언 위치 및 `static`과의 관계**:
    *   **전역 또는 네임스페이스 범위**: `thread_local`로 선언된 변수는 암시적으로 `static` 저장 기간을 갖지 않습니다. 즉, 프로그램 시작 시가 아닌 스레드 시작 시 초기화됩니다.
        ```cpp
        thread_local int g_tls_var = 0; // 각 스레드마다 0으로 초기화
        ```
    *   **클래스/구조체 멤버**: `thread_local`은 반드시 `static` 멤버에만 적용될 수 있습니다. 비정적 멤버 변수에는 사용할 수 없습니다.
        ```cpp
        struct MyClass {
            static thread_local int s_tls_member; // OK
            // thread_local int m_tls_member; // 오류: 비정적 멤버에는 사용 불가
        };
        thread_local int MyClass::s_tls_member = 0; // 정의
        ```
    *   **함수 내 지역 변수**: 함수 내에 `thread_local`로 선언된 지역 변수는 암시적으로 `static` 저장 기간을 가집니다. 즉, `thread_local int x;`는 `static thread_local int x;`와 동일하게 동작하며, 스레드별로 단 한번 초기화되고 스레드 종료 시까지 유지됩니다.
        ```cpp
        void foo() {
            thread_local int count = 0; // 각 스레드에서 foo가 처음 호출될 때 초기화
            count++;
        }
        ```

2.  **선언과 정의 모두에 `thread_local` 명시**:
    *   스레드 로컬 객체의 선언(주로 헤더 파일)과 정의(주로 소스 파일) 모두에 `thread_local` 지정자를 사용해야 합니다.
        ```cpp
        // my_tls.h
        extern thread_local int my_global_tls_var;

        // my_tls.cpp
        thread_local int my_global_tls_var = 10;
        ```

3.  **초기화 제약 사항**:
    *   `thread_local` 변수의 초기화는 스레드가 시작될 때 해당 스레드의 컨텍스트에서 발생합니다.
    *   일반적으로 정적 초기화(컴파일 타임에 값이 결정되는 경우)는 문제가 없지만, 동적 초기화(런타임에 값이 결정되는 복잡한 초기화) 시에는 주의가 필요합니다. 특히 특정 플랫폼(예: 관리 코드/CLR, WinRT 환경)에서는 동적 초기화가 제한되거나 오류(예: C2482)를 발생시킬 수 있습니다.
    *   자기 참조 초기화(예: `thread_local int i = i;`)는 일반적으로 오류입니다.

4.  **소멸자 호출**:
    *   `thread_local`로 선언된 객체의 소멸자는 해당 객체를 소유한 스레드가 정상적으로 종료될 때 호출됩니다.
    *   만약 스레드가 `std::terminate` 호출 등으로 비정상적으로 종료되는 경우, `thread_local` 객체의 소멸자가 호출되지 않을 수 있습니다. 이는 일반적인 C++ 객체의 소멸 규칙과 유사합니다. 따라서 리소스 해제가 중요한 객체를 `thread_local`로 사용할 경우, 스레드의 정상 종료를 보장하거나 다른 리소스 관리 방안을 고려해야 할 수 있습니다.

5.  **성능 고려 사항**:
    *   `thread_local` 변수에 접근하는 것은 일반적인 전역 변수나 멤버 변수에 접근하는 것보다 약간의 오버헤드가 있을 수 있습니다. 이 오버헤드는 플랫폼 및 컴파일러 구현에 따라 다릅니다. 하지만 대부분의 경우, 락을 사용하는 것보다 훨씬 효율적입니다.

## 3. SendBufferManager 최적화 방식

최적화된 `SendBufferManager`는 각 스레드가 가능한 한 자신에게 할당된 로컬 `SendBufferChunk`를 사용하도록 유도하여, 글로벌 자원에 대한 접근과 락 경합을 최소화합니다.

### 3.1. 주요 변경점: `SendBufferManager::Open()`

`SendBufferManager::Open(uint32 size)` 메서드는 다음과 같이 작동합니다.

```cpp
// ServerCore/SendBuffer.cpp
SendBufferRef SendBufferManager::Open(uint32 size)
{
    // 1. 현재 스레드에 할당된 SendBufferChunk가 없는 경우
    if (LSendBufferChunk == nullptr)
    {
        LSendBufferChunk = Pop(); // 글로벌 풀에서 가져오거나 새로 생성
        LSendBufferChunk->Reset();
    }

    // LSendBufferChunk는 현재 사용 중(Open)이 아니어야 함
    ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

    // 2. 현재 스레드의 SendBufferChunk에 요청된 크기만큼의 여유 공간이 없는 경우
    if (LSendBufferChunk->FreeSize() < size)
    {
        LSendBufferChunk = Pop(); // 글로벌 풀에서 새로운 Chunk를 가져와 교체
        LSendBufferChunk->Reset();
    }

    // 3. 현재 스레드의 SendBufferChunk에서 SendBuffer 할당
    return LSendBufferChunk->Open(size);
}
```

**작동 방식 설명:**

1.  **로컬 청크 확인 및 초기화**: `Open()`이 호출되면, 먼저 현재 스레드의 `LSendBufferChunk`가 `nullptr`인지 확인합니다. 만약 `nullptr`이라면 (즉, 해당 스레드에 아직 할당된 청크가 없다면), `Pop()` 메서드를 호출하여 글로벌 `SendBufferChunk` 풀에서 청크를 가져오거나 새로 생성하여 `LSendBufferChunk`에 할당하고 `Reset()`합니다.
2.  **여유 공간 확인 및 교체**: `LSendBufferChunk`가 유효하다면, 해당 청크에 요청된 `size`만큼의 데이터를 기록할 여유 공간(`FreeSize()`)이 있는지 확인합니다. 만약 공간이 부족하다면, 다시 `Pop()`을 호출하여 새로운 청크를 가져와 기존 `LSendBufferChunk`를 교체하고 `Reset()`합니다. 이 과정을 통해 항상 충분한 공간을 가진 청크를 사용하도록 보장합니다.
3.  **버퍼 할당**: 위 조건들을 통과하면, 현재 스레드의 `LSendBufferChunk`에서 `Open(size)`를 호출하여 실제 `SendBuffer`를 할당받아 반환합니다.

### 3.2. `SendBufferManager::Pop()` 과 글로벌 풀

`Pop()` 메서드는 다음과 같이 구현되어 있습니다.

```cpp
// ServerCore/SendBuffer.cpp
SendBufferChunkRef SendBufferManager::Pop()
{
    {
        WRITE_LOCK; // 글로벌 풀 접근을 위한 락
        if (_sendBufferChunks.empty() == false)
        {
            SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
            _sendBufferChunks.pop_back();
            return sendBufferChunk;
        }
    }
    // 글로벌 풀에 사용 가능한 청크가 없으면 새로 생성
    // 새로 생성된 청크는 PushGlobal을 통해 소멸 시 글로벌 풀로 반환됨
    return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);
}
```

`Pop()`은 먼저 `WRITE_LOCK`을 사용하여 `_sendBufferChunks`라는 이름의 글로벌 `SendBufferChunk` 풀에 접근합니다. 풀에 사용 가능한 청크가 있으면 가져와 반환하고, 없다면 새로운 `SendBufferChunk`를 생성하여 반환합니다. 이때, 새로 생성된 청크는 `std::shared_ptr`의 커스텀 삭제자(`PushGlobal`)를 통해 소멸될 때 자동으로 글로벌 풀에 반환되도록 설정됩니다.

### 3.3. `SendBufferChunk::Close()` 와 `_usedSize`

`SendBuffer` 사용이 완료되어 `SendBuffer::Close(uint32 writeSize)`가 호출되면, 내부적으로 `_owner->Close(writeSize)` (즉, `SendBufferChunk::Close(writeSize)`)가 호출됩니다.

```cpp
// ServerCore/SendBuffer.cpp
void SendBufferChunk::Close(uint32 writeSize)
{
    ASSERT_CRASH(_open == true);
    _open = false; // 청크를 다시 사용 가능 상태로 (하지만 아직 데이터는 남아있음)
    _usedSize += writeSize; // 실제 사용된 크기 누적
}
```
`SendBufferChunk::Close()`는 청크의 `_open` 상태를 `false`로 변경하여 다른 `SendBuffer`가 이 청크의 동일한 영역을 재할당받지 않도록 합니다. 그리고 `_usedSize`에 실제 사용된 `writeSize`를 누적합니다. `LSendBufferChunk->Reset()`이 호출될 때 `_usedSize`는 0으로 초기화되어 청크 전체를 다시 사용할 수 있게 됩니다.

## 4. 기대 효과

*   **성능 향상**: 각 스레드가 대부분의 경우 자신만의 로컬 `SendBufferChunk`를 사용하므로, 글로벌 `SendBufferManager`에 대한 접근 빈도가 줄어듭니다.
*   **락 경합 감소**: `SendBufferManager::Open()`의 주된 로직이 TLS 변수를 통해 락 없이 수행되므로, 여러 스레드가 동시에 송신 버퍼를 요청할 때 발생할 수 있는 락 경합이 크게 줄어듭니다. `Pop()` 메서드 내부에서만 짧은 시간 동안 락이 사용됩니다.
*   **메모리 관리 효율성**: `SendBufferChunk`를 풀링하여 재사용하므로, 빈번한 메모리 할당 및 해제로 인한 오버헤드를 줄입니다.

## 5. 결론

스레드 로컬 저장소(`thread_local`)를 활용한 `SendBufferManager` 최적화는 동시성이 높은 서버 환경에서 송신 버퍼 관리의 효율성을 크게 높여줍니다. 각 스레드가 자신만의 `SendBufferChunk`를 우선적으로 사용함으로써 글로벌 리소스에 대한 경쟁을 줄이고, 이는 락 경합 감소 및 전반적인 시스템 응답성과 처리량 개선에 기여할 수 있습니다.

`thread_local`의 특성과 사용 규칙을 정확히 이해하고 적용하면, 스레드 안전성을 확보하면서도 성능을 최적화하는 강력한 도구가 될 수 있습니다.
