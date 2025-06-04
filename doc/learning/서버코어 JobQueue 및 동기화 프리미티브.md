# 학습 문서: 서버코어 JobQueue 및 동기화 프리미티브

## 1. 개요

본 문서는 CPPServer 프로젝트의 `ServerCore` 라이브러리에 새롭게 추가된 JobQueue 시스템과 이를 지원하는 동기화 프리미티브(GlobalQueue, LockQueue, JobTimer)에 대해 설명합니다. 이 변경은 서버의 작업 처리 효율성을 높이고, 멀티스레드 환경에서의 안정적인 동작을 보장하는 것을 목표로 합니다.

## 2. 주요 추가/변경된 컴포넌트

### 2.1. Job 및 JobQueue (`Job.h/cpp`, `JobQueue.h/cpp`)

-   **`Job`**: 비동기적으로 실행될 작업의 기본 단위입니다. `std::function<void()>`를 래핑하여 다양한 작업을 캡슐화합니다.
-   **`JobQueue`**: `Job` 객체들을 관리하는 큐입니다. 스레드 풀에서 워커 스레드들이 이 큐에서 작업을 가져와 실행합니다.
    -   주요 기능: `Push(JobRef job)`, `Pop()`, `Execute()`
    -   내부적으로 Lock-Free 큐 또는 Lock 기반 큐를 사용할 수 있도록 설계될 수 있습니다. (현재 구현은 `LockQueue`를 활용할 가능성이 높음)

### 2.2. GlobalQueue (`GlobalQueue.h/cpp`)

-   **`GlobalQueue`**: 여러 스레드에서 접근 가능한 중앙 집중형 작업 큐입니다. `JobQueue`가 특정 스레드나 스레드 그룹에 작업을 분배하기 전에 사용될 수 있습니다.
    -   주요 기능: `Push(JobRef job)`, `Pop()`
    -   여러 `JobQueue` 인스턴스에 작업을 분배하는 역할을 할 수 있습니다.

### 2.3. LockQueue (`LockQueue.h/cpp`)

-   **`LockQueue<T>`**: `std::mutex`와 `std::condition_variable`을 사용하여 스레드 안전성을 보장하는 일반적인 목적의 큐입니다.
    -   `JobQueue`나 `GlobalQueue`의 내부 구현체로 사용될 수 있습니다.
    -   주요 기능: `Push(T item)`, `Pop(OUT T& item)`, `WaitPop(OUT T& item)`
    -   **중요**: `WaitPop`과 같이 `std::condition_variable`을 사용하는 경우, 반드시 predicate (예: 큐가 비어있지 않은지 확인하는 람다 함수)와 함께 `wait`를 호출해야 합니다. 이는 "Lost Wakeup" (알림 유실) 및 "Spurious Wakeup" (가짜 깨어남) 현상을 방지하기 위함입니다. predicate는 `condition_variable`이 상태를 기억하지 못하는 것을 보완하는 역할을 합니다.

### 2.4. JobTimer (`JobTimer.h/cpp`)

-   **`JobTimer`**: 특정 시간 이후에 실행되어야 하는 `Job`들을 관리합니다.
    -   주요 기능: `AddJob(JobRef job, uint64 delayMs)`, `Tick()`
    -   내부적으로 우선순위 큐를 사용하여 예약된 작업들을 관리하고, 주기적인 `Tick`을 통해 만료된 작업을 `JobQueue`로 푸시합니다.

### 2.5. ThreadManager 수정 (`ThreadManager.h/cpp`)

-   기존 `ThreadManager`가 새로운 JobQueue 시스템과 연동되도록 수정되었습니다.
-   워커 스레드들이 `JobQueue`에서 작업을 가져와 처리하는 로직이 포함됩니다.
-   `Launch(function<void(void)> callback)` 외에 `DoAsyncJob(JobRef job)`와 같은 인터페이스가 추가되었을 수 있습니다.

### 2.6. CoreTLS 및 CoreGlobal 수정 (`CoreTLS.h/cpp`, `CoreGlobal.h/cpp`)

-   `CoreTLS` (Thread Local Storage)에 각 스레드별 `JobQueue` 인스턴스(`LJobQueue`)에 대한 포인터나 참조가 추가되었을 가능성이 있습니다. 이를 통해 스레드는 자신에게 할당된 작업을 빠르게 처리할 수 있습니다.
-   `CoreGlobal`에 전역 `JobQueue`(`GJobQueue`)나 `GlobalQueue` 인스턴스가 추가되어 시스템 전반의 작업을 관리할 수 있습니다.

## 3. 기대 효과

-   **작업 처리 효율성 향상**: Job 기반의 비동기 처리를 통해 CPU 활용도를 높이고 응답성을 개선합니다.
-   **멀티스레드 안정성 강화**: 스레드 안전한 큐와 동기화 메커니즘을 통해 데이터 경쟁 및 데드락 위험을 줄입니다.
-   **코드 모듈성 및 확장성 증대**: 작업 단위를 명확히 하고, 새로운 유형의 Job을 쉽게 추가할 수 있습니다.

## 4. AI 자가 검증

-   웹 검색 및 `context7`을 통해 관련 기술 정보(C++ 비동기 프로그래밍, Lock-Free 자료구조, 스레드 풀 패턴 등) 및 모범 사례를 참고하여 문서 내용을 검증하고 보강할 예정입니다.
