//2018-11-29
//自动锁
//pthread_mutex 和Mutex
#pragma once
#ifdef WIN32
	#include <Windows.h>
	#include "..\pthread\Pre-built.2\include\pthread.h"
//#pragma comment 相对路径是引用工程到库路径的相对路径，而非xAutoLock.hpp到库目录的相对路径。
	#pragma comment(lib,"pthreadVC2.lib")
	#pragma comment(lib,"pthreadVCE2.lib")
	#pragma comment(lib,"pthreadVSE2.lib")
#else
	#include <pthread.h>
	#include<sys/time.h>
#endif
#include "xbaseclass.h"
namespace SEABASE{
#define Mutex xMutex
#define CONST_NAO_PER_SECOND 	1000000000L
#define CONST_NAO_PER_MICRO 	1000L
static const struct timespec maxTimeWait={-1,-1};  //设置该超时时间会无限等待，直到有信号产生
static const struct timespec zeroTimeWait={0,0};
inline static bool TimevalIsequal(const struct timespec&first,const struct timespec&second)
{
	if(first.tv_sec == second.tv_sec && first.tv_nsec==second.tv_nsec)
		return true;
	return false;
}

class xMutex
{
public:
	xMutex(){pthread_mutex_init(&m_lock,NULL);}
	~xMutex(){pthread_mutex_destroy(&m_lock);}
	void lock() const 
	{
		pthread_mutex_lock(&m_lock);
	}
	void unlock() const 
	{
		pthread_mutex_unlock(&m_lock);
	}
	bool tryLock() const 
	{
		pthread_mutex_trylock(&m_lock);
	}
private:
	mutable pthread_mutex_t m_lock;
	friend class xCondition;
};
class xAutoLock
{
public:
	//xAutoLock(const Mutex* mutex):m_mutex(mutex)
	//{
	//	//pthread_mutex_lock(&m_mutex);
	//	m_mutex->lock();
	//	printf("lock\n");
	//}
	xAutoLock(const Mutex & mutex):m_mutex(&mutex)
	{
		//pthread_mutex_lock(&m_mutex);
		m_mutex->lock();
		//printf("lock\n");
	}
	~xAutoLock()
	{
		//pthread_mutex_unlock(&m_mutex);
		if(m_mutex)
			m_mutex->unlock();
		m_mutex=NULL;
		//printf("unlock\n");
	}
private:
	const Mutex* m_mutex;
};

//条件信号
class xCondition:protected Noncopyable
{
public:
	xCondition(void)throw(){pthread_cond_init(&m_cond,NULL);}
	~xCondition(){pthread_cond_destroy(&m_cond);}

	//激活一个等待线程，存在多个等待线程时按入队顺序激活其中一个。
	void signal(void)
	{
		int Ret = pthread_cond_signal(&m_cond);
			
	}
	//激活所有等待线程
	void broadCast(void)
	{
		int Ret = pthread_cond_broadcast(&m_cond);
	}
	//wait 等待条件的触发
	//在调用wait以及timewait接口之前，一定要加锁，避免多线程同时访问。
	void wait(xMutex &lock)
	{
		int Ret = pthread_cond_wait(&m_cond,&(lock.m_lock));
	}
	// timewait 时间等待。如果在给定时刻前条件没有满足，则返回ETIMEOUT,结束等待。
	//其中参数abstime以于time()系统调用相同意义的绝对时间形式出现，0表示1970年1月1日0时0分0秒
	bool timewait(xMutex &lock,const struct timespec&abstime)
	{
		if( abstime.tv_sec == maxTimeWait.tv_sec &&abstime.tv_nsec==maxTimeWait.tv_nsec)
		{
			wait(lock);
			return true;
		}
		//如果传入时间参数不是无限等待，则设置超时时间为现在时间加上传入的时间间隔
#if defined(WIN32)
		SYSTEMTIME	system_time;
		FILETIME	ft;
		GetLocalTime( &system_time );
		SystemTimeToFileTime(&system_time, &ft);  // converts to file time format

		LONGLONG nLL;
		ULARGE_INTEGER ui;
		ui.LowPart = ft.dwLowDateTime;
		ui.HighPart = ft.dwHighDateTime;
		nLL = (ft.dwHighDateTime << 32) + ft.dwLowDateTime;
		long  _nSec = (long)((LONGLONG)(ui.QuadPart - 116444736000000000)/10000000-28800);
		long _nNSec = (long)((LONGLONG)(ui.QuadPart - 116444736000000000)*100%1000000000);
#else
		timeval time_value;
		gettimeofday( &time_value, NULL );
		LONGLONG _nSec = time_value.tv_sec;
		LONGLONG _nNSec = time_value.tv_usec * CONST_NAO_PER_MICRO;
#endif // CMX_WIN32_VER
		struct timespec timeval_;
		timeval_.tv_sec=abstime.tv_sec;
		timeval_.tv_nsec=abstime.tv_nsec;
		timeval_.tv_sec+=_nSec;
		timeval_.tv_nsec+=_nNSec;
		if(timeval_.tv_nsec>=CONST_NAO_PER_SECOND)
		{
			timeval_.tv_nsec-=CONST_NAO_PER_SECOND;
			timeval_.tv_sec+=1;
		}

 		return timeWaitUntil(lock,timeval_);
	//	return true;
	}
	//如果超时，就返回，如果等到变量信号，也返回。
	bool timeWaitUntil(xMutex&lock,const struct timespec& waitTime)
	{
		if(waitTime.tv_sec == maxTimeWait.tv_sec &&waitTime.tv_nsec==maxTimeWait.tv_nsec)
		{
			wait(lock);
			return true;
		}
		struct timespec abstime;
		abstime.tv_sec = waitTime.tv_sec;
		abstime.tv_nsec = waitTime.tv_nsec;
		//Ret为0正确，其他调用失败
		int Ret = pthread_cond_timedwait(&m_cond,&(lock.m_lock),&abstime);
		return true;
	}
protected:
// 	void condUnblock(int nUnBlockAll);
// 	long m_WaitersBlocked;			//阻塞的线程数
// 	long m_WaitersGone;				//超时的线程数
// 	long m_WaitersUnblocked;		//未阻塞的线程数

	pthread_cond_t m_cond;			//pthread_cond_t 表示多线程的条件变量，用于控制线程等待和就绪的条件。
};

//测试。 waitfuc()里面遇到wait（）就会阻塞，等待信号。
//xMutex mymutex;
//xCondition myCondition;
//static unsigned int __stdcall waitfuc(void *)
//{
//	xAutoLock L(mymutex);
//	printf("i am waittinf for the signal\n");
//	myCondition.wait(mymutex);
//	printf("---it coming!\n ");
//	return 0;
//}
//static unsigned int __stdcall siginalfuc(void *)
//{
//	xAutoLock L(mymutex);
//	printf("give the sigianl;\n");
//	myCondition.signal();
//	return 0;
//}
//void testcondition()
//{
//	xThread mythread[2];
//	mythread[0].start(waitfuc,"");
//	//xMutex mutex;
//	Sleep(2000);
//	mythread[1].start(siginalfuc,"");
//	mythread[0].join();
//	mythread[1].join();
//
//}

}