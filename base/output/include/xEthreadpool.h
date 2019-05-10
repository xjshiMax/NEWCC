//2019-4-17 新版线程池 简单稳定
/*
addTask 添加任务，然后等待线程去执行，在分配到线程以后，
去对应任务的Onrun（）方法去执行。
进入Onrun（） 以后只有工作线程一个。
*/

#include "xEmutex.h"
#include "xsema.h"
#include "xsync_vector.h"
#include "xsmartPtr.h"
#include <vector>
#ifdef WIN32
#include <process.h>
#include <Windows.h>
#else
#define __stdcall
#include<pthread.h>
#endif
namespace SEABASE
{
	typedef void* (*threadfunc_t)(const bool*, void*);

	/*
	 带有原子计数功能的类
	*/
	class xShareable
	{
	public:
		virtual ~xShareable(){}
		void inc();
		int32_t dec();
		int32_t get_count();
	protected:
		xShareable();
	private:
		int32_t m_count;
		xEmutex m_mutex;

	};
	/*
		线程运行基类，在创建线程时，需要先继承，实现子类，然后当参数传入
	*/
	class xRunable:public xShareable
	{
	public:
		virtual int32_t Onrun(const bool * isstoped,void*param=NULL)=0;
		virtual ~xRunable(){};
	};

	/*
		面向对象的线程基类，
	*/

	class xEthreadbase:public xShareable
	{
	private:
		enum state_t{
			INIT,
			START,
			JOINED,
			STOP
		};
	public:
		xEthreadbase(xsmartPtr<xRunable>runner,bool detached = false);
		xEthreadbase(threadfunc_t func, void* arg = NULL, bool detached = false);
		~xEthreadbase();
		bool start();		//开始线程
		bool join();		//等待线程
		bool stop();		//停止线程
#ifdef WIN32
		unsigned int get_thread_id() const;  //返回线程id
		size_t gethandle();			  //返回线程句柄
#else
		pthread_t get_thread_id() const; // 返回线程id
#endif
	private:
		bool setDetached();
#ifdef WIN32
		static unsigned int __stdcall thread_proxy(void* arg);
#else
		static void*  __stdcall thread_proxy(void* arg);
#endif
		bool m_use_functor;
		xsmartPtr<xRunable>m_runfuncptr;
		threadfunc_t m_funcptr;
		void* m_func_arg;
		xSemaphore m_sema;
		volatile bool m_detached;
		state_t m_state;

		bool m_isstoped;
#ifdef WIN32
		size_t _handle;
		unsigned int _thread_id;
#else
		pthread_t _thread_id;
#endif
	};

	/*
		线程组，管理一组线程，方便添加和关闭线程。
	*/
	class xThreadGroup
	{
	public:
		xThreadGroup();
		~xThreadGroup();
		bool addThread(xsmartPtr<xEthreadbase>thread);
		bool join();		//等待所有的线程结束
		size_t size();		//获取线程个数
		bool terminateAll();//终止所有线程
	private:
		xThreadGroup(const xThreadGroup&);
		xThreadGroup& operator=(const xThreadGroup&);

		std::vector<xsmartPtr<xEthreadbase> > m_threads;
		typedef std::vector<xsmartPtr<xEthreadbase> >::const_iterator citr_type;
	};

	/*
		简单线程池，维护一组预先创建好的线程以及一个任务队列。线程依次执行任务队列中的任务
	*/
	class xThreadPool:public xShareable
	{
	private:
		class ThreadPoolRunner;
	public:
		enum state_t{
			 UNINITIALIZED, INITIALIZED
		};
		xThreadPool();
		~xThreadPool();
		static const int DEFAULT_THREADS_NUM = 10; //默认线程数量
		int init(int nThreads=DEFAULT_THREADS_NUM); //	初始化线程池，指定线程数量
		bool addTask(xRunable* runinst);			// 	添加任务
		bool join();								//等待所有线程结束
		size_t size();			//获取线程池中线程数量
		bool terninate();		//终止所有线程的执行
	private:
		int addthread(int nthreads);	//添加nthreads个线程
		xSyncVector<xRunable*> m_tasks;	//任务队列
		xThreadGroup m_threadgroup;		//线程组
		state_t m_state;				//线程池状态

	};
}

