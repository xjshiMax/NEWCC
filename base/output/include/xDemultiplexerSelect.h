//2018-12-17
// 基于select io 复用模式，实现分离器
/*
1）继承	xEventDemultiplexer。 实现相应的接口
2) 默认读，写，错误 最多各注册64个fd.
*/
#pragma once
#include "xbaseclass.h"
#include "basesock.h"
namespace SEABASE{

	class xSelectDemultiplexer:public xEventDemultiplexer
	{
	public:
		xSelectDemultiplexer();
		virtual ~xSelectDemultiplexer();
		virtual int WaitEvents(int timeout=1,xtime_heap* event_timer=NULL );
		//添加或则修改文件描述符的注册事件信息
		virtual int RequestEvent(xEvent_t&e);


		virtual int UnrequestEvent(handle_t handle);
	private:
		int m_epoll_fd;  //epoll_creat 返回的描述符
		int m_fd_num;	  // 当前加入集合的描述符数量
		int m_maxfdID;	  //最大fd值加1.在select模型中常用这个代替数量。
		//fd_set m_fdread;
		//fd_set m_fdError;
		//fd_set m_fdwrite;
		fd_set m_fdReadSave;
		std::vector<handle_t> m_Readevents;
	};

}