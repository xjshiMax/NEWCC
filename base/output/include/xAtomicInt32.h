//原子操作
//原子操作主要用于实现资源计数，很多引用计数(refcnt)就是通过原子操作实现的
//原子操作保证了基数加和减不会被打乱，不可分割。但涉及到其他如打印之类的还得加锁。
#pragma once
#ifdef WIN32

#include <Windows.h>
#pragma warning(disable: 4146)
#else
#define USE_x386_ASM_ATOMIC
#endif

#if !defined(_WIN32_VER) 
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) && !defined(USE_GENERIC_ATOMICS)
#ifndef USE_x386_ASM_ATOMIC
#define USE_x386_ASM_ATOMIC
#endif
#endif
#endif
class xAtomicInt32
{
public:
	xAtomicInt32():m_val(0){}
	xAtomicInt32(unsigned int val):m_val(val)
	{
	}
	//获取原子变量的值
	unsigned int get()
	{
#ifdef WIN32
		return m_val;
#elif defined USE_x386_ASM_ATOMIC
		return m_val;

#endif
	}

	unsigned int add(unsigned int value)
	{
#ifdef WIN32
		return InterlockedExchangeAdd(&m_val,value);
#elif defined(USE_x386_ASM_ATOMIC)
		asm volatile ("lock; xaddl %0,%1"
			: "=r"(val), "=m"(m_val) /* outputs */
			: "0"(val), "m"(value)   /* inputs */
			: "memory", "cc");
		return val;
#endif

	}
	void		 sub(unsigned int value)
	{
#ifdef WIN32
		InterlockedExchangeAdd(&m_val,-value);
#elif defined(USE_x386_ASM_ATOMIC)
		asm volatile ("lock; subl %1, %0"
			:
		: "m" (m_nVal), "r" (val)
			: "memory", "cc");
#endif
	}
	unsigned int inc()
	{
#ifdef WIN32
		return InterlockedIncrement(&m_val) - 1;
#elif defined(USE_x386_ASM_ATOMIC)
		return add(1)
#endif
	}
	unsigned int dec()
	{
#ifdef WIN32
		return InterlockedDecrement(&m_val);
#elif defined(USE_x386_ASM_ATOMIC)
		unsigned char prev;

		asm volatile ("lock; decl %1;\n\t"
			"setnz %%al"
			: "=a" (prev)
			: "m" (m_val)
			: "memory", "cc");
		return prev;
#endif
	}
	void		 set(unsigned int value)
	{
#ifdef WIN32
		InterlockedExchange(&m_val,value);
#elif defined(USE_x386_ASM_ATOMIC)
		m_val=value;
#endif
	}
	unsigned int swap_when_equal(unsigned int with,unsigned int cmp)
	{
#ifdef WIN32
		return InterlockedCompareExchange(&m_val,with,cmp);
#elif defined(USE_x386_ASM_ATOMIC)
		cmx_uint_32 prev;

		asm volatile ("lock; cmpxchgl %1, %2"             
			: "=a" (prev)               
			: "r" (with), "m" (m_val), "0"(cmp) 
			: "memory", "cc");
		return prev;
#endif
	}
	unsigned int swap(unsigned int with)
	{
#ifdef WIN32
		return InterlockedExchange(&m_val,with);
#elif defined(USE_x386_ASM_ATOMIC)
	cmx_uint_32 prev = with;

	asm volatile ("lock; xchgl %0, %1"
		: "=r" (prev)
		: "m" (m_val), "0"(prev)
		: "memory");
	return prev;
#endif
	}
private:
#if defined(WIN32)
	LONG			m_val;
#elif defined(USE_x386_ASM_ATOMIC)
	volatile unsigned int 		m_val;

#endif
};