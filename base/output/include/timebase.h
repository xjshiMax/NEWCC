//2018-11-29   创建一个线程安全的计时对象
#pragma once
#include <stdio.h>
#include <time.h>
#ifdef WIN32
	#include <Windows.h>
	//#define LONGLONG INT64
#else
	#define LONGLONG long long 
#endif
#define objMS 10000
#define objS  10000000
class timeobj
{
public:
	timeobj(char *pmsg="life:",int timeval=objMS):m_val(timeval),m_pmsg(pmsg)
	{
#ifdef WIN32
		GetSystemTimeAsFileTime((LPFILETIME)&m_starttime);
#endif
	}
	virtual ~timeobj()
	{
#ifdef WIN32
		GetSystemTimeAsFileTime((LPFILETIME)&m_endtime);
		m_lifeTime=m_endtime-m_starttime;
		if(m_pmsg!=NULL)
			printf("%s %f(ms)\n",m_pmsg,m_lifeTime*1.0/m_val);
		else
			printf("the life cost(100ns):%f\n",m_lifeTime*1.0/m_val);
#endif
	}
	LONGLONG startcount()
	{
#ifdef WIN32
		GetSystemTimeAsFileTime((LPFILETIME)&m_starttime);
		return m_starttime; 
#endif
	}
	LONGLONG utilstartcount()  //从调用start开始到待用utilstartcount 走过的ns数
	{
#ifdef WIN32
		GetSystemTimeAsFileTime((LPFILETIME)&m_endtime);
		return m_endtime-m_starttime;
#endif
	}
private:
	LONGLONG m_starttime;
	LONGLONG m_endtime;
	LONGLONG m_lifeTime;
	int m_val;
	char* m_pmsg;
};
