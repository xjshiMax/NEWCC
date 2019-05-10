//2019/3/22
//xjshi 
/*
主要实现tcp 客户端 或者服务端接收数据。
*/
//一次最大读取8k字节。
#pragma once
#define MAXREADSIZE		1024*8
#include "xbaseclass.h"
#include "basesock.h"
namespace SEABASE{

	class xItcpserver:public xReceivebackbase
	{
	public:
		virtual int Onaccept(int socketfd,char*data,int len,IN xReceivebackbase**clientHandle=NULL)=0;
		virtual void Ondata(int socketfd,char*date,int len){return ;}
		virtual void Onclose(int socketfd){return ;}
	};
	class Eventcallback
	{
	public:
		static void InitEvent(xEvent_t& e, SOCKET sock, void *arg,pcallbackptr func);
		static void AcceptCallback(int sockfd,xEventDemultiplexer*Demultiplexer,void *arg);
		static void DataCallback(int sockfd,xEventDemultiplexer*Demultiplexer,void *arg);
		static void ErrCallback(int sockfd,xEventDemultiplexer*Demultiplexer,void *arg);
	};

}