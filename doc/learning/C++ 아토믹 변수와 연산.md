# C++ 아토믹 변수와 연산 (std::atomic)

## 1. 아토믹 연산이란?

아토믹 연산(Atomic Operation, 원자적 연산)은 실행 도중에 인터럽트되거나 중단되지 않는, 즉 분해할 수 없는 단일 연산을 의미합니다. 멀티스레딩 환경에서 여러 스레드가 공유 데이터에 동시에 접근할 때 발생할 수 있는 데이터 경쟁(race condition) 문제를 해결하기 위해 사용됩니다.

아토믹 연산을 사용하면, 복잡한 락(lock) 메커니즘(예: 뮤텍스) 없이도 특정 데이터 타입을 스레드로부터 안전하게 읽고 쓸 수 있습니다.

## 2. `std::atomic` 헤더와 템플릿 클래스

C++ 표준 라이브러리는 `<atomic>` 헤더를 통해 `std::atomic` 템플릿 클래스를 제공합니다. 이를 사용하면 다양한 데이터 타입(정수형, 포인터, bool 등)을 원자적으로 다룰 수 있습니다.

```cpp
#include <atomic>

std::atomic<int> atomic_int_var;
std::atomic<bool> atomic_bool_var;
std::atomic<MyClass*> atomic_pointer_var; // 사용자 정의 타입의 포인터도 가능
```

## 3. 주요 아토믹 연산 멤버 함수

`std::atomic` 객체는 여러 유용한 멤버 함수를 제공하여 원자적 연산을 수행합니다.

*   **`load()`**: 원자적으로 값을 읽습니다.
*   **`store(value)`**: 원자적으로 값을 씁니다.
*   **`exchange(value)`**: 원자적으로 새 값을 저장하고 이전 값을 반환합니다.
*   **`compare_exchange_weak(expected, desired)` / `compare_exchange_strong(expected, desired)`**: 현재 값이 `expected`와 같으면 `desired`로 바꾸고 `true`를 반환합니다. 다르면 `expected`를 현재 값으로 업데이트하고 `false`를 반환합니다. `weak` 버전은 실패할 가능성이 있지만(spurious failure), 특정 상황에서 더 효율적일 수 있습니다.
*   **`fetch_add(value)`**: 원자적으로 값을 더하고, 더하기 전의 원래 값을 반환합니다. (정수 및 포인터 타입에 사용 가능)
*   **`fetch_sub(value)`**: 원자적으로 값을 빼고, 빼기 전의 원래 값을 반환합니다. (정수 및 포인터 타입에 사용 가능)
*   **`fetch_and(value)`, `fetch_or(value)`, `fetch_xor(value)`**: 각각 원자적으로 비트 AND, OR, XOR 연산을 수행하고, 연산 전의 원래 값을 반환합니다. (정수 타입에 사용 가능)

그 외에도 `operator++()`, `operator--()` 등 연산자 오버로딩을 통해 편리하게 사용할 수 있는 경우도 있습니다.

## 4. `GameServer.cpp` 예제 분석

제공된 `GameServer.cpp` 코드는 `std::atomic<int32>` 타입의 전역 변수 `sum`을 사용합니다.

```cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<int32> sum = 0; // int32 타입을 위한 아토믹 변수

void Add()
{
    for (int32 i = 0; i < 1000000; i++)
    {
        sum.fetch_add(1); // sum += 1 과 유사하지만 원자적으로 수행
    }
}

void Sub()
{
    for (int32 i = 0; i < 1000000; i++)
    {
        sum.fetch_sub(1); // sum -= 1 과 유사하지만 원자적으로 수행
    }
}

int main()
{
    std::thread t1(Add);
    std::thread t2(Sub);

    t1.join();
    t2.join();

    std::cout << "Final sum: " << sum << std::endl; // sum.load()와 동일하게 동작
    return 0;
}
```

위 코드에서 `sum.fetch_add(1)`과 `sum.fetch_sub(1)`은 `sum` 변수의 값을 여러 스레드에서 동시에 수정하더라도 데이터 경쟁 없이 안전하게 처리되도록 보장합니다. 만약 `sum`이 일반 `int32` 타입이었다면, `Add` 스레드와 `Sub` 스레드가 동시에 `sum`을 읽고, 수정하고, 쓰는 과정에서 경쟁이 발생하여 최종 결과가 예상과 다를 수 있습니다 (예: 0이 아닐 수 있음).

## 5. 메모리 순서 (Memory Order)

`std::atomic` 연산은 추가적인 인자로 `std::memory_order`를 지정하여 메모리 가시성과 명령어 재배치에 대한 제어를 더 세밀하게 할 수 있습니다. 이는 고급 주제이며, 기본값(`std::memory_order_seq_cst`)은 가장 강력한 순차적 일관성을 보장하여 이해하기 쉽지만, 성능상 오버헤드가 있을 수 있습니다. 최적화가 필요한 경우 `memory_order_relaxed`, `memory_order_acquire`, `memory_order_release` 등을 고려할 수 있습니다.

## 6. 결론

`std::atomic`은 C++에서 멀티스레드 환경의 동시성 제어를 위한 강력하고 효율적인 도구입니다. 공유 데이터에 대한 간단한 연산의 경우, 뮤텍스와 같은 락 기반 동기화보다 더 가볍고 성능적으로 유리할 수 있습니다.
