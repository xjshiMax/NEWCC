//2019-4-15
//xjshi 
//添加消息put-get模型处理机制
//直接修改run（）里面的内容使用 或者继承以后重写run方法。
#pragma  once
#include "xsync_queue.h"
#include "xthreadbase.h"
namespace SEABASE{

template <class element>
class xEmsgqueue:public Threadbase
{
public:
	xEmsgqueue():m_flag(true){}
	~xEmsgqueue(){};
	virtual void run();
	int stopMsgqueue();
	void in_queue(element* p_event);
public:
	bool m_flag;
	xSyncQueue<element*> m_eventqueue;
};
template <class element>
void xEmsgqueue<element>::run()
{
	while(m_flag)
	{
		element* tvalue;
		m_eventqueue.BlockPop(tvalue);
		if(tvalue)
		{
			int i=0;
			//do something
		}
	}
}
template <class element>
int xEmsgqueue<element>::stopMsgqueue()
{
	m_flag=false;
}
template <class element>
void xEmsgqueue<element>::in_queue(element* p_event)
{
	m_eventqueue.BlockPush(p_event);
}
}