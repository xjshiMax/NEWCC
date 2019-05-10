//2018-12-6
//没有结构体的队列，也不依赖stl
//boost用在封装reactor里

//2018-12-7
//线程安全的任务队列，可结合线程池使用。
//应该说该队列就是准备给多线程的生产者---消费者模式的队列。
//在多线程里定义一个队列。可供任意线程是的存取任务对象。
//且实现逻辑：队列空的时候，读队列线程等待任务到来，写入队列的线程可以任意加入任务。
//           队列满的时候，读队列线程可随意读，写入队列的线程等待。
//           正常情况，队列未满且不空，任意线程读取（消耗），任意线程写入（生产）
#pragma once
#include "xAutoLock.h"
#include <deque>
namespace SEABASE{
template <typename Taskobj>
class xTaskqueue
{
public:
	typedef typename std::deque<Taskobj> Tasklist;
	typedef typename std::deque<Taskobj>::iterator iterator;
	typedef typename std::deque<Taskobj>::const_iterator const_iterator;
	xTaskqueue(size_t size=1000):m_IsActive(true),
		m_Maxsize(size),m_CurTaskCount(0),m_CurReadWaiter(0),
		m_CurWriteWaiter(0)
	{}
		
	virtual ~xTaskqueue(void) {clearAllTask();}

	//返回是否active
	bool isActive(){return m_IsActive;}
	//设置任务队列为active状态
	void setActive(void)
	{
		xAutoLock L(m_LockTask);
		m_IsActive=true;
	}
	//设置任务队列为不活跃状态
	void setDeadstatus(void);
	//清除队列所有任务
	void clearAllTask(void);
	//唤醒队列
	void wakeup(void);

	void resizeQueue(size_t MaxSize);

	//获取队列中任务数量
	size_t getTaskCount(void) const {return m_CurTaskCount;}
	//获取队列中等待的read waiter线程。
	size_t getCurReadTaskWaiter(void) const {return m_CurReadWaiter;}
	//获取当前写等待 writer waiter
	size_t getCurWriteTaskWaiter(void) const {return m_CurWriteWaiter;}
 	size_t getMaxSize(void)		const {return m_Maxsize;}			//获取队列最大容量
 	bool queueIsFull(void)	const {return m_CurTaskCount >= m_Maxsize;}				//队列是否已满
 	bool queueIsEmpty(void)	const {return m_CurTaskCount ==0;}				//队列是否为空

	//等待任务，并传入超时时间，默认是无限等待，直到信号到来
	//如果数据带来，通过出参 node传出。
	bool waitForTask(Taskobj&node, const struct timespec& timeval_ = maxTimeWait);
	bool pushTask(const Taskobj& node);

	bool pushTaskWithTimeOut(const Taskobj& node,const struct timespec & timeval=maxTimeWait);
	xMutex getTaskLock(void) {return m_LockTask;}		//获取任务锁
	iterator begin(void) {return m_tasklist.begin();}	//获取第一个元素的迭代器
	const_iterator begin(void) const {return m_tasklist.begin();}
	iterator end(void) {return m_tasklist.end();}
	const_iterator end(void) const {return m_tasklist.end();}
protected:
	bool m_IsActive;
	size_t m_Maxsize;		//队列最大容量
	size_t m_CurTaskCount;	//当前任务数
	size_t m_CurReadWaiter;	//当前对
	size_t m_CurWriteWaiter;	//当前写等待

	std::deque<Taskobj> m_tasklist; //任务队列
	xMutex m_LockTask;				//任务锁，配合条件变量一起使用
	xCondition m_CondTask;			//条件变量，用来阻塞线程，等待任务。
	xCondition m_CondTaskFree;
};
template <class Taskobj>
void xTaskqueue<Taskobj>::setDeadstatus(void)
{
	xAutoLock L(m_LockTask);
	m_IsActive = false;
	m_CondTask.broadCast();
	m_CondTaskFree.broadCast();
}
template <class Taskobj>
void xTaskqueue<Taskobj>::clearAllTask(void)
{
	xAutoLock L(m_LockTask);
	m_tasklist.clear();
	m_CurTaskCount=0;
	m_CondTask.broadCast();
	m_CondTaskFree.broadCast();
}
template <class Taskobj>
void xTaskqueue<Taskobj>::wakeup(void)
{
	xAutoLock L(m_LockTask);
	m_CondTask.broadCast();
	m_CondTaskFree.broadCast();
}
template <class Taskobj>
void xTaskqueue<Taskobj>::resizeQueue(size_t MaxSize)
{
	xAutoLock L(m_LockTask);
	m_Maxsize=MaxSize;
}
template <class Taskobj>
bool xTaskqueue<Taskobj>::waitForTask(Taskobj&node,const struct timespec& timeval_)
{
	xAutoLock L(m_LockTask);
	if(!m_IsActive)
	{
		return false;
	}
	if(m_tasklist.empty())
	{
		if(TimevalIsequal(timeval_ ,maxTimeWait))
		{
			++m_CurReadWaiter;
			m_CondTask.wait(m_LockTask);
			--m_CurReadWaiter;
		}
		else if(TimevalIsequal(timeval_ , zeroTimeWait))
		{
			return false;
		}
		else
		{
			++m_CurReadWaiter;
			m_CondTask.timewait(m_LockTask,timeval_);
			--m_CurReadWaiter;
		}
		if(!m_IsActive || m_tasklist.empty())
			return false;
	}
	node = m_tasklist.front();
	m_tasklist.pop_front();
	--m_CurTaskCount;
	if(m_CurWriteWaiter)
		m_CondTaskFree.signal();
	return true;
}
template <class Taskobj>
bool xTaskqueue<Taskobj>::pushTask(const Taskobj& node)
{
	xAutoLock L(m_LockTask);
	if(!m_IsActive)
		return false;
// 	if(queueIsFull())
// 	{
// 		++m_CurWriteWaiter; 
// 		m_CondTaskFree.wait(m_LockTask);
// 		--m_CurWriteWaiter;
// 	}
	m_tasklist.push_back(node);
	++m_CurTaskCount;
	if(m_CurReadWaiter)
		m_CondTask.signal();
	return true;
}
template <class Taskobj>
bool xTaskqueue<Taskobj>::pushTaskWithTimeOut(const Taskobj& node,const struct timespec & timeval)
{
	xAutoLock L(m_LockTask);
	if(!m_IsActive)
		return false;
	if(queueIsFull())
	{
		if(TimevalIsequal(timeval ,maxTimeWait))
		{
			++m_CurWriteWaiter;
			m_CondTaskFree.wait(m_LockTask);
			--m_CurWriteWaiter;
		}
		else if (TimevalIsequal(timeval ,zeroTimeWait))
			return false;
		else
		{
			++m_CurWriteWaiter;
			m_CondTaskFree.timewait(m_LockTask,timeval);
			--m_CurWriteWaiter;
		}
		if(!m_IsActive || queueIsFull())
			return false;
	}
	m_tasklist.push_back((Taskobj)node);
	++m_CurTaskCount;
	if(m_CurReadWaiter)
		m_CondTask.signal();
	return true;
}
}
