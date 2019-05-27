#pragma once
#include "CalloutManager.h"
#include "base/jsoncpp/json/json.h"
#include "base/pugixml/include/pugixml.hpp"
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
			 //暂停
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
FreeswitchXmlCtl::FreeswitchXmlCtl()
{
   m_fsxmlpath=ManagerDN::m_FSXMLPATH;
}
void FreeswitchXmlCtl::Creategata(string name,string realm,string proxy,string expirese,string username,string password,string strregister)
{
	pugi::xml_document xdoc;

	//pugi::xml_node decl=xdoc.prepend_child(pugi::node_declaration);
	pugi::xml_node xnode=xdoc.append_child("include");

	pugi::xml_node xgate = xnode.append_child("gateway");
	pugi::xml_attribute attname=xgate.append_attribute("name");
	attname.set_value(name.c_str());
	//xgata.append_attribute("name")=name;
	pugi::xml_node param1=xgate.append_child("param");
	pugi::xml_attribute name1=param1.append_attribute("name");
	name1.set_value("realm");
	pugi::xml_attribute value1=param1.append_attribute("value");
	value1.set_value(realm.c_str());

	pugi::xml_node param2=xgate.append_child("param");
	pugi::xml_attribute name2=param2.append_attribute("name");
	name2.set_value("proxy");
	pugi::xml_attribute value2=param2.append_attribute("value");
	value2.set_value(proxy.c_str());

	pugi::xml_node param3=xgate.append_child("param");
	pugi::xml_attribute name3=param3.append_attribute("name");
	name3.set_value("expire-seconds");
	pugi::xml_attribute value3=param3.append_attribute("value");
	value3.set_value(expirese.c_str());

	pugi::xml_node param4=xgate.append_child("param");
	pugi::xml_attribute name4=param4.append_attribute("name");
	name4.set_value("username");
	pugi::xml_attribute value4=param4.append_attribute("value");
	value4.set_value(username.c_str());

	pugi::xml_node param5=xgate.append_child("param");
	pugi::xml_attribute name5=param5.append_attribute("name");
	name5.set_value("password");
	pugi::xml_attribute value5=param5.append_attribute("value");
	value5.set_value(password.c_str());

	pugi::xml_node param6=xgate.append_child("param");
	pugi::xml_attribute name6=param6.append_attribute("name");
	name6.set_value("register");
	pugi::xml_attribute value6=param6.append_attribute("value");
	value6.set_value(strregister.c_str());
	xdoc.print(std::cout);
	string filename=m_fsxmlpath+name+".xml";
	xdoc.save_file(filename.c_str(),"\t",pugi::format_no_declaration);
	Reloadfs();

}
void FreeswitchXmlCtl::Setgataxml(string name,string param,string strvalue)
{
	 string fullpath=m_fsxmlpath+name+".xml";
	 pugi::xml_document xdoc;
	 pugi::xml_parse_result result = xdoc.load(fullpath.c_str();
	 if(!result.status)
	 {
		 if(param=="register")
		 {
			pugi::xml_node xnode = xdoc.child("include").child("register");
			pugi::xml_attribute attvalue=xnode.attribute("vaule");
			attvalue.set_value(strvalue.c_str());
		 }
		 else //网关下的参数
		 {
			  pugi::xml_node xnode = xdoc.child("include").child(name.c_str()).child(param.c_str());
			  pugi::xml_attribute attvalue=xnode.attribute("vaule");
			  attvalue.set_value(strvalue.c_str());
		 }
		 xdoc.save_file(fullpath.c_str());
	 }
	 Reloadfs();

}
bool FreeswitchXmlCtl::Reloadfs()
{
	esl_handle_t handle = {{0}};
	esl_status_t status;
	char uuid[128];

	string fsip,fspwd;
	int fsport;
	ManagerDN::Instance()->GetFSconfig(fsip,fspwd,fsport);
	status = esl_connect(&handle, fsip.c_str(), fsport, NULL, fspwd.c_str());
	//esl_execute(&handle, "reloadxml",NULL,NULL)	;
	status = esl_send_recv(&handle,"api sofia profile external restart reloadxml \nconsole_execute: true\n\n");
	printf("\n FreeswitchXmlCtl::Reloadfs statis=%d\n",status);
}
void FreeswitchXmlCtl::CreateDNxml(string dnnumber)
{
	pugi::xml_document xdoc;

	//pugi::xml_node decl=xdoc.prepend_child(pugi::node_declaration);
	pugi::xml_node xnode=xdoc.append_child("include");

	pugi::xml_node user = xnode.append_child("user");
	//user.attribute("id")=dnnumber;
	pugi::xml_node params=user.append_child("params");
	pugi::xml_node param1=params.append_child("param");

	pugi::xml_attribute attname=param1.append_attribute("name");
	attname.set_value("password");
	pugi::xml_attribute attvalue=param1.append_attribute("value");
	attvalue.set_value("$${default_password}");
	pugi::xml_node param2=params.append_child("param");
	pugi::xml_attribute attname2=param2.append_attribute("name");
	attname2.set_value("vm-password");
	pugi::xml_attribute attvalue2=param2.append_attribute("value");
	attvalue2.set_value(dnnumber.c_str());

	pugi::xml_node variables = user.append_child("variables");
	pugi::xml_node variable1=variables.append_child("variable");
	pugi::xml_attribute var_name1=variable1.append_attribute("name");
	var_name1.set_value("toll_allow");
	pugi::xml_attribute var_value1=variable1.append_attribute("value");
	var_value1.set_value("domestic,international,local");

	pugi::xml_node variable2=variables.append_child("variable");
	pugi::xml_attribute var_name2=variable2.append_attribute("name");
	var_name2.set_value("accountcode");
	pugi::xml_attribute var_value2=variable2.append_attribute("value");
	var_value2.set_value(dnnumber.c_str());

	pugi::xml_node variable3=variables.append_child("variable");
	pugi::xml_attribute var_name3=variable3.append_attribute("name");
	var_name3.set_value("user_context");
	pugi::xml_attribute var_value3=variable3.append_attribute("value");
	var_value3.set_value("default");

		pugi::xml_node variable4=variables.append_child("variable");
	pugi::xml_attribute var_name4=variable4.append_attribute("name");
	var_name4.set_value("effective_caller_id_name");
	pugi::xml_attribute var_value4=variable4.append_attribute("value");
	string temp="Extension "+dnnumber;
	var_value4.set_value(temp.c_str());

	pugi::xml_node variable5=variables.append_child("variable");
	pugi::xml_attribute var_name5=variable5.append_attribute("name");
	var_name5.set_value("effective_caller_id_number");
	pugi::xml_attribute var_value5=variable5.append_attribute("value");
	var_value5.set_value(dnnumber.c_str());

	pugi::xml_node variable6=variables.append_child("variable");
	pugi::xml_attribute var_name6=variable6.append_attribute("name");
	var_name6.set_value("outbound_caller_id_name");
	pugi::xml_attribute var_value6=variable6.append_attribute("value");
	var_value6.set_value("$${outbound_caller_name}") ;

		pugi::xml_node variable7=variables.append_child("variable");
	pugi::xml_attribute var_name7=variable7.append_attribute("name");
	var_name7.set_value("outbound_caller_id_number");
	pugi::xml_attribute var_value7=variable7.append_attribute("value");
	var_value7.set_value("$${outbound_caller_id}");

	pugi::xml_node variable8=variables.append_child("variable");
	pugi::xml_attribute var_name8=variable8.append_attribute("name");
	var_name8.set_value("callgroup");
	pugi::xml_attribute var_value8=variable8.append_attribute("value");
	var_value8.set_value("techsupport") ;


	xdoc.print(std::cout);
	string filename="/usr/local/freeswitch/conf/directory/default/"+dnnumber+".xml";
	xdoc.save_file(filename.c_str(),"\t",pugi::format_no_declaration);
	Reloadconf_directory();
}
void FreeswitchXmlCtl::SetDNpasswd(string dnnumber,string password)
{
	string fullpath="/usr/local/freeswitch/conf/directory/default/"+dnnumber+".xml";
	pugi::xml_document xdoc;
	 pugi::xml_parse_result result = xdoc.load_file(fullpath.c_str());
	if(!result.status)
	{
		
			pugi::xml_node xnode = xdoc.child("include").child("user").child("params").first_child();
			pugi::xml_attribute attvalue=xnode.attribute("value");
			attvalue.set_value(password.c_str());
			xdoc.save_file(fullpath.c_str());
	
	}
	Reloadconf_directory();
}
bool FreeswitchXmlCtl::Reloadconf_directory()
{
	esl_handle_t handle = {{0}};
	esl_status_t status;
	char uuid[128];

	string fsip,fspwd;
	int fsport;
	ManagerDN::Instance()->GetFSconfig(fsip,fspwd,fsport);
	status = esl_connect(&handle, fsip.c_str(), fsport, NULL, fspwd.c_str());
	//esl_execute(&handle, "reloadxml",NULL,NULL)	;
	status = esl_send_recv(&handle,"api reloadxml \nconsole_execute: true\n\n");
	printf("\n FreeswitchXmlCtl::Reloadconf_directory status=%d\n",status);
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
	
	if((root["cmd"].isNull()||!root["cmd"].isString()) )
	{
		return "param type or num is less";
	}
	string strcmd=root["cmd"].asString();
	if(strcmd=="CreateTask")
	{
		if((root["companyID"].isNull()||!root["companyID"].isString()) ||(root["taskID"].isNull()||!root["taskID"].isString()))
		{
			return "CreateTask lost param";
		}
		string companyid = root["companyID"].asString();
		string taskid = root["taskID"].asString();
		CallTask* task = new CallTask(taskid,companyid);
		task->start();
		m_tasklist.push_back(task);
		return "create task successfully ";
	}
	else if(strcmd=="SetCallrate")
	{
		if((root["companyID"].isNull()||!root["companyID"].isString()) ||(root["taskID"].isNull()||!root["taskID"].isString())||(root["rate"].isNull()||!root["rate"].isInt()))
		{
			return "SetCallrate lost param";
		}
		string taskid=root["taskID"].asString();
		 int rate=root["rate"].asInt();
		vector<CallTask*>::iterator ite=m_tasklist.begin();
		while(ite!=m_tasklist.end())
		{
			CallTask*ptask=*ite;
			if(ptask->task_id == taskid)
			{
				ptask->m_rate=rate;
			}
			ite++;
		}
		 // int rate=root["rate"].asInt();
	}
	else if(strcmd=="CreateFSxml")
	{
		if((root["name"].isNull()||!root["name"].isString()) ||(root["realm"].isNull()||!root["realm"].isString())||(root["proxy"].isNull()||!root["proxy"].isString())||
			(root["expirese"].isNull()||!root["expirese"].isString()) ||(root["username"].isNull()||!root["username"].isString())||(root["password"].isNull()||!root["password"].isString())||
			(root["register"].isNull()||!root["register"].isString()))
		{
			return "CreateFSxml lost param";
		}
		FreeswitchXmlCtl pfsctl;
		string name=root["name"].asString();
		string realm=root["realm"].asString();
		string proxy=root["proxy"].asString();
		string expirese=root["expirese"].asString();
		string username=root["username"].asString();
		string password=root["password"].asString();
		string strregister=root["register"].asString();
		pfsctl.Creategata(name,realm,proxy,expirese,username,password,strregister);
	}
	else if(strcmd=="SetFSxml")
	{
		if((root["name"].isNull()||!root["name"].isString()) ||(root["param"].isNull()||!root["param"].isString()) ||(root["value"].isNull()||!root["value"].isString()))
		{
			return "SetFSxml lost param";
		}
		string name=root["root"].asString();
		string param=root["param"].asString();
		string strvalue=root["value"].asString();
		FreeswitchXmlCtl pfsctl;
		pfsctl.Setgataxml(name,param,strvalue);
	}
	else if(strcmd=="CreateDNxml")
	{
		if((root["DNnumber"].isNull()||!root["DNnumber"].isString()))
		{
			return "SetFSxml lost param";
		}
		string DNnumber=root["DNnumber"].asString();
		FreeswitchXmlCtl pfsctl;
		pfsctl.CreateDNxml(DNnumber);
	}
	else if(strcmd=="SetDNpasswd")
	{
		if((root["DNnumber"].isNull()||!root["DNnumber"].isString())|| (root["password"].isNull()||!root["password"].isString()))
		{
			return "SetFSxml lost param";
		}
		string DNnumber=root["DNnumber"].asString();
		string passwd=root["password"].asString();
		FreeswitchXmlCtl pfsctl;
		pfsctl.SetDNpasswd(DNnumber,passwd);
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