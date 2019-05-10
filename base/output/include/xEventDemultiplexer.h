//2018/12/11
// 分发器实现（IO复用分离时间的机制）
#pragma once
#include "xbaseclass.h"
#include "xtimeheap.h"
#ifndef WIN32
#include<sys/epoll.h>
#include <unistd.h>
#endif
#include <errno.h>
namespace SEABASE{
//epoll IO复用实现分离器
///
/*
			epoll_create()
				|
			   \|/
	fd-----> epoll_ctl(epoll_fd,EPOLL_CTL_ADD,max_epoll_size,epll_event_set)
				|
				|
			 epoll_wait()
				|-------------> event_handler()
	
*/
///
class xEpollDemultiplexer:public xEventDemultiplexer
{
public:
	xEpollDemultiplexer();
	virtual ~xEpollDemultiplexer();
	virtual int WaitEvents(int timeout=1,xtime_heap* event_timer=NULL );
	//添加或则修改文件描述符的注册事件信息
	virtual int RequestEvent(xEvent_t&e);


	virtual int UnrequestEvent(handle_t handle);
private:
	int m_epoll_fd;  //epoll_creat 返回的描述符
	int m_fd_num;	  // 当前加入集合的描述符数量
};



 }