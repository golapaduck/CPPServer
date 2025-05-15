#pragma once

#define OUT


/*==================
		Lock
====================*/

#define USE_MANY_LOCKS(count)	Lock _lock[count];
#define USE_LOCK				USE_MANY_LOCKS(1)
#define	READ_LOCK_IDX(idx)		ReadLockGuard readLockGuard##idx(_locks[idx],typeid(this).name();
#define READ_LCOK				READ_LOCK_IDX(0)
#define	WRITE_LOCK_IDX(idx)		WriteLockGuard writeLockGuard##idx(_locks[idx],typeid(this).name());
#define WRITE_LCOK				WRITE_LOCK_IDX(0)

/*==================
		Crash
====================*/

#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
	CRASH("ASSERT_CASH");			\
		__analysis_assume(expr);	\
	}								\
}
