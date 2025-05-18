# C++ 메모리 관리 시스템 및 할당자

## 개요

C++에서 메모리 관리는 애플리케이션의 성능과 안정성에 큰 영향을 미칩니다. 기본적인 `new`/`delete` 연산자 외에도, 특정 상황에 최적화된 다양한 메모리 할당 전략과 커스텀 할당자를 구현하여 사용할 수 있습니다. 이 문서에서는 메모리 풀과 더불어 사용될 수 있는 몇 가지 고급 메모리 관리 기법과 할당자 종류에 대해 설명합니다.

## 메모리 관리 시스템의 필요성

- **성능 최적화**: 일반적인 동적 할당은 시스템 호출을 포함하여 오버헤드가 클 수 있습니다. 커스텀 메모리 관리 시스템은 이를 최소화합니다.
- **메모리 단편화 방지**: 반복적인 작은 크기의 할당과 해제는 메모리 단편화를 유발합니다. 메모리 풀과 같은 기법은 이를 효과적으로 관리합니다.
- **디버깅 용이성**: 메모리 관련 오류(누수, 이중 해제 등)를 추적하고 디버깅하는 데 도움을 줄 수 있는 기능을 추가할 수 있습니다.
- **특정 요구사항 충족**: 실시간 시스템이나 제한된 메모리 환경에서는 특정 요구사항에 맞는 메모리 관리 방식이 필요합니다.

## 다양한 할당자(Allocator) 종류

### 1. 기본 할당자 (BaseAllocator)

- 가장 기본적인 할당자로, 일반적으로 시스템에서 제공하는 `malloc`과 `free` (또는 C++의 `::operator new`, `::operator delete`)를 직접 호출하여 메모리를 할당하고 해제합니다.
- 특별한 최적화나 관리 기능은 없지만, 간단하게 사용할 수 있습니다.

```cpp
// BaseAllocator.h 예시
class BaseAllocator
{
public:
    static void* Alloc(int32 size);
    static void  Release(void* ptr);
};

// BaseAllocator.cpp 예시
#include "pch.h"
#include "Allocator.h"

void* BaseAllocator::Alloc(int32 size)
{
    return ::malloc(size);
}

void BaseAllocator::Release(void* ptr)
{
    ::free(ptr);
}
```

### 2. 스톰프 할당자 (StompAllocator)

- 메모리 오버런(memory overrun)과 언더런(underrun)을 감지하기 위한 디버깅 목적의 할당자입니다.
- 할당된 메모리 블록의 앞뒤에 접근 불가능한 가드 페이지(guard page)를 두어, 해당 영역에 접근 시 예외를 발생시켜 오류를 즉시 감지할 수 있도록 합니다.
- Windows API의 `VirtualAlloc`과 `VirtualFree`를 사용하여 페이지 단위로 메모리를 관리합니다.

```cpp
// StompAllocator.h 예시
class StompAllocator
{
    enum { PAGE_SIZE = 0x1000 }; // 일반적으로 4KB
public:
    static void* Alloc(int32 size);
    static void  Release(void* ptr);
};

// StompAllocator.cpp 예시
#include "pch.h"
#include "Allocator.h"

void* StompAllocator::Alloc(int32 size)
{
    // 요청된 크기를 포함할 수 있는 페이지 수 계산
    const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    // 실제 데이터가 저장될 위치의 오프셋 계산 (페이지 끝에 맞춤)
    const int64 dataOffset = pageCount * PAGE_SIZE - size;

    // 한 페이지를 더 할당하여 가드 페이지로 사용 (baseAddress가 가리키는 페이지)
    // 실제 데이터는 그 다음 페이지의 dataOffset 위치부터 저장됨
    void* baseAddress = ::VirtualAlloc(NULL, (pageCount + 1) * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    if (baseAddress == nullptr)
        return nullptr;

    // 데이터가 저장될 페이지의 접근 권한을 READWRITE로 변경
    char* dataPageAddress = static_cast<char*>(baseAddress) + PAGE_SIZE;
    ::VirtualProtect(dataPageAddress, pageCount * PAGE_SIZE, PAGE_READWRITE, nullptr);

    return static_cast<void*>(dataPageAddress + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
    const int64 address = reinterpret_cast<int64>(ptr);
    // 할당 시 VirtualAlloc으로 받은 시작 주소 계산
    // ptr은 실제 데이터 시작 주소. 페이지 경계로 맞춘 후, 가드페이지 크기(PAGE_SIZE)만큼 빼줌
    const int64 baseAddress = (address - (address % PAGE_SIZE)) - PAGE_SIZE;
    ::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}
```

