# 스레드 매니저와 Thread Local Storage (TLS)

멀티스레드 프로그래밍에서는 여러 스레드를 효율적으로 관리하고, 각 스레드별로 고유한 데이터를 유지하는 것이 중요합니다. 이 문서에서는 스레드 매니저 클래스와 Thread Local Storage(TLS)의 구현 및 사용 방법에 대해 설명합니다.

## 1. 스레드 매니저 (ThreadManager)

스레드 매니저는 여러 스레드를 생성하고 관리하는 클래스로, 스레드 생성, 스레드 조인, 스레드별 고유 ID 할당 등의 기능을 제공합니다.

### 1.1 스레드 매니저 클래스 정의

```cpp
// ThreadManager.h
class ThreadManager
{
public:
    ThreadManager();
    ~ThreadManager();

    void    Launch(function<void(void)> callback);
    void    Join();

    static void    InitTLS();
    static void    DestroyTLS();

private:
    Mutex           _lock;
    vector<thread>  _threads;
};
```

### 1.2 주요 기능 설명

#### 1.2.1 스레드 생성 (Launch)

`Launch` 함수는 콜백 함수를 받아 새로운 스레드를 생성하고, 해당 스레드에서 콜백 함수를 실행합니다. 이때 스레드별 고유 ID를 할당하기 위해 `InitTLS` 함수를 호출합니다.

```cpp
// ThreadManager.cpp
void ThreadManager::Launch(function<void(void)> callback)
{
    LockGuard guard(_lock);

    _threads.push_back(thread([=]()
        {
            InitTLS();
            callback();
            DestroyTLS();
        }));
}
```

#### 1.2.2 스레드 조인 (Join)

`Join` 함수는 생성된 모든 스레드가 작업을 완료할 때까지 대기하고, 완료된 스레드를 정리합니다.

```cpp
// ThreadManager.cpp
void ThreadManager::Join()
{
    for (thread& t : _threads)
    {
        if(t.joinable())
            t.join();
    }
    _threads.clear();
}
```

#### 1.2.3 스레드 로컬 스토리지 초기화 (InitTLS)

`InitTLS` 함수는 스레드별 고유 ID를 할당하는 기능을 수행합니다. 이때 `Atomic<uint32>`를 사용하여 스레드 안전하게 ID를 증가시킵니다.

```cpp
// ThreadManager.cpp
void ThreadManager::InitTLS()
{
    static Atomic<uint32> SThreadId = 1;
    LThreadId = SThreadId.fetch_add(1);
}
```

## 2. Thread Local Storage (TLS)

Thread Local Storage(TLS)는 각 스레드마다 독립적인 저장 공간을 제공하는 메커니즘입니다. C++11에서는 `thread_local` 키워드를 사용하여 TLS 변수를 선언할 수 있습니다.

### 2.1 TLS 변수 선언

```cpp
// CoreTLS.h
extern thread_local uint32 LThreadId;
```

```cpp
// CoreTLS.cpp
thread_local uint32 LThreadId = 0;
```

`thread_local` 키워드로 선언된 변수는 각 스레드마다 독립적인 인스턴스를 가집니다. 따라서 한 스레드에서 이 변수의 값을 변경해도 다른 스레드의 변수 값에는 영향을 주지 않습니다.

### 2.2 TLS 사용 예시

```cpp
// GameServer.cpp
void ThreadMain()
{
    while (true)
    {
        cout << "Hello ! I am thread..." << LThreadId << endl;
        this_thread::sleep_for(1s);
    }
}

int main()
{
    for (int32 i = 0;i < 5;i++)
    {
        GThreadManager->Launch(ThreadMain);
    }

    GThreadManager->Join();
}
```

위 예제에서는 5개의 스레드를 생성하고, 각 스레드에서 `ThreadMain` 함수를 실행합니다. 각 스레드는 자신의 고유 ID(`LThreadId`)를 출력합니다.

## 3. 코어 글로벌 객체 (CoreGlobal)

코어 글로벌 객체는 프로그램 전체에서 사용되는 전역 객체들을 초기화하고 정리하는 역할을 담당합니다.

### 3.1 코어 글로벌 클래스 정의

```cpp
// CoreGlobal.h
extern class ThreadManager* GThreadManager;

class CoreGlobal
{
public:
    CoreGlobal();
    ~CoreGlobal();
};
```

### 3.2 코어 글로벌 객체 구현

```cpp
// CoreGlobal.cpp
#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"

ThreadManager* GThreadManager = nullptr;

CoreGlobal::CoreGlobal()
{
    GThreadManager = new ThreadManager();
}

CoreGlobal::~CoreGlobal()
{
    delete GThreadManager;
}
```

코어 글로벌 객체의 생성자에서는 스레드 매니저 객체를 생성하고, 소멸자에서는 이를 정리합니다. 이렇게 함으로써 프로그램 시작 시 필요한 전역 객체들이 자동으로 초기화되고, 프로그램 종료 시 정리됩니다.

### 3.3 코어 글로벌 객체 사용

```cpp
// GameServer.cpp
#include "CorePch.h"
#include "CoreMacro.h"
#include "ThreadManager.h"

CoreGlobal Core;

// ... 코드 생략 ...

int main()
{
    for (int32 i = 0;i < 5;i++)
    {
        GThreadManager->Launch(ThreadMain);
    }

    GThreadManager->Join();
}
```

`CoreGlobal Core;` 문장을 통해 코어 글로벌 객체를 생성하면, 생성자에서 스레드 매니저 등의 전역 객체들이 초기화됩니다. 이후 `GThreadManager`를 통해 스레드 매니저의 기능을 사용할 수 있습니다.

## 4. 크래시 매크로 (CoreMacro)

크래시 매크로는 디버깅 및 오류 처리를 위한 매크로 함수들을 제공합니다.

### 4.1 크래시 매크로 정의

```cpp
// CoreMacro.h
#pragma once

/*==================
        Crash
====================*/

#define CRASH(cause)                        \
{                                            \
    uint32* crash = nullptr;                \
    __analysis_assume(crash != nullptr);    \
    *crash = 0xDEADBEEF;                    \
}

#define ASSERT_CRASH(expr)            \
{                                    \
    if (!(expr))                    \
    {                                \
    CRASH("ASSERT_CASH");            \
        __analysis_assume(expr);    \
    }                                \
}
```

### 4.2 크래시 매크로 사용

```cpp
void SomeFunction(int* ptr)
{
    // ptr이 nullptr이 아닌지 확인하고, 만약 nullptr이면 크래시를 발생시킵니다.
    ASSERT_CRASH(ptr != nullptr);
    
    // 이후 코드는 ptr이 유효한 포인터임을 가정하고 실행됩니다.
    *ptr = 10;
}
```

`ASSERT_CRASH` 매크로는 주어진 조건이 거짓일 경우 프로그램을 크래시시킵니다. 이는 디버깅 중에 오류를 빠르게 발견하고 수정하는 데 도움이 됩니다.

## 결론

스레드 매니저와 Thread Local Storage(TLS)는 멀티스레드 프로그래밍에서 중요한 역할을 합니다. 스레드 매니저는 여러 스레드를 효율적으로 생성하고 관리하며, TLS는 각 스레드별로 독립적인 데이터를 유지할 수 있게 해줍니다. 또한 코어 글로벌 객체와 크래시 매크로는 프로그램의 초기화, 정리, 디버깅을 돕는 유용한 도구입니다.

이러한 기능들을 적절히 활용하면 안정적이고 효율적인 멀티스레드 프로그램을 개발할 수 있습니다.
