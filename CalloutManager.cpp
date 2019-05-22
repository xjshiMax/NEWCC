#include "CalloutManager.h"
#include "base/jsoncpp/json/json.h"
#include "DNManager.h"
 void CallTask::run()
 {
	 esl_handle_t handle = {{0}};
	 esl_status_t status;
	 char uuid[128];

	 string fsip,fspwd;
	 int fsport;
	 ManagerDN::Instance()->GetFSconfig(fsip,fspwd,fsport);
	 status = esl_connect(&handle, fsip.c_str(), fsport, NULL, fspwd.c_str());
	 GetCallList();
	 vector<t_Outcallinfo>::iterator ite=m_OutcallInfo.begin();
	  int num = ManagerDN::GetWaitingDN(atoi(company_id.c_str()));
	 while(ite!=m_OutcallInfo.end())
	 {
		// int num = ManagerDN::GetWaitingDN(atoi(company_id.c_str()));
		 if(num<=0)
		 {
			 //ÔÝÍ£
			 ManagerDN::Waitcalloutsema();
			 num = ManagerDN::GetWaitingDN(atoi(company_id.c_str()))*m_rate;
		 }else{
			 num--;
			 // ManagerDN::Outcall((ite)->company_id,(ite->task_id),(ite->phone));
 			 char callCmd[256]={0};
 			 sprintf(callCmd,"bgapi originate {ignore_early_media=true,company_id=%d,taskID=%s}sofia/gateway/reingw/%s%s &park()",(ite)->company_id,(ite->task_id).c_str(),ManagerDN::Instance()->GetPrefixnum().c_str(),(ite->phone).c_str());
 			 esl_send_recv(&handle,callCmd);   
			 ite++;
		 }
	 }
	  taskStatus = TASK_FINISH;
 }
void CallTask::GetCallList()
{
	db_operator_t::GetOutcalllist(m_OutcallInfo);
}

CalloutManager* CalloutManager::Instance()
{
	 static CalloutManager Manager;
	 return &Manager;
}
int CalloutManager::Onaccept(int socketfd,char*data,int len,IN xReceivebackbase**clientHandle)
{

}
void CalloutManager::Ondata(int socketfd,char*date,int len)
{
		Json::Value response;
		string strmsg = OnMsg(date);
		response["status"]=strmsg;
		string strrespom=response.toStyledString();
		SendSocket(socketfd,(char*)strrespom.c_str(),strrespom.length());
}
void CalloutManager::Onclose(int socketfd)
{

}

string CalloutManager::OnMsg(char*data)
{
	//{"cmd":"create","companyID":3,"taskID":10000}
	//{"cmd":"setparam","companyID":3,"taskID":10000,"surate":0.25}
	Deletetask();
	Json::Value	  root;
	Json::Reader reader;
	if(!reader.parse(data,root))
	{
		return "parse json failed";
	}
	
	if((root["cmd"].isNull()||!root["cmd"].isString()) ||(root["companyID"].isNull()||!root["companyID"].isString()) ||(root["taskID"].isNull()||!root["taskID"].isString()))
	{
		return "param type or num is less";
	}
	string strcmd=root["cmd"].asString();
	if(strcmd=="create")
	{
		string companyid = root["companyID"].asString();
		string taskid = root["taskID"].asString();
		CallTask* task = new CallTask(taskid,companyid);
		task->start();
		m_tasklist.push_back(task);
		return "create task successfully ";
	}
	else if(strcmd=="setparam")
	{

	}
	return "ok";
}

void CalloutManager::Deletetask(/*string taskid*/)
{
	 vector<CallTask*>::iterator ite = m_tasklist.begin();
	 while(ite!=m_tasklist.end()&&m_tasklist.size()!=0)
	 {
		 if((*ite)->taskStatus==CallTask::TASK_FINISH)
		 {
			 CallTask*ptask=*ite;
			 if(ptask)
				 delete ptask;
			 m_tasklist.erase(ite++);

			 continue;
		 }
		 ite++;
	 }
}