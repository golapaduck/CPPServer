# C++11 동시성: std::future, std::promise, std::packaged_task

C++11은 현대적인 동시성 프로그래밍을 지원하기 위해 여러 유용한 도구들을 표준 라이브러리에 도입했습니다. 그중 `std::future`, `std::promise`, `std::packaged_task`는 비동기 작업의 결과를 효과적으로 관리하고 스레드 간 데이터 전달을 용이하게 합니다. 이러한 기능들은 기존의 플랫폼 종속적인 동기화 프리미티브(예: Windows의 `HANDLE` 및 이벤트 객체) 사용의 복잡성을 줄이고, 보다 안전하고 이식성 높은 코드를 작성하는 데 도움을 줍니다.

`GameServer.cpp`에서는 이러한 C++11 동시성 기능들을 활용하여 비동기 작업을 처리하는 예제를 보여줍니다.

## 1. `std::future` 와 `std::async`

`std::future`는 비동기적으로 수행되는 작업의 결과를 나타내는 객체입니다. 이 결과를 실제로 사용할 수 있을 때까지 기다리거나, 준비되었는지 확인할 수 있습니다. `std::async` 함수는 함수를 비동기적으로 실행하고, 그 결과에 대한 `std::future`를 반환합니다.

**주요 특징:**
-   `std::async`는 실행 정책을 지정할 수 있습니다:
    -   `std::launch::async`: 새로운 스레드에서 작업을 즉시 실행합니다.
    -   `std::launch::deferred`: `future`의 `get()`이나 `wait()`가 호출될 때까지 작업 실행을 지연시킵니다. (호출한 스레드에서 실행)
    -   `std::launch::async | std::launch::deferred` (기본값): 구현이 최적의 방식을 선택합니다. **주의:** 이 경우, 작업이 즉시 새 스레드에서 시작되지 않을 수 있으며, `future` 객체의 소멸자가 호출될 때까지 작업 실행이 지연되거나, 심지어 `future`의 소멸자가 블로킹될 수 있습니다 (만약 비동기 작업이 완료되지 않았다면). 따라서 명시적인 실행 정책(`std::launch::async` 또는 `std::launch::deferred`)을 사용하는 것이 예측 가능한 동작을 위해 권장됩니다.
-   `future.get()`: 작업이 완료될 때까지 대기하고 결과를 반환합니다. `get()`은 한 번만 호출할 수 있습니다. 호출 후 해당 `future` 객체는 더 이상 유효하지 않게 됩니다 (`valid()`가 `false`를 반환). 결과를 여러 번 참조해야 한다면 `std::shared_future`를 사용해야 합니다 (원본 `future`에서 `share()` 멤버 함수를 통해 얻을 수 있습니다).
-   비동기 작업에서 예외가 발생하면, `future.get()`을 호출할 때 해당 예외가 다시 던져집니다.
-   `future.valid()`: `future`가 유효한 공유 상태를 참조하는지 확인합니다. `get()`이나 `wait()` 호출 전에 확인하는 것이 좋습니다.

**예시 (`GameServer.cpp`):**
```cpp
#include <future>
#include <iostream>

// 시간이 오래 걸리는 계산 함수
int64 Calculate()
{
    int64 sum = 0;
    for (int32 i = 0; i < 100000; i++) // 예시를 위해 반복 횟수 조정
    {
        sum += i;
    }
    return sum;
}

void example_future_async()
{
    // Calculate 함수를 비동기적으로 실행 (새 스레드에서)
    std::future<int64> future = std::async(std::launch::async, Calculate);

    // 다른 작업 수행 가능 ...

    // 비동기 작업의 결과가 필요할 때 get() 호출
    // Calculate 함수가 완료될 때까지 여기서 대기
    int64 sum = future.get();
    std::cout << "Calculate 결과: " << sum << std::endl;
}
```

## 2. `std::promise`

