//2018-12-13
/*
封装基本套接字接口，供其他文件使用 linux或者windows
1) 基本接口 socket bind listen accept connect send recv close shutdown peek
2）udp和tcp
3) 包含广播和多播
*/
#pragma once
#define IN
#define OUT
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#else
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> 
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#ifndef SOCKADDR 
#define SOCKADDR struct sockaddr
#endif
typedef int SOCKET;
#endif
namespace SEABASE
{


inline static int InitSocket()
{
#ifdef WIN32
	WORD wV=MAKEWORD(2,2);
	WSADATA wsadata;
	int err=WSAStartup(wV,&wsadata);
	return err;
#endif
}

//socket() 创建套接字。 
/*
type: SOCK_STREAM 或者 SOCK_DGRAM
*/
inline static int CreateSocket(int type,int af=AF_INET ,int protocal=0)
{
	int socket_=socket(af,type,protocal);
#ifdef WIN32
	int nBuf = 0;
	setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,(char *) &nBuf, sizeof(nBuf));
#endif
	return socket_;
}
//bind 套接字和指定地址绑定。
/*
socket:标识一个套接口的描述字。
addr: 指定地址。
struct socketaddr_in{
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
}
struct in_addr{
	uint32_t s_addr;
}
addrlen: 地址结构体长度。
成功返回0，失败返回-1。
*/
inline static int BindSocket(int socket,struct sockaddr*addr,socklen_t addrlen)
{
	//struct sockaddr 
	return bind(socket,addr,addrlen);
}
// setsockopt: 用于任意类型，任意接口的设置选项值。尽管再不同协议层上存在选项，但本函数仅定义了最高的
//				“套接口”层次上的选项。选项影响套接口的操作，如是否在普通数据流中接收，广播数据是否可以
//				从套接口接收
/*
socket:标识一个套接口的描述字。 
level: 现象定义的层次；目前只支持SOL_SOCKET 和 IPPOTO_TCP 。
option_name: 需要设置的选项。
option_value： 指针，指向存放选项值的缓冲区。
option_len： optval缓冲区的长度。
成功返回0.失败返回-1.
用法：
1. option_name=SO_DEBUG // 打开或者关闭调试信息。
	当option_value不等于0时，打开调试信息，否则关闭调试信息。
2. option_name= SO_REUSEADDR //打开或关闭地址复用功能。
	当option_value不等于0时，打开，否则，关闭。它实际所做的工作时置sock->sk->sk_reuse为1或者0。
原因：closesocket 接口在关闭套接字时一般不会立即关闭而是要经历TIME_WAIT 的过程。后想继续重用该socket，则设置SO_REUASEADDR。
用例：bool bReuseaddr=TRUE;
	 setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,(constchar*)&bReuseaddr,sizeof(bool))
3. option_name=SO_BROADCAST //允许或禁止发送广播数据。
	当option_value 不等于0时，允许，否则，禁止。实际所做的工作是在sock->sk->sk_flag中置或清SOCK_BROADCAST位。
4. option_name= SO_SNDBUF // 设置发送缓冲区大小。
	发送缓冲区的大小是有大小限制的，上限是 256*（sizeof(struct sk_buff)+256）,下限为2048字节。
	该操作将sock->sk->sk_sndbuf设置为val * 2，之所以要乘以2，是防止大量数据的发送，突然导致缓冲区溢出。
原因： 在send（）的时候，返回的是实际发送出去的字节（同步）或发送到socket缓冲区的字节（异步）；系统默认的状态发送和接收一次的8688
	  字节（约8.5k）。 在实际的过程中发送数据量和接收数据量比较大，可以设置socket缓冲区，而避免了send(),recv()不断的循环收发。
用例： int nSendBuf=32*1024 //32k
	  setsockopt(socket,SOL_SOCKET,SO_SNDBUF.(const char*)nSendBuf,sizeof(nSendBuf));
5. option_name=SO_RCVBUF //设置接收缓冲区的大小
	接收缓冲区大小的上下线分别是256*（sizeof(struct sk_buff)+256）和256字节。
6. option_name=SO_RCVTIMEO // 设置接收超时时间 超时未接收，则会返回0.
	该选项将发送超时时间赋给sock->sk->sk_sndtimeo。
原因：在send()或者recv() 过程中有时由于网络状况等原因，收发不能预期进行，而进行收发限时/
用例：int nNetTimeout=1000; //秒
	 setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO/SO_RCVTIMEO.(char*)&nNetTimeout,sizeof(int));
7. option_name=SO_ASNDTIMEO //设置发送超时时间
	该选项最终将发送超时时间赋给sock->sk->sk_sndtimeo。
8. option_name=SO_LINGER // 如果选择此项，close或shutdown将等到所有套接字里排队的消息成功发送或到达延时时间
	后 才会返回，否则，将立即返回。
	struct linger{
		int l_onoff;  // l_onoff 未0关闭，为1，开启。
		int l_linger;
	}
原因： 如果在发送数据的过程中去关闭（send()还没完成），而调用了closesocket()，以前我们一般采取
	  shutdown（是，SO_BOTH）,这样数据会丢失。设置SO_LINGER 可以然后数据发送完以后再关闭。
用例:  linger m_sling;
	  m_sling.l_onoff=1; (就算调用closesocket，如果数据没发送完成，也可以短暂逗留。)
	  m_sling.l_linger=5 // 容许逗留5秒
	  setsockopt(socket,SOL_SOCKET,SO_LINGER,(const char*)&m_sling,sizeof(linger));
//////////////////////////////////////////////////////////////////////////
udp多播 （getsockopt()/setsockopt()）
IP_MULTICAST_TTL		设置多播组数据的TTL值
IP_ADD_MEMBERSHIP		在指定接口上加入组播组
IP_DROP_MEMBERSHIP		退出组播组
IP_MULTICAST_IF			获取默认接口或设置接口
IP_MULTICAST_LOOP		禁止组播数据回送
*/
inline static int SetSockOpt( int socket, int level, int option_name,
	const char *option_value, size_t option_len)
{
	return setsockopt(socket,level,option_name,option_value,option_len);
}
inline static int AccpetSocket(int socket,struct sockaddr* addr,socklen_t*addrlen)
{
	return accept(socket,addr,addrlen);
}
//listen
/*
socket: 被listen的套接字。 listen会将该套截字设为被动套接字
backlog: 指定了在socket句柄上pending的连接队列可达到的最长长度。最大为128.
		一般小于30

成功返回0，失败返回-1
*/
inline static int ListenSocket(int socket,int backlog)
{
	return listen(socket,backlog);
}

