#pragma  once
/*#include "base/output/include/xEMsgqueue.h"*/
#include "base/output/include/xEmutex.h"
#include "base/output/include/xsema.h"
#include "base/output/include/xthreadbase.h"
#include "IVRmanager.h"
#include <queue>
using namespace SEABASE;
template <typename ElemType>
class priorityqueue
{
private:
	xSemaphore* m_psem;
	xEmutex m_mutex;
	std::priority_queue<ElemType> m_queue;
public:
	priorityqueue()
	{
		m_psem=new xSemaphore;
	}
	~priorityqueue()
	{
		if(m_psem!=NULL)
			delete m_psem;
	}
	int32_t put(ElemType elem)
	{
		//xAutoLock L(m_mutex);
		xGuard<xEmutex> autolock(&m_mutex);
		try{
			m_queue.push(elem);
		}catch(std::bad_alloc &){
			return -1;
		}
		m_psem->signal();
		return 0;
	}
	int32_t get(ElemType&elem,int millisecond)
	{
		int32_t ret = m_psem->wait(millisecond);
		if(0 == ret)
		{
			xGuard<xEmutex> autolock(&m_mutex);
			elem = m_queue.top();
			m_queue.pop();
		}
		else
		{
			return -1;
		}
		return 0;
	}
// 	int32_t checktop(ElemType&elem,int millisecond)
// 	{
// 		int32_t ret = m_psem->wait(millisecond);
// 		if(0 == ret)
// 		{
// 			xGuard<xEmutex> autolock(&m_mutex);
// 			elem = m_queue.top();
// 			//m_queue.pop();
// 		}
// 		else
// 		{
// 			return -1;
// 		}
// 		return 0;
// 	}

	int32_t size() {
		return (int32_t)m_queue.size();
	}
	void clear()
	{
		xGuard<xEmutex> autolock(&m_mutex);
		while(m_queue.size()){
			m_queue.pop();
		}
	}
protected:
	priorityqueue(const priorityqueue&);
	priorityqueue& operator=(const priorityqueue&);
};




//template <class element>
class ACDqueue:public Threadbase
{
	
	public:
		ACDqueue():m_flag(true){m_psem=new xSemaphore;}
		~ACDqueue(){
			if(m_psem!=NULL)
				delete m_psem;
		};
		virtual void run();
		int stopMsgqueue()
		{
			m_flag=false;
		}
		void in_queue(ivrsession p_event)
		{
			m_q.put(p_event);
			return ;
		}
	public:
		bool m_flag;
		priorityqueue<ivrsession> m_q;
		xSemaphore* m_psem;
} ;