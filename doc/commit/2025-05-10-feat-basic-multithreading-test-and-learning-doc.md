## 날짜

2025-05-10

## 타입

feat

## 제목

기본 멀티스레딩 테스트 코드 및 학습 문서 추가

## 변경된 주요 내용

- GameServer.cpp에 std::thread를 사용한 간단한 멀티스레딩 예제 구현
- 여러 스레드를 생성하고 각 스레드에서 숫자를 출력하는 기능 테스트
- main 함수에서 스레드 생성 및 join 로직 추가
- `doc/learning/멀티 쓰레딩.md` 학습 문서 추가 ([learning] 멀티 쓰레딩.md):
    - C++ 스레드의 기본 개념, 생성 방법, 관리 (join, detach) 설명
    - GameServer.cpp의 멀티스레딩 예제 코드를 문서에 포함
