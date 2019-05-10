//2018-12-5
//创建基于对象的线程池
//      threadpool --------taskbase.run()
//2019-3-3 修改线程池 by xjshi
/* 
xthreadPool 作为一个管理类，不需要wait操作。统一的任务池
threadobj在类外边定义带线程的对象，但是没有自己的任务池。
*/
#pragma once
#include <stdio.h>
#include <string>
#include <list>
#include <exception>
#include "xAutoLock.h"
#include "xbaseclass.h"
#include "xthreadbase.h"
#include "xTaskqueue.h"
#ifndef WIN32
#define OUT 
#define IN 
#endif
using namespace std;
namespace SEABASE{
	//任务对象的即基类。每个job都是xtaskbase派生类的对象
	//对于继承xtaskbase的类，最好显示写构造和析构函数。
	class xtaskbase
	{
	public:
		xtaskbase(void *arg=NULL,const std::string taskName = "")
			:arg_(arg),taskName_(taskName)
		{

		}
		virtual ~xtaskbase(){}
		void setArg(void *arg){arg_=arg;};
		virtual int run() {return 0;} ; //任务类重写run，在里面实现业务逻辑。
	protected:
		void* arg_;
		std::string taskName_;

	};

	class xsimpleThreadPool
	{
	public:
		xsimpleThreadPool(){}
		xsimpleThreadPool(const char*poolname):m_PoolName(poolname){}
		~xsimpleThreadPool()
		{
			stopPool();
			xAutoLock L(m_threadLock);
		}
		void initPool(size_t LowThreadNumber=4)
		{
			m_threadNum=LowThreadNumber;
		}
		void startPool(bool defaultpools=false )
		{
			if(!shutdown)
				return;
			shutdown=false;
			for(int i=0;i<m_threadNum;i++)
			{
				xThread *workThread=new xThread;
				workThread->start(threadproxy,this);
				m_threadList.push_back(workThread);
			}
		}
		void stopPool(bool defaultpools=true)
		{
			if(shutdown)
			{
				return  ;
			}
			shutdown = true;
			m_threadCond.broadCast();
			for( vector<xThread*>::iterator ite=m_threadList.begin();ite!=m_threadList.end();)
			{
				(*ite)->join();
				(*ite)->destory();
				ite=m_threadList.erase(ite++);
			}
		}	
		void pushObj(xtaskbase*task)
		{
			xAutoLock L(m_threadLock);
			m_taskList.push_back(task);
			m_threadCond.signal();
		}
		void waitforAllTaskDone(void)
		{
			for( vector<xThread*>::iterator ite=m_threadList.begin();ite!=m_threadList.end();ite++)
			{
				(*ite)->join();
			}
		}
		void Cleartask(bool isdelete=false)
		{
			xAutoLock L(m_threadLock);
			if(isdelete)
			{
				std::deque<xtaskbase*>::iterator threadite=m_taskList.begin();
				while(threadite!=m_taskList.end())
				{
					xtaskbase* ptask = *threadite;
					if(ptask)
						delete ptask;
					//threadite->erasr
				}
			}
			m_taskList.clear();
		}
#ifdef WIN32
		static unsigned int __stdcall threadproxy(void* arg);
#else
		static void*  __stdcall threadproxy(void* arg);
#endif

	private:
		static std::deque<xtaskbase*> m_taskList;
		std::vector<xThread*> m_threadList;
		int m_threadNum;
		static xMutex m_threadLock;
		static xCondition m_threadCond;
		static bool shutdown;
		string m_PoolName;
	};
	//需要设置成不可复制的类
	class xthreadPool
	{
	protected:
		xthreadPool(const xthreadPool&);
		xthreadPool& operator=(const xthreadPool&);
	public:
		enum state_t {
			UNINITIALIZED, INITIALIZED
		};
		xthreadPool()
		{
		}
		virtual ~xthreadPool(){this->stopPool();};
		//初始化线程池
		void initPool(size_t LowThreadNumber=4);
		void startPool(bool defaultpools=false );
		void stopPool(bool defaultpools=true);  //默认defaultpools为true，清除掉所有线程
		//push的时候注意，尽量不要使用局部变量，否则在还没有调用任务的run之前，该对象就已经析构，会造成奇怪的问题。
		bool pushObj(xtaskbase*node,const struct timespec & Timeout=maxTimeWait);
		void joinAllThread();
		class threadobj:public Threadbase
		{
		public:
			threadobj():m_bisStop(false){}
			~threadobj(){
				endthreadobj();
			}
			void beginthreadobj(xthreadPool* threadpool)
			{
				m_bisStop=false;
				m_parentPool=threadpool;
				start();
			}
			void endthreadobj()
			{
				m_bisStop=true;
			}
			virtual void run()
			{	
				pair<xtaskbase*,bool> temptask;
				do{
					if(waitforjob(m_parentPool->m_tasklist,maxTimeWait,temptask))
					{
						xtaskbase*ptask=temptask.first;
						if(ptask)
						{
							(ptask)->run();
						}
						//m_parentPool->onFinishTask();
					}
				}while(!m_bisStop);
			}
			bool waitforjob(xTaskqueue<pair<xtaskbase*,bool> >&tasklist,const struct timespec &timeout,OUT pair<xtaskbase*,bool> &tasknode)
			{
				//pair<xtaskbase*,bool> temptask;
				if(!tasklist.waitForTask(tasknode,timeout))
					return false;
				return true;
			}
			xtaskbase* m_task;
			xMutex m_mutex;
			bool m_bisStop;
			xthreadPool* m_parentPool;
			
			friend class xthreadPool;

			//pair<xtaskbase*,bool> m_task; //
		};

	protected:
		size_t			m_threadNum;
		int				m_state;
		xMutex			m_lockForThread;
		xTaskqueue<pair<xtaskbase*, bool> >		m_tasklist;  //这里放入容器，必须得是指针才会有多态
		std::list<threadobj*> m_ThreadList;		//这里在startPool的时候不能定义局部变量，否则线程还没执行完，对象被释放了会导致一些奇怪的错误。
	};
	
};
