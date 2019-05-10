#pragma once
//#include <Winsock2.h>
#ifdef WIN32
	#include <process.h>
	#include <Windows.h>
#else
	#define __stdcall
	#include<pthread.h>
#endif
#include "xsema.h"
//#include "xAutoLock.h"
namespace SEABASE{

#pragma once
#ifdef  WIN32
	typedef unsigned int (__stdcall*pfunc)(void*);
#else
	typedef void* (__stdcall pfunc)(void*);
#endif
	enum threadstatus
	{
		INIT,
		START,
		JOINED,
		STOP
	};

//基于对象的模式
class Threadbase
{
public:
	Threadbase(bool bDetach=true);
	virtual ~Threadbase();
	/*
	在继承run 以后，业务执行如果使用循环，切记在析构之前退出循环，否则线程会一直运行。
	*/
	virtual void run()=0;		//业务接口
	int start();			//启动线程
	int join();				//等待线程结束
	void destory();			//销毁线程所申请的资源

	int get_thread_id(){return thr_id;}
	int set_thread_id(unsigned long thrID){thr_id=thrID;}

protected:
#ifdef WIN32
	static unsigned int __stdcall thread_proxy(void* arg);
#else
	static void*  __stdcall thread_proxy(void* arg);
#endif
private:
#ifdef WIN32
	size_t thr_id;
#else
	pthread_t thr_id;
#endif
	bool bExit_;			//线程是否要退出标志
	threadstatus m_state;
	xSemaphore m_sema;
};
//面向对象的模式
class xThread
{
public:
	xThread(bool bDetach=true):thr_id(0),m_state(INIT)
	{
	}
	virtual ~xThread(){
		destory();
		m_state=STOP;
	};
	int start(pfunc func,void *arg);			//启动线程
	int join();				//等待线程结束
	void destory();			//销毁线程所申请的资源

	int get_thread_id(){return thr_id;}
	int set_thread_id(unsigned long thrID){thr_id=thrID;}
public:
#ifdef WIN32
	size_t thr_id;
#else
	pthread_t thr_id;
#endif
	bool bExit_;
	threadstatus m_state;
};





}