### 3. 풀 할당자 (PoolAllocator)

- 앞서 설명한 메모리 풀(Memory Pool)을 사용하는 할당자입니다.
- 정해진 크기의 메모리 블록을 미리 할당해두고 재사용하여 할당/해제 속도를 높이고 단편화를 줄입니다.
- 게임 객체나 네트워크 패킷과 같이 크기가 일정하고 빈번하게 생성/소멸되는 객체에 적합합니다.
- 전역 메모리 관리자(`GMemory`)를 통해 실제 메모리 풀에 접근하여 할당/해제합니다.

```cpp
// PoolAllocator.h 예시
class PoolAllocator
{
public:
    static void* Alloc(int32 size);
    static void  Release(void* ptr);
};

// PoolAllocator.cpp 예시
#include "pch.h"
#include "Allocator.h"
#include "Memory.h" // GMemory 인스턴스 접근

// GMemory는 전역적으로 관리되는 메모리 관리자 객체라고 가정
extern Memory* GMemory;

void* PoolAllocator::Alloc(int32 size)
{
    return GMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{
    GMemory->Release(ptr);
}
```

### 4. STL 할당자 (StlAllocator)

- C++ 표준 라이브러리(STL) 컨테이너(예: `std::vector`, `std::list`, `std::map` 등)와 함께 사용될 수 있는 커스텀 할당자입니다.
- STL 컨테이너가 내부적으로 메모리를 할당하고 해제할 때, 기본 `std::allocator` 대신 사용자가 정의한 할당 로직(예: 풀 할당자)을 사용하도록 합니다.
- 이를 통해 STL 컨테이너의 메모리 관리 방식도 애플리케이션의 전반적인 메모리 전략에 통합할 수 있습니다.

```cpp
// StlAllocator.h 예시
template<typename T, typename Allocator = PoolAllocator> // 기본적으로 PoolAllocator 사용
class StlAllocator
{
public:
    using value_type = T;

    StlAllocator() { }

    template<typename Other>
    StlAllocator(const StlAllocator<Other>&) { }

    T* allocate(size_t count)
    {
        const int32 size = static_cast<int32>(count * sizeof(T));
        return static_cast<T*>(Allocator::Alloc(size)); // 커스텀 할당자 사용
    }

    void deallocate(T* ptr, size_t count)
    {
        Allocator::Release(ptr); // 커스텀 해제자 사용
    }
};

// 사용 예시
// std::vector<MyObject, StlAllocator<MyObject, PoolAllocator>> myVector;
```

## 전역 메모리 관리자 (Global Memory Manager)

애플리케이션 전반의 메모리 할당 요청을 중앙에서 관리하는 시스템입니다. `Memory` 클래스와 같이 구현될 수 있으며, 다양한 크기의 요청에 대해 적절한 메모리 풀을 선택하여 할당해주는 역할을 합니다.

- **메모리 풀 테이블**: 다양한 크기의 메모리 풀들을 관리하고, 요청된 크기에 가장 적합한 풀을 빠르게 찾을 수 있도록 테이블 또는 다른 자료구조를 사용합니다.
- **`xnew` / `xdelete`**: 커스텀 할당/해제 매크로 또는 함수를 제공하여, 사용자가 일반적인 `new`/`delete`처럼 쉽게 커스텀 메모리 관리 시스템을 이용할 수 있도록 합니다.

