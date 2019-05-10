//tcp client
#pragma once
#include<string>
#include "basesock.h"
//#define _IS_NEED_CALLBACK
#ifdef _IS_NEED_CALLBACK
#include "xbaseclass.h"
#endif
using namespace std;
namespace SEABASE{
#ifdef _IS_NEED_CALLBACK
	class ExTcpClient:public xReceivebackbase
#else
	class ExTcpClient
#endif
	{
	public:
		ExTcpClient():m_sockfd(-1),m_clientport(0),m_serverport(0)
		{
			memset(m_serverip,0,32);
			memset(m_clientip,0,32);
			InitSocket();
			m_sockfd=CreateSocket(SOCK_STREAM);
		}

		//ExTcpClient(char* clientip,int clientport);
		int connectTCP(char*serverip,int serverport);
		int recieve(char* buf,int len);
		int sendMsg(char*buf,int len);
		int close();
		int getSockfd()
		{
			return m_sockfd;
		}
	private:
		char m_clientip[32];
		char m_serverip[32];
		int  m_clientport;
		int  m_serverport;
		int  m_sockfd;
	};
}