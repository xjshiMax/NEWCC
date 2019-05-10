//xjshi 2019-4-15
//同步队列，用来存消息，存取效率高
#pragma once
#include <queue>
#include "xEmutex.h"
#include "xsema.h"
using namespace std;
namespace SEABASE{

const uint32_t MAX_QUEUE_COUNT = 1000000;
template<typename T>
class xSyncQueue
{
public:
	xSyncQueue();
	~xSyncQueue();
	bool Empty();
/*	bool Empty();*/
	typename queue<T>::size_type Size();
	void BlockFront(T& value);
	bool Pop(T& value);
	void BlockPop();
	void BlockPop(T& value);
	void HarfBlockPop(T& value);
	void Push(const T& value);
	void BlockPush(const T& value);
	void HarfBlockPush(const T& value);

public:
	queue<T> m_queue;
	xEmutex m_lock;
	xSemaphore m_Fullsema;
	xSemaphore m_Emptysema;
};

template<typename T>
xSyncQueue<T>::xSyncQueue():m_Fullsema(0), m_Emptysema(MAX_QUEUE_COUNT)
{

}
template<typename T>
xSyncQueue<T>::~xSyncQueue() {
}

template<typename T>
bool xSyncQueue<T>::Empty() {
	//SingleLocker s(&mLocker);
	xGuard guard(m_lock);
	return mQueue.empty();
}

template<typename T>
typename queue<T>::size_type xSyncQueue<T>::Size() {
	xGuard guard(m_lock);
	return m_queue.size();
}

template<typename T>
void xSyncQueue<T>::BlockFront(T& value) {
	m_Fullsema.wait();
	m_lock.lock();
	value = m_queue.front();
	m_lock.unlock();
	m_Fullsema.signal();
}

template<typename T>
bool xSyncQueue<T>::Pop(T& value) {
	xGuard guard(m_lock);

	if (m_queue.empty()) {
		return false;
	}

	value = m_queue.front();
	m_queue.pop();
	return true;
}

template<typename T>
void xSyncQueue<T>::BlockPop() {
	m_Fullsema.wait();
	m_lock.lock();
	m_queue.pop();
	m_lock.unlock();
	m_Emptysema.signal();
}

template<typename T>
void xSyncQueue<T>::BlockPop(T& value) {
	m_Fullsema.wait();
	m_lock.lock();
	value = m_queue.front();
	m_queue.pop();
	m_lock.unlock();
	m_Emptysema.signal();
}

template<typename T>
void xSyncQueue<T>::HarfBlockPop(T& value) {
	m_Fullsema.wait();
	m_lock.lock();
	value = m_queue.front();
	m_queue.pop();
	m_lock.unlock();
}

template<typename T>
void xSyncQueue<T>::Push(const T& value) {
	xGuard guard(m_lock);

	if (m_queue.size() >= MAX_QUEUE_COUNT) { //maybe overflow, need log out
		m_queue.pop();
	}

	m_queue.push(value);
}

template<typename T>
void xSyncQueue<T>::BlockPush(const T& value) {
	m_Emptysema.wait();
	m_lock.lock();
	m_queue.push(value);
	m_lock.unlock();
	m_Fullsema.signal();
}

template<typename T>
void xSyncQueue<T>::HarfBlockPush(const T& value) {
	bool isFull = false;
	m_lock.lock();

	if (m_queue.size() >= MAX_QUEUE_COUNT) { //maybe overflow, need log out
		isFull = true;
		m_queue.pop();
	}

	m_queue.push(value);
	m_lock.unlock();

	if (!isFull) {
		m_Fullsema.signal();
	}
}


}