//connect 主动连接服务端
/*
socket: 没有绑定的套接字。本地客户端套接字。
server_addr: 服务端地址
addrlen ： 服务端地址长度
成功返回0 ，失败返回-1.
*/
inline static int ConnectSocket(int socket,const struct sockaddr* server_addr,socklen_t addrlen)
{
	return connect(socket,server_addr,addrlen);
}
//send 发送数据
/*
socket: 对方socket
buf :   数据串
len :   数据长度
flags:  一般设置为0
成功返回0 ，失败返回-1
*/
inline static int SendSocket(int socket,char*buf,int len,int flags=0)
{
#ifdef WIN32
	return send(socket,buf,len,flags);
#else
	return write(socket,buf,len);
#endif
}
//recv  接收数据
/*
socket:指定接收端套接字描述符
buf 缓冲区用来存放接收到的数据
len 缓冲区长度
flags 一般设置为0
成功返回0，失败返回-1 （windows上失败返回错误码）
*/
inline static int ReadSocket(int socket,char* buf,int len ,int flags=0)
{
#ifdef WIN32
		int res= recv(socket,buf,len,flags);
		if(res==0)
			return 0;
		else if(res==SOCKET_ERROR)
		{
			int err=WSAGetLastError();
			return err;
		}
		return res;
#else
	return recv(socket,buf,len,flags);
#endif
}
//sendto 用于udp发送数据。不会阻塞
/*
sockfd: 代表你与远程程序连接的套接字描述符
msg:信息首地址
len:要发送信息的长度
flags:发送标记。一般都设为0。
to: 是指向struct sockaddr结构的指针，里面包含了远程主机的IP地址和端口数据。
tolen: to（ sizeof(struct sockaddr) ）的大小。
返回值： 正确返回0，错误返回-1.
*/
inline static int Sendto(int sockfd, const char *msg, int len, unsigned int flags,
	const struct sockaddr *to, int tolen)
{
	return sendto(sockfd,msg,len,flags,to,tolen);
}
//recvfrom : 用于udp的数据接收。会阻塞等待数据的到来
/*
sockfd: 需要读取数据的套接字描述符
buf: 存储数据的内存缓冲区
len: 缓存区的最大尺寸
flags: 是recv（）函数的一个标志，一般都是0.
from: 本地指针，指向一个struct sockaddr 的结构，(存有源IP和端口)
fromlen: sizeof(strucr sockaddr)
*/
inline static int Recvfrom(int sockfd, char *buf, int len, unsigned int flags,
struct sockaddr *from, int *fromlen)
{
#ifdef WIN32
	return recvfrom(sockfd,buf,len,flags,from,fromlen);
#else
	return recvfrom(sockfd,buf,len,flags,from,(socklen_t*)fromlen);
#endif
}
inline static int CloseSocket(int socket)
{
#ifdef WIN32
	return closesocket(socket);
#else
	return close(socket);
#endif
}
//shutdown close会把读写通道全部关闭，有时我们只希望关闭一个方向，这个时候我们
//		可以使用shutdown.
/*
socket: 需要关闭的套接字
howto: 0 这个时候系统会关闭读通道，但是可以继续往套接字里写。
	   1 这个时候关闭写通道。但是可以读。
	   2 关闭读写通道，和close一样了。在多线程程序里，如果有几个子进程共享一个套接字时，
	     如果我们使用shutdown，那么所有的子进程都不能操作了，这个时候我们只能够使用close来关闭
		 子进程的套接字。
成功返回0，失败返回-1.
*/
inline static int ShutDownSocket(int socket,int howto)
{
	return shutdown(socket,howto);
}


