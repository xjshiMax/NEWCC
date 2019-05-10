//2018-12-18添加定时器基类
/*
1）定时器单独开一个线程，
2）继承定时器基类，然后可以一直执行。
3) 确保在主线程会等待，不会马上退出，不然定时器线程也没有足够的时间执行。
*/
//#include "xAutoLock.h"
#include "xsema.h"
#include "xthreadbase.h"
namespace SEABASE{
	//定时器，毫秒级。
	class OnTimerBase:public Threadbase
	{
	public:	
		OnTimerBase(int timeout):m_TimeOut(timeout),m_bIsstop(false){}
		int startTimer()
		{
			return start();
		}
		void stopTimer()
		{
			destory();
			m_bIsstop=true;
		}
		virtual void timeout()=0;
	private:
		virtual void run()
		{
			while(!m_bIsstop){
				m_sema.wait(m_TimeOut);
				this->timeout();
			}

		}
	private:
		int m_TimeOut;
		xSemaphore m_sema;
		bool m_bIsstop;
	};
}