```cpp
// Memory.h 예시 (일부)
class Memory
{
    enum
    {
        POOL_COUNT = /* 풀 개수 정의 */,
        MAX_ALLOC_SIZE = /* 최대 풀 할당 크기 */
    };

public:
    Memory();
    ~Memory();

    void* Allocate(int32 size); // 요청 크기에 맞는 풀에서 할당
    void Release(void* ptr);    // 적절한 풀에 반환

private:
    std::vector<MemoryPool*> _pools;       // 생성된 모든 메모리 풀 관리
    MemoryPool* _poolTable[MAX_ALLOC_SIZE + 1]; // 크기별 풀 매핑 테이블
};

// 커스텀 new/delete 연산자 오버로딩 또는 매크로
template<typename Type, typename... Args>
Type* xnew(Args&&... args)
{
    // GMemory를 통해 메모리 할당
    Type* memory = static_cast<Type*>(GMemory->Allocate(sizeof(Type))); 
    // Placement new를 사용하여 객체 생성
    new(memory) Type(std::forward<Args>(args)...);
    return memory;
}

template<typename Type>
void xdelete(Type* obj)
{
    if (obj == nullptr) return;
    obj->~Type(); // 명시적 소멸자 호출
    GMemory->Release(obj); // GMemory를 통해 메모리 해제
}
```

## 결론

효율적인 메모리 관리 시스템과 다양한 목적의 할당자를 구현하는 것은 고성능 C++ 애플리케이션 개발에 있어 핵심적인 부분입니다. 메모리 풀, 스톰프 할당자, 풀 할당자, STL 호환 할당자 등을 통해 애플리케이션의 성능을 극대화하고, 메모리 관련 오류를 효과적으로 디버깅하며, 특정 환경의 요구사항을 만족시킬 수 있습니다.

### 3. 풀 할당자 (PoolAllocator)

풀 할당자는 미리 정해진 크기(`_allocSize`)의 메모리 블록들을 관리하는 메모리 풀을 사용합니다. `StompAllocator`와 유사하게 `MemoryHeader`를 사용하여 할당된 메모리 블록의 크기를 추적합니다. `PoolAllocator`는 `MemoryPool`의 `Pop()`과 `Push()` 함수를 사용하여 메모리를 할당하고 해제합니다. 모든 할당 요청은 메모리 풀에서 사용 가능한 블록을 가져오며, 해제 시에는 해당 블록을 다시 풀로 반환합니다.

내부적으로 `MemoryPool`은 `SLIST_ENTRY` (또는 유사한 방식의 단일 연결 리스트 노드)를 사용하여 사용 가능한 메모리 블록들의 리스트를 관리합니다. 메모리 블록이 할당될 때는 이 리스트의 헤드에서 블록을 가져오고(Pop), 해제될 때는 해당 블록을 리스트의 헤드에 다시 추가합니다(Push). 이러한 방식은 매우 빠른 할당 및 해제 속도를 제공합니다.

**특징:**

*   **고정 크기 할당**: 각 할당은 메모리 풀에 정의된 블록 크기만큼의 메모리를 예약합니다. 요청된 크기가 블록 크기보다 작더라도 전체 블록이 할당됩니다.
*   **빠른 할당/해제**: 미리 할당된 블록 리스트에서 가져오거나 반환하므로 매우 빠릅니다.
*   **단편화 감소**: 동일 크기의 블록을 사용하므로 외부 단편화가 줄어듭니다. 특히, 할당하려는 객체의 크기가 메모리 풀의 블록 크기와 비슷하거나 동일할 때 메모리 사용 효율성이 극대화됩니다.
*   **메모리 헤더**: `MemoryHeader`를 사용하여 할당 크기를 관리합니다.
*   **멀티스레드 지원**: Windows의 `SLIST`를 사용하면 락-프리(lock-free) 방식으로 스레드 안전성을 확보할 수 있습니다. (예제 코드에서는 `std::atomic`을 사용하여 할당 카운트를 관리합니다.)

**구현 (`PoolAllocator`)**:

