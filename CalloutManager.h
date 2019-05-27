#pragma once
#include "base/output/include/xEtcpserver.h"
/*#include "base/output/include/xEMsgqueue.h"*/
#include "base/output/include/xthreadbase.h"
#include "common/DBOperator.h"
using namespace SEABASE;

class CallTask:public Threadbase
{
public:
	enum{
		 TASK_Init,
		 TASK_FINISH
	}	;
   CallTask(string taskid,string conpanyid,int rate=1.0):
		 task_id(taskid),company_id(conpanyid),taskStatus(TASK_Init),isStop(false),m_rate(rate)
	{
	}
   virtual void run();
   void GetCallList();
   bool isStop;
   string task_id;
   string company_id;
   float m_rate;
   string m_prefix;
   int taskStatus;
   vector<t_Outcallinfo> m_OutcallInfo;
};

class FreeswitchXmlCtl
{  
public:
	FreeswitchXmlCtl();
	void Creategata(string name,string realm,string proxy,string expirese,string username,string password,string strregister);
	void Setgataxml(string name,string param,string strvalue);
	bool Reloadfs();
	void CreateDNxml(string dnnumber);
	void SetDNpasswd(string dnnumber,string password);
	bool Reloadconf_directory();
	string m_fsxmlpath;
};
class CalloutManager:public xtcpserver
{
public:
	enum{
		MSG_CREATETASK,
		MSG_CHANGE_PARAM,
		MSG_FSCreategate,
		MSG_FSSetParam
	};
	virtual int Onaccept(int socketfd,char*data,int len,IN xReceivebackbase**clientHandle=NULL);
	virtual void Ondata(int socketfd,char*date,int len);
	virtual void Onclose(int socketfd);
	void Setfreeswitchxml();
	static CalloutManager* Instance();
	string OnMsg(char*data);
	vector<CallTask*> m_tasklist;
	void Deletetask(/*string taskid*/);
	string m_xmlpath;
};