//与socket 相关的一些函数封装。

// namespace Network_function
// {
// 	//获取网络端口号
// 	/*
// 		lpServcie :
// 	*/
// 	unsigned int getPortNumber(const char* lpService,const char*lptransport)throw();
// 	//通过主机名称获取主机数据
// 	bool getHostByName(IN const char *host,OUT struct in_addr&addr)throw(){return true;}
// 	//通过网络地址获取主机信息
// 	std::string getHostByAddr(IN const struct in_addr& addr)throw(){return "";}
// 	//获取非阻塞IO的socket 错误号
// 	int getSocktAsyncError(IN int socket){return 0;}
// 	//设置socket是否阻塞。
// 	int setSocketBlock(IN int socket,IN bool boBlocking){return 0;}
// 
// 	//设置tcp 延时
// 	int setTCPDelay(IN int socket,bool boDelay=false){return 0;}
// 	//
// 	bool getPeerInfo(IN int socket,OUT struct sockaddr_in &addr);
// 	bool getPeerInfo(IN int socket,OUT char*ip,OUT int &port);
// 	bool getLocalInfo(IN int socket ,OUT struct sockaddr_in &addr );
// 	bool getLocalInfo(IN int socket,OUT char*ip,OUT int &port);
// 
// 
// }
// //



inline static unsigned int getPortNumber(const char* lpService,const char*lptransport)throw()
{
	return 0;
}
inline static bool getPeerInfo(IN int socket,OUT struct sockaddr_in &addr)
{
	if(socket!= INVALID_SOCKET)
	{
		socklen_t namelen=sizeof(addr);
		if(getpeername(socket,(struct sockaddr*)&addr,(socklen_t*)&namelen)==0)
		{
			return true;
		}
	}
	return false;
}
inline static bool getLocalInfo(IN int socket ,OUT struct sockaddr_in &addr )
{
	if(socket!=INVALID_SOCKET)
	{
		socklen_t namelen=sizeof(addr);
		if(getsockname(socket,(struct sockaddr*)&addr,(socklen_t*)&namelen)==0)
		{
			return true;
		}
	}
	return false;
}
inline static bool getPeerInfo(IN int socket,OUT char*ip,OUT int &port)
{
	struct sockaddr_in name;
	if(getPeerInfo(socket,name))
	{
		if(sizeof(ip)<sizeof(inet_ntoa(name.sin_addr)))
			return false;
		strcpy(ip,inet_ntoa(name.sin_addr));
		port=ntohs(name.sin_port);
		return true;
	}
	return false;
}
inline static bool getLocalInfo(IN int socket,OUT char*ip,OUT int &port)
{
	struct sockaddr_in name;
	if ( getLocalInfo(socket, name) )
	{
		if(sizeof(ip)<sizeof(inet_ntoa(name.sin_addr)))
			return false;
		strcpy(ip,inet_ntoa(name.sin_addr));
		port = ntohs(name.sin_port);
		return true;
	} 
	return false;
}

}