```cpp
// PoolAllocator 클래스 선언 (Allocator.h)
class PoolAllocator : public Allocator
{
public:
    PoolAllocator(MemoryPool* pool) : _pool(pool) { }
    virtual ~PoolAllocator() { } // 소멸자에서 풀을 해제하지 않음 (외부에서 관리)

    virtual void* Alloc(int32 size) override;
    virtual void Release(void* ptr) override;

private:
    MemoryPool* _pool; // 사용할 메모리 풀
};

// PoolAllocator 클래스 정의 (Allocator.cpp)
void* PoolAllocator::Alloc(int32 size)
{
    // 요청된 크기에 메모리 헤더 크기를 더함
    // 실제로는 풀의 블록 크기와 요청 크기를 비교하여 적절한 풀을 선택해야 하지만,
    // 여기서는 단일 풀을 사용한다고 가정
    // MemoryHeader* header = reinterpret_cast<MemoryHeader*>(_pool->Pop());
    // return MemoryHeader::AttachHeader(header, size);
    // 위 코드는 MemoryPool의 Pop이 MemoryHeader*를 반환하고, _allocSize를 사용자가 아닌 풀이 결정한다고 가정할 때의 예시
    // 현재 코드에서는 MemoryPool::Pop이 void*를 반환하고 _allocSize는 사용자 지정 가능.
    // 따라서 Memory::Allocate의 로직처럼, 요청 size에 맞는 풀을 찾아서 할당해야 함.
    // 이 PoolAllocator는 특정 풀에 종속적이므로, 해당 풀의 _allocSize와 요청 size를 직접 비교.
    // 다만, PoolAllocator는 일반적으로 '특정 크기 전용 풀'에서만 할당하므로, size 검증은 생략될 수 있음.
    // (외부에서 이미 적절한 풀을 선택하여 PoolAllocator를 사용한다고 가정)
    
    // MemoryPool의 Pop()은 이미 헤더를 포함할 수 있는 충분한 공간을 가진 블록을 반환하거나
    // 새로 할당한다. AttachHeader는 그 공간에 헤더 정보를 기록하고 실제 데이터 영역 포인터를 반환.
    return MemoryHeader::AttachHeader(static_cast<MemoryHeader*>(_pool->Pop()), _pool->GetAllocSize());
}

void PoolAllocator::Release(void* ptr)
{
    // 포인터에서 메모리 헤더를 분리하여 풀에 반환
    _pool->Push(MemoryHeader::DetachHeader(ptr));
}
```

**MemoryPool의 내부 동작 (요약)**:

```cpp
// MemoryPool.h 에서...
DECLSPEC_ALIGN(SLIST_ALIGNMENT)
struct MemoryHeader : public SLIST_ENTRY { /* ... */ };

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
class MemoryPool {
public:
    MemoryPool(int32 allocSize);
    ~MemoryPool();
    void Push(MemoryHeader* ptr); // 해제된 블록을 리스트의 헤드에 추가
    MemoryHeader* Pop();          // 리스트의 헤드에서 블록을 가져옴
    int32 GetAllocSize() { return _allocSize; }
private:
    SLIST_HEADER _header;      // 사용 가능한 블록들의 단일 연결 리스트 헤더
    int32 _allocSize = 0;
    std::atomic<int32> _allocCount = 0;
};

// MemoryPool.cpp 에서...
void MemoryPool::Push(MemoryHeader* ptr) {
    ::InterlockedPushEntrySList(&_header, static_cast<PSLIST_ENTRY>(ptr));
    _allocCount.fetch_sub(1);
}

MemoryHeader* MemoryPool::Pop() {
    MemoryHeader* memory = static_cast<MemoryHeader*>(::InterlockedPopEntrySList(&_header));
    if (memory == nullptr) {
        // 풀에 가용한 블록이 없으면 새로 정렬된 메모리 할당
        memory = reinterpret_cast<MemoryHeader*>(::_aligned_malloc(_allocSize, SLIST_ALIGNMENT));
    }
    _allocCount.fetch_add(1);
    return memory;
}