`std::promise`는 특정 값을 생산하여 연결된 `std::future`를 통해 소비자에게 전달하는 메커니즘을 제공합니다. 한 스레드에서 `promise`에 값을 설정(`set_value()`)하면, 다른 스레드에서 해당 `promise`와 연관된 `future`를 통해 그 값을 받을 수 있습니다.

**주요 특징:**
-   `promise.get_future()`: `promise`와 연결된 `future` 객체를 가져옵니다.
-   `promise.set_value()`: 값을 설정하여 `future`를 준비 상태로 만듭니다. 예외를 전달하려면 `set_exception()`을 사용합니다. `promise` 객체는 일반적으로 한 번만 값을 설정하거나 예외를 설정하는 데 사용됩니다.

**예시 (`GameServer.cpp`):**
```cpp
#include <future>
#include <string>
#include <thread>
#include <iostream>

// promise를 통해 메시지를 전달하는 작업자 함수
void PromiseWorker(std::promise<std::string>&& promise)
{
    // 어떤 작업을 수행한 후...
    promise.set_value("Secret Message From Promise");
}

void example_promise()
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    // PromiseWorker를 새 스레드에서 실행하고, promise의 소유권을 이전
    std::thread t(PromiseWorker, std::move(promise));

    // future를 통해 PromiseWorker가 전달한 메시지를 받음
    // PromiseWorker가 set_value()를 호출할 때까지 대기
    std::string message = future.get();
    std::cout << "PromiseWorker가 전달한 메시지: " << message << std::endl;

    t.join();
}
```

## 3. `std::packaged_task`

`std::packaged_task`는 함수나 호출 가능한 객체를 래핑(wrapping)하여, 그 실행 결과를 `std::future`를 통해 비동기적으로 받을 수 있게 합니다. `std::function`과 유사하게 다양한 호출 가능한 대상을 담을 수 있으며, 실행 시점을 제어할 수 있습니다.

**주요 특징:**
-   생성 시 호출 가능한 대상을 인자로 받습니다.
-   `task.get_future()`: `packaged_task`와 연결된 `future` 객체를 가져옵니다.
-   `task()` (operator()): 래핑된 함수를 실행합니다. 이 실행 결과가 `future`에 설정됩니다.
-   `task.reset()`: `packaged_task`를 초기 상태로 되돌려, 새로운 `future`를 얻고 다시 실행할 수 있게 합니다. 이는 재사용 가능한 작업을 만들 때 유용합니다.
-   `task.valid()`: `packaged_task`가 유효한 공유 상태를 참조하는지 확인합니다.

**예시 (`GameServer.cpp`):**
```cpp
#include <future>
#include <thread>
#include <iostream>

// packaged_task를 실행하는 작업자 함수
// std::packaged_task<int64(void)>는 int64를 반환하고 인자를 받지 않는 함수 시그니처를 의미
void TaskWorker(std::packaged_task<int64(void)>&& task)
{
    // 전달받은 task를 실행
    task();
}

void example_packaged_task()
{
    // Calculate 함수를 실행할 packaged_task 생성
    std::packaged_task<int64(void)> task(Calculate);
    std::future<int64> future = task.get_future();

    // TaskWorker를 새 스레드에서 실행하고, task의 소유권을 이전
    std::thread t(TaskWorker, std::move(task));

    // future를 통해 task가 실행한 Calculate 함수의 결과를 받음
    // task()가 호출되고 완료될 때까지 대기
    int64 sum = future.get();
    std::cout << "PackagedTask (Calculate) 결과: " << sum << std::endl;

    t.join();
}
```

## 결론

`std::future`, `std::promise`, `std::packaged_task`는 C++에서 비동기 프로그래밍을 위한 강력하고 유연한 도구입니다. 이들을 사용하면 스레드 간의 결과 전달, 작업 예약, 예외 처리 등을 표준화된 방식으로 처리할 수 있어, 복잡한 동시성 로직을 보다 쉽게 구현하고 관리할 수 있습니다.
