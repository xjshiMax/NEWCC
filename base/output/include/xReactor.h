//2018/12/10
//简单封装reactor

#pragma  once 


#include <map>
#include "xbaseclass.h"
#include "xtimeheap.h"
#include "xNetdata.h"
#ifdef WIN32
	#include "xDemultiplexerSelect.h"
#else
	#include "xEventDemultiplexer.h"
#endif

namespace SEABASE
{
	class datatype
	{
	public:
		enum databackType
		{
			TcpAcceptCallback,
			TcpDataCallback
		};
	};
#define INITSIZE 100
	class xReactorImplentation
	{
	public:
		//这里给定定时器最小堆的最大大小为 INITSIZE=100.如果超过会自动resize()
		xReactorImplentation()
		{
#ifdef WIN32
			m_demultiplexer = new xSelectDemultiplexer();
#else
			m_demultiplexer = new xEpollDemultiplexer();
#endif
			//m_demultiplexer = static_cast<xEventDemultiplexer*>(new xEpollDemultiplexer());
			m_eventtimer = new xtime_heap(INITSIZE);
		}
		~xReactorImplentation()
		{
			delete m_demultiplexer;
		}
		int RegisterHandler(xReceivebackbase*handler,handle_t&sockfd,int type=datatype::TcpAcceptCallback); //给tcp client 注册
		int RemoveHandlerbyfd(handle_t handlefd);
		void HandlerEvents();
		void StopEventLoop()
		{
			_bIsstop=true;
		}
		void StartLoop()
		{
			_bIsstop=false;
			HandlerEvents();
		}
		int RegisterTimeTask(xheaptimer* timerevent);
	private:
		xEventDemultiplexer *		m_demultiplexer;
		//std::map<handle_t,xEventHandler*> m_handlers;
		xtime_heap* m_eventtimer;
		bool _bIsstop;
	};

class xReactor
{
public:
	xReactor();

// 	{
// 		m_reactorimp = new xReactorImplentation();
// 	}
	virtual ~xReactor();
	int start() //启动事件循环
	{
		HandlerEvents();
		return 0;
	}
	int stop()
	{
		m_reactorimp->StopEventLoop();
	}
	//注册事件，事件响应对象和事件类型，读写

	int RegisterHandler(xReceivebackbase*handler,handle_t&sockfd,int type=datatype::TcpAcceptCallback);
	//int RemoveHandler(xEventHandler* handler);
	int	RemoveHandlerbyfd(handle_t handlefd);				//针对tcp等需要根据fd来注销事件的场景添加。
	void HandlerEvents();

	int RegisterTimeTask(xheaptimer* timerevent);
	private:
		xReactor(const xReactor&);
		xReactor & operator=(const xReactor&);
private:

	xReactorImplentation* m_reactorimp;

};

//reactor 单例
class ReactorInstance
{
public:
	static xReactor* GetInstance()
	{
		if(m_reactor==NULL)
			m_reactor=new xReactor();
		return m_reactor;
	}
private:
	static xReactor* m_reactor;;
};

}