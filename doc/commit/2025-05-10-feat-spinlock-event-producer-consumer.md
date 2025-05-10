# 2025-05-10: feat: SpinLock 개선 및 생산자-소비자 예제 추가

## 변경 사항 요약

-   **`GameServer/GameServer.cpp`**:
    -   기존 SpinLock에 Windows Event Object (`CreateEvent`, `WaitForSingleObject`, `SetEvent`)를 통합하여, 스핀 대기 중 CPU 사용량을 줄이고 컨텍스트 스위칭을 유도하는 방식으로 개선을 시도했습니다.
    -   `std::mutex`, `std::condition_variable`, `std::queue`를 사용한 생산자-소비자(Producer-Consumer) 패턴의 기본 예제 코드를 추가했습니다.
-   **`README.md`**:
    -   새로운 학습 문서인 "Windows Event와 SpinLock 개선 및 생산자-소비자 패턴" 링크를 추가했습니다.
    -   기존 "C++ SpinLock과 CAS" 학습 문서 링크를 사용자가 직접 복원했습니다.
-   **학습 문서 추가**:
    -   `doc/learning/Windows Event와 SpinLock 개선 및 생산자-소비자 패턴.md`: Windows Event Object를 활용한 SpinLock 개선 아이디어와 `std::condition_variable`을 이용한 생산자-소비자 패턴을 설명하는 새 문서를 작성했습니다.

## 세부 변경 내역

### `GameServer.cpp`
-   `SpinLock` 클래스:
    -   Windows `HANDLE` 멤버 변수 추가.
    -   생성자에서 `CreateEvent` 호출 (자동 리셋, 초기 signaled).
    -   `lock()` 메소드: `WaitForSingleObject` 및 `ResetEvent` 로직 추가 (상세 로직은 코드 참조).
    -   `unlock()` 메소드: `SetEvent` 로직 추가.
-   `Producer`, `Consumer` 함수 및 관련 전역 변수 (`g_queue`, `g_mutex`, `g_cv`, `g_done`)를 사용하여 생산자-소비자 예제 구현.

### `README.md`
-   학습 문서 섹션에 두 개의 링크가 있는지 확인 (사용자 수정분 반영).

## 학습 문서
- [learning] Windows Event와 SpinLock 개선 및 생산자-소비자 패턴.md
