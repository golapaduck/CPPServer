# 커밋 상세: SendBuffer TLS 최적화

**날짜:** 2025-05-30

**커밋 메시지:**
```
feat: SendBuffer 최적화를 위한 스레드 로컬 저장소(TLS) 도입

- `CoreTLS` 및 `SendBuffer` 컴포넌트에 `thread_local` 변수 (`LThreadId`, `LLockStack`, `LSendBufferChunk`) 적용
- `SendBuffer` 및 `SendBufferChunk` 클래스에서 TLS를 활용하여 쓰레드별 버퍼 관리 효율성 증대
- [learning] 스레드 로컬 SendBuffer 최적화.md
- `README.md` 학습 문서 목록 업데이트
```

**주요 변경 내용:**

- **TLS 도입**: `thread_local` 키워드를 사용하여 `CoreTLS.h/cpp` 및 `SendBuffer.h/cpp`에 쓰레드별 지역 변수를 도입했습니다.
    - `LThreadId`: 현재 쓰레드의 ID를 저장합니다.
    - `LLockStack`: 쓰레드별 락 스택을 관리하여 데드락 감지에 활용될 수 있습니다.
    - `LSendBufferChunk`: 각 쓰레드가 자신만의 `SendBufferChunk`를 소유하여 `SendBuffer` 할당 시 락 경쟁을 최소화하고 성능을 향상시킵니다.
- **SendBuffer 최적화**: `SendBuffer` 생성 시 `TLS::GetSendBufferChunk()`를 통해 현재 쓰레드에 할당된 `SendBufferChunk`를 사용하도록 수정했습니다. 이를 통해 버퍼 할당 로직에서 전역 락의 필요성을 줄였습니다.
- **학습 문서 추가**: `doc/learning/스레드 로컬 SendBuffer 최적화.md` 파일을 생성하고 TLS의 개념, 사용법, 장점 및 프로젝트 내 적용 사례를 상세히 기술했습니다.
- **README 업데이트**: `README.md` 파일의 학습 문서 목록에 새로 추가된 문서를 링크했습니다.

**변경된 파일 목록:**

- `DummyClient/DummyClient.cpp`
- `GameServer/GameServer.vcxproj`
- `GameServer/GameSession.cpp`
- `ServerCore/Container.h`
- `ServerCore/CoreGlobal.cpp`
- `ServerCore/CoreGlobal.h`
- `ServerCore/CoreTLS.cpp`
- `ServerCore/CoreTLS.h`
- `ServerCore/SendBuffer.cpp`
- `ServerCore/SendBuffer.h`
- `ServerCore/Types.h`
- `doc/learning/스레드 로컬 SendBuffer 최적화.md`
- `README.md`
