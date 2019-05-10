//2018-12-18
//xheaptimer 作为定时器使用方法，
//1）定义xheaptimer对象
//2）定义xreactor，然后注册定时事件，启动xreactor
//3）该定时器只作用一次。
#pragma once
#include<iostream>
#ifdef WIN32
#include <time.h>
#else
#include <netinet/in.h>
#include <time.h>
#endif
namespace SEABASE{
#define BUFFER_SIZE 64


//使用小根对来实现定时器
class xheaptimer
{
public:
	//延时时间，单位秒s.
	xheaptimer(int delay)
	{
		expire = time(NULL)+delay;
	}
public:
	time_t expire;
	void (*cb_func)(void*);
	void* user_data;
};

//小根堆的实现
class xtime_heap
{
public:
	xtime_heap(int cap) throw(std::exception)
		:m_capacity(cap),m_cur_size(0)
	{
		array = new xheaptimer*[m_capacity];
		if(!array)
		{
			throw std::exception();
		}
		for(int i=0;i<m_capacity;i++)
		{
			array[i]=NULL;
		}
	}
	xtime_heap(xheaptimer** init_array,int size,int capacity) throw(std::exception)
		:m_cur_size(size),m_capacity(capacity)
	{
		if(m_capacity < size)  //超过堆的最大容量，抛异常
		{
			throw std::exception();
		}
		array = new xheaptimer*[m_capacity];
		if(!array) throw std::exception();
		for(int i=0;i<m_capacity;i++)
		{
			array[i]=NULL;
		}
		if(size!=0)
		{
			for(int i=1;i<size;i++)
			{
				array[i]=init_array[i];
			}
			for(int i=(m_cur_size)/2;i>=0;--i)
			{
				percolate_down(i);
			}
		}
	}
	virtual ~xtime_heap()
	{

	}
public:
	void add_timer(xheaptimer*timer)throw(std::exception);
	void del_timer(xheaptimer* timer);
	xheaptimer * top()const ;
	void pop_timer();
	void tick();
	bool empty()const {return m_cur_size==0;}
private:
	void percolate_down(int hole);  //最小堆调整
	void resize()throw(std::exception);



private:
	xheaptimer**array;
	int m_capacity;
	int m_cur_size;
};

}