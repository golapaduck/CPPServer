# C++ 스핀락(SpinLock) 구현과 CAS(Compare-And-Swap)

## 1. 스핀락(SpinLock)이란?

스핀락은 멀티스레딩 환경에서 공유 자원에 대한 접근을 동기화하기 위한 락(Lock) 메커니즘 중 하나입니다. 스핀락은 락을 획득할 수 있을 때까지 반복적으로 확인하며 대기(busy-waiting)하는 방식으로 동작합니다. 이는 컨텍스트 스위칭 없이 CPU 시간을 소모하며 대기하기 때문에, 락이 매우 짧은 시간 동안만 유지될 것으로 예상되는 경우에 유용할 수 있습니다.

## 2. `std::atomic`을 사용한 스핀락 구현

C++에서는 `std::atomic` 라이브러리를 사용하여 스핀락을 구현할 수 있습니다. `std::atomic<bool>` 변수를 사용하여 락의 상태를 나타내고, CAS(Compare-And-Swap) 연산을 통해 락을 획득하고 해제합니다.

### `GameServer.cpp`의 `SpinLock` 예제

```cpp
#include <atomic>

class SpinLock
{
public:
    void lock()
    {
        bool expected = false; // 현재 락이 걸려있지 않은 상태 (false)를 예상
        bool desired = true;  // 락을 획득한 상태 (true)로 변경하고 싶음

        // _locked의 현재 값이 expected와 같다면 desired로 바꾸고 true 반환
        // 다르다면 _locked의 현재 값을 expected에 쓰고 false 반환
        while (_locked.compare_exchange_strong(expected, desired) == false)
        {
            // compare_exchange_strong이 실패하면, _locked의 현재 값이 expected에 기록됨.
            // 다음 시도에서는 다시 락이 풀린 상태(false)를 기대해야 하므로 expected를 false로 설정.
            expected = false;
            // CPU 과부하를 줄이기 위해 잠시 멈추거나 다른 스레드에 양보하는 코드를 추가할 수 있음 (예: std::this_thread::yield())
        }
    }

    void unlock()
    {
        // 락을 해제 (false로 설정)
        // 다른 스레드가 CAS 연산을 통해 true로 변경할 수 있도록 함
        _locked.store(false);
    }

private:
    std::atomic<bool> _locked = false; // 락의 상태. false: unlocked, true: locked
};
```

## 3. CAS (Compare-And-Swap) 연산

`compare_exchange_strong(expected, desired)` 함수는 원자적으로 다음 작업을 수행합니다:

1.  현재 `std::atomic` 변수(`_locked`)의 값을 `expected`의 값과 비교합니다.
2.  **값이 같다면**: `_locked`의 값을 `desired`로 변경하고 `true`를 반환합니다.
3.  **값이 다르다면**: `expected`의 값을 `_locked`의 현재 값으로 변경하고 `false`를 반환합니다.

`SpinLock::lock()` 메소드에서는 `_locked`가 `false`(잠기지 않음, `expected`)일 때 `true`(잠김, `desired`)로 바꾸려고 시도합니다. 이 작업이 성공할 때까지 (즉, `compare_exchange_strong`이 `true`를 반환할 때까지) 루프를 반복합니다.

루프 내에서 `expected = false;`를 다시 설정하는 이유는, 만약 `compare_exchange_strong`이 실패했다면 (다른 스레드가 이미 락을 잡았거나, `_locked`가 `true`인 상태), `expected`는 `_locked`의 현재 값인 `true`로 업데이트됩니다. 따라서 다음 루프 반복에서 `false` 상태를 다시 기대하도록 `expected`를 명시적으로 `false`로 설정해야 합니다.

## 4. 바쁜 대기(Busy-Waiting)의 장단점

-   **장점**: 락을 기다리는 시간이 매우 짧을 것으로 예상될 때 컨텍스트 스위칭 비용을 피할 수 있어 성능상 이점을 가질 수 있습니다.
-   **단점**:
    -   락을 기다리는 동안 CPU 자원을 계속 소모합니다 (CPU 사이클 낭비).
    -   락을 오래 기다려야 하는 경우 시스템 전체 성능에 악영향을 줄 수 있습니다.
    -   단일 코어 CPU에서는 사실상 의미가 없으며, 오히려 다른 작업을 방해할 수 있습니다.

따라서 스핀락은 매우 신중하게, 짧은 임계 영역에만 사용해야 합니다. 실제로는 `std::mutex`와 같은 운영체제 수준의 동기화 객체를 사용하는 것이 더 일반적이고 안전한 경우가 많습니다.
