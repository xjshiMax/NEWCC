#include "DNManager.h"
#include "common/DBOperator.h"
#include "base/output/include/xTimeuil.h"
#include "base/output/include/xReactorwithThread.h"
#include "base/jsoncpp/json/json.h"
#include "base/inifile/inifile.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <pthread.h>
using namespace SEABASE;

void DNuser::inline_Setcompanyid(int companyid)
{
	m_commpanyid = companyid;
}
int  DNuser::inline_Getcompanyid()
{
		return m_commpanyid;
}
void DNuser::SetagnetStatus(int agentStatus)
{
	m_agentstatus = agentStatus;
	if(agentStatus==DN_Waiting_ready)
		ManagerDN::SetDNsemaSignal();
}
void DNuser::GetagnetStatus(int& agentStatus)
{
	agentStatus=m_agentstatus;
}
string DNuser::serialize()
{
	//Json::Writer writer;
	Json::Value root;
	root["commpanyid"]=m_commpanyid;
	root["DN"]=m_DN;
	root["agentid"]= m_agentid;
	root["reason"]= m_reason;
	root["agentstatus"]= m_agentstatus;
	root["agentPwd"]= m_agentPwd;
	root["autoAnswer"]= m_autoAnswer;
	root["fcSignin"]= m_fcSignin;
	root["skills"]= m_skills;
	root["peerIP"]= m_peerIP;
	return root.toStyledString();
}
void DNuser::Unserialize(string serizestr)
{
	Json::Reader read;
	Json::Value root;
	if(read.parse(serizestr,root))
	{
		m_commpanyid=root["commpanyid"].asInt();
		m_DN=root["DN"].asString();
		m_agentid=root["agentid"].asString();
		m_reason=root["reason"].asString();
		m_agentstatus=root["agentstatus"].asInt();
		m_agentPwd=root["agentPwd"].asString();
		m_autoAnswer=root["autoAnswer"].asBool();
		m_fcSignin=root["fcSignin"].asBool();
		m_skills=root["skills"].asString();
		m_peerIP=root["peerIP"].asString();
		m_s=NULL;
		//m_hdl=0;
	}
}
int DNuser::Getidelminiute()
{
	 int currenttime=time(NULL);
	 return currenttime - m_idelminiutes - m_callminiutes;
}
void SqlitePersist::LoadDNSet(map<string,DNuser>&dnset)
{
	string dbname="data.db";
	 if(!CreateDB(dbname))
		 return;
	 int nrow = 0;
	 int ncolumn = 0;
	 char*errmsg=NULL;
	 char ** azResult; //返回结果集
	 int iret = sqlite3_get_table(m_pdb , "select * from dnuser" , &azResult , &nrow , &ncolumn , &errmsg );	//age,sex,17,male,18,female, 结果集为每一行平铺的一维数组
	 if(iret==SQLITE_OK)
	 {
		  for(int i=1;i<=nrow;i++)
		  {
			  
			  string strdn=azResult[i*ncolumn+0];
			  string strserize=azResult[i*ncolumn+1];
			  if(strserize!="")
			  {
				  DNuser user;
				  user.Unserialize(strserize);
				  //重新加载的时候，将状态置为初始状态
				  user.m_agentstatus = DNuser::DN_Waiting_ready;
				  if(user.m_agentstatus==DNuser::DN_Waiting_ready)
					  ManagerDN::SetDNsemaSignal();
				  dnset[strdn]=user;
			  }			
		  }
	 }
	 else
	 {
		CreateTable();
	 }
	 
	 sqlite3_free_table(azResult);

}
bool SqlitePersist::CreateDB(string daname)
{
	int iret = sqlite3_open(daname.c_str(),&m_pdb);
	if(iret==SQLITE_OK)
		return true;
	return false;
}
bool SqlitePersist::CloseDB()
{
	int iret = sqlite3_close(m_pdb);
	return true;
}
bool SqlitePersist::CreateTable()
{
// 	if(!IsExistTable())
// 		return false;
	char*errmsg=NULL;
	int iret = sqlite3_exec(m_pdb,"create table dnuser(strdn varchar(255) ,strserize varchar(255))",NULL,NULL,&errmsg);
	if(iret==SQLITE_OK)
		return true;
	return false;
}
bool SqlitePersist::IsExistTable()
{
	std::string strFindTable = "SELECT COUNT(*) FROM 'dnuser' ";
	int nrow = 0;
	int ncolumn = 0;
	char ** azResult; //返回结果集
	char*errmsg=NULL;
	int iret= sqlite3_get_table(m_pdb , strFindTable.c_str() , &azResult , &nrow , &ncolumn , &errmsg );	//age,sex,17,male,18,female, 结果集为每一行平铺的一维数组
	if(iret==SQLITE_OK)
		return true;
	return false;
}
bool SqlitePersist::InsertTable(DNuser&user)
{
	string sqlstr = "Insert into  dnuser (strdn  ,strserize ) values";
	char charsql[1024]={0};
	sprintf(charsql,"%s('%s','%s') ",sqlstr.c_str(),user.m_DN.c_str(),user.serialize().c_str());
	char*errmsg=NULL;
	int iret = sqlite3_exec(m_pdb,charsql,NULL,NULL,&errmsg);
	if(iret==SQLITE_OK)
		return true;
	return false;
}
bool SqlitePersist::DeleteTable(DNuser&user)
{
	string sqlstr = "delete from  dnuser where strdn = ";
	char charsql[1024]={0};
	sprintf(charsql,"%s'%s'",sqlstr.c_str(),user.m_DN.c_str());
	char*errmsg=NULL;
	int iret = sqlite3_exec(m_pdb,charsql,NULL,NULL,&errmsg);
	if(iret==SQLITE_OK)
		return true;
	return false;
}
bool SqlitePersist::Changestate(DNuser&user)
{
	DeleteTable(user);
	return InsertTable(user);
}


map<string,ivrsession> ManagerDN::m_ivrmap;
xEmutex ManagerDN::m_agentlock;
xSemaphore ManagerDN::m_readyDNsema;
xSemaphore ManagerDN::m_calloutsema;
string ManagerDN::m_fsip;
int ManagerDN::m_fsport;
string ManagerDN::m_fspwd;

string ManagerDN::m_mysqlip;
int ManagerDN::m_mysqlport;
string ManagerDN::m_mysqluser;
string ManagerDN::m_mysqlpwd;

string ManagerDN::m_prefix;	//拨打电话前缀
int ManagerDN::m_tcpbussinessPort;
int ManagerDN::m_wsbussinessPort;
string ManagerDN::m_ServerIP;
string ManagerDN::m_FSXMLPATH;
int   ManagerDN::m_DNChooseRule=DNuser::DNRULE_select;
map<int,int> ManagerDN::m_compyidDNrulemap;
ManagerDN::ManagerDN()
{

}
ManagerDN::~ManagerDN()
{

}
ManagerDN* ManagerDN::Instance()
{
	static ManagerDN manager;
	return &manager;
}

bool ManagerDN::checkvaildDN(string dnid)
{
	vector<Route>::iterator iteroute =  m_gRoute.begin();
	while(iteroute!=m_gRoute.end())
	{
		if(iteroute->agent == dnid) //路由表中存在该dn
		{
			return  true;
		}
		iteroute++;
	}
	return false;
}
bool ManagerDN::checkPasswd(string dnid,string pwd)
{
	map<string,string>::iterator ite= m_agentloginInfo.find(dnid);
	if(ite!=m_agentloginInfo.end())
	{
		if(ite->second == pwd)
			return true;
	}
	return false;
}
int ManagerDN::Signin(const std::string& agentId,
	const std::string& agentDn, const std::string& agentPwd,
	const int & statusChangetype, bool autoAnswer,
	bool fcSignin, const std::string& skills,int department_id,const std::string department_name,string user_name,
	string peerIP,wsServer *s, websocketpp::connection_hdl hdl)
{
	map<string,DNuser>::iterator ite=m_DNmap.begin();
	if(checkvaildDN(agentDn))
	{
		ite=m_DNmap.find(agentDn);
		if(ite==m_DNmap.end()) //从未登陆过
		{

			DNuser user(agentId,agentDn,agentPwd,statusChangetype,autoAnswer,fcSignin,skills,peerIP,s,hdl,department_id,department_name,user_name);
			string commpanyid = GetcompanyidbyDN(agentDn);
			if(statusChangetype==DNuser::DN_Waiting_ready)
				SetDNsemaSignal();
			user.inline_Setcompanyid(atoi(commpanyid.c_str()));
 			m_DNmap.insert(pair<string ,DNuser>(agentDn,user));
			m_presisted.InsertTable(user);

			return DN_OPERATOR_SUCCESSED;
		}
		m_DNmap[agentDn].m_agentstatus = DNuser::DN_Waiting_ready;
		return DN_HAS_SIGNINED;
	}
	return DN_INVALIDDN;
}
int ManagerDN::Signout(const std::string& agentId,
	string peerIP)
{
// 	if(m_DNmap.find(agentId)==m_DNmap.end())
// 		return -1;
	string strDN=GetDNbyagentid(agentId);
	if(strDN!="")
	{
			m_presisted.DeleteTable(m_DNmap[strDN]);
			m_DNmap.erase(strDN);
			return DN_OPERATOR_SUCCESSED;
	}
		return DN_INVALIDDN;

}
int ManagerDN::SetDNstatus(const std::string& agentId,
	const int& agentStatus, const std::string& restReason,
	string peerIP)
{	 
	string strdn=GetDNbyagentid(agentId) ;
	if(strdn!="")
	{
		 DNuser&puser=m_DNmap[strdn];
		 puser.SetagnetStatus(agentStatus);
		 m_presisted.Changestate(puser);
		 return DN_OPERATOR_SUCCESSED;
	}
	return DN_INVALIDDN;
}
int ManagerDN::GetDNstatus(const std::string& agentId, int& agentStatus,
	string peerIP)
{
	string strdn=GetDNbyagentid(agentId) ;
	if(strdn!="")
	{
		DNuser&puser=m_DNmap[strdn];
		puser.GetagnetStatus(agentStatus);
		return DN_OPERATOR_SUCCESSED;
	}
	return DN_INVALIDDN;
}

int ManagerDN::reloaddb()
{
	m_agentRoute.clear();
	m_gRoute.clear();
	m_agentloginInfo.clear();

	db_operator_t::SelectRouteAgent(m_agentRoute, m_gRoute);
	db_operator_t::Getagentandpwd(m_agentloginInfo);
	db_operator_t::GetDNchooserule(m_compyidDNrulemap);
	Managerivr::Reload();
	return 0;
}
int ManagerDN::loaddb()
{
	db_operator_t::initDatabase();
	db_operator_t::SelectRouteAgent(m_agentRoute, m_gRoute);
	db_operator_t::Getagentandpwd(m_agentloginInfo);
	db_operator_t::GetDNchooserule(m_compyidDNrulemap);
	Managerivr::Init();
	m_presisted.LoadDNSet(m_DNmap);
	return 0;
}
int ManagerDN::loadConfigini()
{
	inifile::IniFile   file;
	if(!file.load("Service.ini"))
	{
		int iret=-1;
		m_fsip=file.getStringValue("FREESWITCH","IP",iret);
		if(iret!=0)
		{
			m_fsip="0.0.0.0";
		}
		m_fsport=file.getIntValue("FREESWITCH","PORT",iret);
		if(iret!=0)
		{
			m_fsport=8021;
		}
		m_fspwd=file.getStringValue("FREESWITCH","PSSWD",iret);
		if(iret!=0)
		{
			m_fspwd="tx@infosun";
		}
		m_FSXMLPATH=file.getStringValue("FREESWITCH","XMLPATH",iret);
		if(iret!=0)
		{
			m_FSXMLPATH="/usr/local/freeswitch/conf/sip_profiles/external/";
		}
		m_prefix=file.getStringValue("TXBETA","prefix",iret);
		if(iret!=0)
		{
			m_prefix="88";
		}
		m_ServerIP=file.getStringValue("TXBETA","serverip",iret);
		if(iret!=0)
		{
			m_prefix="88";
		}
		m_wsbussinessPort=file.getIntValue("TXBETA","wsport",iret);
		if(iret!=0)
		{
			m_wsbussinessPort=10081;
		}
		m_tcpbussinessPort=file.getIntValue("TXBETA","outcalltcpport",iret);
		if(iret!=0)
		{
			m_tcpbussinessPort=10090;
		}

	}
}
int ManagerDN::startServer()
{
	int ret = 0;
	pthread_t pthid1, pthid2, pthid3, pthid4, pthid5;

	ret = pthread_create(&pthid1, NULL, Inbound_Init, NULL);
	if (ret) // 非0则创建失败
	{
		perror("createthread 1 failed.\n");
		return 1;
	}
	ret = pthread_create(&pthid2, NULL, Heatbeat_Process, NULL);
	if (ret) // 非0则创建失败
	{
		perror("createthread 3 failed.\n");
		return 1;
	}
	//esl_mutex_create(&m_gMutex);
	ret = pthread_create(&pthid2, NULL, listenthread_Process, NULL);
	if (ret) // 非0则创建失败
	{
		perror("createthread 3 failed.\n");
		return 1;
	}
	m_acdqueue.start();

	m_Outcallmanager = CalloutManager::Instance();
	int listenfd = m_Outcallmanager->startTcpSvr(m_ServerIP.c_str(),m_tcpbussinessPort);
	xReactorwithThread Reactor;
	Reactor.RegisterHandler(m_Outcallmanager,listenfd);
	Reactor.startReactorWithThread();
	m_apserver.InitApServer(m_ServerIP.c_str(),m_wsbussinessPort,10);
	m_apserver.startServer();
// 	ManagerDN* pmanager=ManagerDN::Instance();
// 	pmanager->loaddb();

	printf("cccc\n");

//	esl_mutex_destroy(&gMutex);
	pthread_join(pthid1, NULL);
	pthread_join(pthid2, NULL);
	pthread_join(pthid3, NULL);
	pthread_join(pthid4, NULL);

	return 0;
}
string ManagerDN::GetskillIDfromcaller(string callernum)
{
	map<string,DNuser>::iterator ite=m_DNmap.find(callernum);
	if(ite!=m_DNmap.end())
	{
		return ite->second.m_skills;
	}
	return "";
}
string getcurrenttime()
{
	struct tm *tblock;
	time_t timer = time(NULL);
	char strtime[64] = {0};
	tblock = localtime(&timer);
	sprintf(strtime, "%04d-%02d-%02d_%02d:%02d:%02d", tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	return strtime;
}

string getGroupId(const string &agent, const vector<Route> &vRoute)
{
	for (size_t i = 0; i < vRoute.size(); i++)
	{
		if (vRoute.at(i).agent == agent)
		{
			return vRoute.at(i).group_id;
		}
	}
	return "";
}

string getGroupNumber(const string &agent, const vector<Route> &vRoute)
{
	for (size_t i = 0; i < vRoute.size(); i++)
	{
		if (vRoute.at(i).agent == agent)
		{
			return vRoute.at(i).group;
		}
	}
	return "";
}
map<string, callout_info_t> ManagerDN::callInfoMap;
vector<Route> ManagerDN::m_gRoute;
map<string, vector<agent_t> > ManagerDN::m_agentRoute;
map<string, reg_info_t> ManagerDN::regInfoMap;

void ManagerDN::process_event(esl_handle_t *handle,
	esl_event_t *event,
	map<string, reg_info_t> &regInfoMap,
	const vector<Route> &vRoute)
{
	string caller_id, agentId;
	string destination_number;
	char tmp_cmd[1024] = {0};
	string strUUID;

	strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
	caller_id = esl_event_get_header(event, "Caller-Caller-ID-Number") ? esl_event_get_header(event, "Caller-Caller-ID-Number") : "";
	destination_number = esl_event_get_header(event, "Caller-Destination-Number") ? esl_event_get_header(event, "Caller-Destination-Number") : "";
	if(caller_id=="1005"||caller_id=="1006")
		esl_log(ESL_LOG_INFO, "caller_id :%s ---->event_id:%d\n", caller_id.c_str(),event->event_id);
	string event_subclass, contact, from_user;
	switch (event->event_id)
	{
	case ESL_EVENT_DTMF:
		{
			string dtmf = esl_event_get_header(event, "DTMF-Digit") ? esl_event_get_header(event, "DTMF-Digit") : "";
			//uuid = esl_event_get_header(event, "Caller-Unique-ID");
			strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
			//a_uuid = esl_event_get_header(event, "variable_a_leg_uuid");
			destination_number = esl_event_get_header(event, "Caller-Destination-Number");
			string is_callout, a_leg_uuid;
			is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; // ����Ϊ1�������������?
			const char *eventbody = esl_event_get_body(event);
			printf("body:\n%s\n", eventbody);
			esl_log(ESL_LOG_INFO, "dtmf :%s\n", dtmf.c_str());
			printf("ESL_EVENT_DTMF:inbound dtmf :%s\n", dtmf.c_str());
            map<string,ivrsession>::iterator sessionite = m_ivrmap.find(strUUID);
            if(sessionite!=m_ivrmap.end())
            {
                ivrsession&psession=m_ivrmap[strUUID];
                t_ivrnode*pnode = Managerivr::Instance()->Getnodeinfo(psession.m_companyid,psession.m_currentnodeid,dtmf);
                if(pnode)
                {
					m_ivrmap[strUUID].m_currentnodeid = pnode->node_id;
					if(pnode->recordfile==IVR_AGNET_MSG) //转人工
						TransformAgent(handle,strUUID,psession);
					else
						PlayBack(handle,pnode->recordfile,psession.m_uuid);
                }
            }
			break;
		}
	case ESL_EVENT_CHANNEL_ORIGINATE:
		{
			//添加预测式外呼功能
			//string strtaskID=esl_event_get_header(event, "Variable_taskID") ? esl_event_get_header(event, "Variable_taskID") : "";

			string is_callout;
			strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
			string sipcallid = esl_event_get_header(event, "variable_sip_call_id") ? esl_event_get_header(event, "variable_sip_call_id") : "";
			destination_number = esl_event_get_header(event, "Caller-Destination-Number");
			string createTime = esl_event_get_header(event, "Caller-Channel-Created-Time") ? esl_event_get_header(event, "Caller-Channel-Created-Time") : "";

			esl_log(ESL_LOG_INFO, "ESL_EVENT_CHANNEL_ORIGINATE :%s\n", strUUID.c_str());
			if(caller_id=="1005"||caller_id=="1006")
				printf("ddd");
			esl_log(ESL_LOG_INFO, "destination_number:caller_id  %s,%s\n",
				destination_number.c_str(), caller_id.c_str());
			if(Instance()->m_DNmap.find(caller_id)==Instance()->m_DNmap.end())  // 非法坐席或者无关者
				return;
			callout_info_t callinfo;
			callinfo.N_skill_id = Instance()->GetskillIDfromcaller(caller_id);
			if(callinfo.N_skill_id=="") //找不到caller 则呼叫类型设置为呼入
				callinfo.N_call_type="1";
			else
				callinfo.N_call_type="2";
			callinfo.N_call_start_time = xTimeUtil::format_time("%Y-%m-%d %H:%M:%S");
			callinfo.N_user_number = caller_id;
			callinfo.N_customer_number = destination_number;
			callinfo.N_call_id = sipcallid;
			callinfo.N_callstate= 1;
			callinfo.N_company_id = atoi(getGroupId(caller_id, vRoute).c_str());
			//callinfo.gateway_url = "1";
			if (callinfo.N_company_id == 3)
			{
				callinfo.N_call_prefix = "89";
			}
			else
			{
				callinfo.N_call_prefix = "83";
			}

			callinfo.N_extension_number = getGroupNumber(caller_id, vRoute);

			callinfo.record_file = strUUID + getcurrenttime() + ".mp4";
			callinfo.N_record_file=strUUID + getcurrenttime() + ".mp4";
			callInfoMap[strUUID] = callinfo;

			//坐席呼入功能，进行桥接
			string cmd="sofia/gateway";
			char charcmd[128]={0};
			string prefix=m_gRoute[callinfo.N_company_id].prefix;
			string gatename=m_gRoute[callinfo.N_company_id].gataname;
			sprintf(charcmd,"%s/%s/%s%s",cmd.c_str(),gatename.c_str(),prefix.c_str(),destination_number.c_str());
			esl_status_t iret = esl_execute(handle, "bridge",charcmd,strUUID.c_str());
			break;
		}
	case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
		{
			esl_log(ESL_LOG_DEBUG, "ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:inbound EXECUTE_COMPLETE :%s\n", strUUID.c_str());
			const char *application = esl_event_get_header(event, "Application");

			break;
		}
	case ESL_EVENT_CHANNEL_HANGUP:
		{
			string is_callout;
			is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; // ����Ϊ1�������������?
			string bridged_uuid;
			string hangup_cause;
			hangup_cause = esl_event_get_header(event, "variable_sip_term_cause") ? esl_event_get_header(event, "variable_sip_term_cause") : "";

			string hangupTime = esl_event_get_header(event, "Caller-Channel-Hangup-Time") ? esl_event_get_header(event, "Caller-Channel-Hangup-Time") : "";

			map<string, callout_info_t>::iterator iter = callInfoMap.find(strUUID);
			if (iter != callInfoMap.end())
			{
				callout_info_t& info = iter->second;
				info.callout_end_time = time((time_t *)NULL);
				info.N_call_end_time =  xTimeUtil::format_time("%Y-%m-%d %H:%M:%S");
			}
			break;
		}
	case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
		{
			strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
			string billsec = esl_event_get_header(event, "variable_billsec") ? esl_event_get_header(event, "variable_billsec") : "";
			esl_log(ESL_LOG_INFO, "CALL OUT HANGUP_COMPLETE :%s,%s\n", strUUID.c_str(),billsec.c_str());
			map<string, callout_info_t>::iterator iter = callInfoMap.find(strUUID);
			if (iter != callInfoMap.end())
			{
					callout_info_t& info = iter->second;
					info.N_call_duration=atoi(billsec.c_str());
			}
			break;
		}
	case ESL_EVENT_CHANNEL_BRIDGE:
		{
			string direct = esl_event_get_header(event, "call-direction") ? esl_event_get_header(event, "call-direction") : "";
			string caller_username = esl_event_get_header(event, "Caller-Channel-Name") ? esl_event_get_header(event, "Caller-Channel-Name") : "";

			esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_BRIDGE :%s ,%s,%s\n", strUUID.c_str(), direct.c_str(), caller_username.c_str());

			break;
		}
	case ESL_EVENT_CHANNEL_DESTROY:
		{
			map<string, callout_info_t>::iterator iter = callInfoMap.find(strUUID);
			if (iter != callInfoMap.end())
			{
				esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_DESTROY :%s\n", strUUID.c_str());
				callout_info_t info = iter->second;
				callInfoMap.erase(strUUID);
				info.callout_end_time = time((time_t *)NULL);
				info.N_call_end_time =  xTimeUtil::format_time("%Y-%m-%d %H:%M:%S");
				info.N_create_at = xTimeUtil::format_time("%Y-%m-%d %H:%M:%S");
				db_operator_t::insertCallInfoSql(info);
			}
			map<string,DNuser>::iterator sessionite= Instance()->m_DNmap.find(destination_number);
			string billsec = esl_event_get_header(event, "variable_duration") ? esl_event_get_header(event, "variable_duration") : "";
			if(sessionite!=Instance()->m_DNmap.end())
			{
				sessionite->second.SetagnetStatus(DNuser::DN_Waiting_ready);
				SetDNsemaSignal();
				Setcalloutsemafree();

				sessionite->second.m_callminiutes+=	atoi(billsec.c_str());
				sessionite->second.m_calltimes++;
			}
			break;
		}
	case ESL_EVENT_CHANNEL_ANSWER:
		{
			//添加预测式外呼功能
			string strtaskID=esl_event_get_header(event, "Variable_taskID") ? esl_event_get_header(event, "Variable_taskID") : "";
			if(strtaskID!="")
			{
				string companyid=esl_event_get_header(event, "Variable_company_id") ? esl_event_get_header(event, "Variable_company_id") : "";
				CalloutManager* Outcallmanager=CalloutManager::Instance();
				//Outcallmanager.

				ivrsession session;
				session.m_uuid=  strUUID;
				session.m_companyid=atoi(companyid.c_str());
				session.m_currentnodeid=1;
				m_ivrmap[strUUID] = session;
				//esl_execute(&handle, "answer", NULL, NULL);

				t_ivrnode*pnode = Managerivr::Instance()->Getnodeinfo(session.m_companyid,1,IVR_ANS_DFTM);
				if(pnode)
				{
					m_ivrmap[caller_id].m_currentnodeid = pnode->node_id;
					if(pnode->recordfile==IVR_AGNET_MSG) //转人工
						TransformAgent(handle,session.m_uuid,session);
					else
						PlayBack(handle,pnode->recordfile,session.m_uuid);
				}

			}
			map<string, callout_info_t>::iterator iter = callInfoMap.find(strUUID);
			esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_ANSWER :%s\n", strUUID.c_str());
			if (iter != callInfoMap.end())
			{
				iter->second.N_callstate = "1";
				esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_ANSWER  input:%s %d\n", strUUID.c_str(), callInfoMap.size());

				char tmp_cmd[300];
				string filePath;
				filePath = "/home/records/" + iter->second.record_file;
				//sprintf(tmp_cmd, "api uuid_record %s start %s 9999 \n\n", strUUID.c_str(), filePath.c_str());
				// sprintf(tmp_cmd, "api uuid_record %s start %s 9999 \n\n", caller_uuid.c_str(), strFullname.c_str());

				//esl_send_recv_timed(handle, tmp_cmd, 1000);
			}
			map<string,DNuser>::iterator sessionite= Instance()->m_DNmap.find(destination_number);
			if(sessionite!=Instance()->m_DNmap.end())
			{
				sessionite->second.SetagnetStatus(DNuser::DN_Talking);
			}
			break;
		}
	case ESL_EVENT_PLAYBACK_START:
		{

			string is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; 

			{
				esl_log(ESL_LOG_INFO, "CALL IN ESL_EVENT_PLAYBACK_START %s\n", strUUID.c_str());
			}
			break;
		}
	case ESL_EVENT_PLAYBACK_STOP:
		{
			destination_number = esl_event_get_header(event, "Caller-Destination-Number");
			strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
			string is_callout, a_leg_uuid;
			is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : "";
			{
				esl_log(ESL_LOG_INFO, "CALL IN ESL_EVENT_PLAYBACK_STOP %s\n", strUUID.c_str());
			}
			// esl_execute(handle, "hangup", NULL, strUUID.c_str());

			break;
		}

	}
}


/*static esl_mutex_t *gMutex = NULL;*/

agent_t *ManagerDN::find_available_agent(string callInNumber)
{
	//esl_mutex_lock(gMutex);
	int last_agent_index;
	map<string, vector<agent_t> >::iterator iter;
	iter = m_agentRoute.find(callInNumber);
	int last;
	int maxAgent = 0;
	if (iter != m_agentRoute.end())
	{
		maxAgent = (iter->second).size();
		last_agent_index = maxAgent;
		last = last_agent_index;
	}

	agent_t *agent = NULL;

	while (true)
	{
		if (last_agent_index >= maxAgent - 1)
		{
			last_agent_index = 0;
		}
		else
		{
			last_agent_index++;
		}

		agent = &((iter->second).at(last_agent_index));

		if (agent == NULL)
		{
			//esl_mutex_unlock(gMutex);

			return agent;
		}

		esl_log(ESL_LOG_INFO, "agent info [%d,%s,%s]\n", last_agent_index, agent->exten, agent->state == AGENT_IDLE ? "IDLE" : "IDLE");
		return agent;
		map<string, reg_info_t>::iterator itReg;
		itReg = regInfoMap.find(agent->exten);

		// if (itReg == regInfoMap.end())
		// {
		//     continue;
		// }

		if (agent->state == AGENT_IDLE)
		{
			agent->state = AGENT_BUSY;
			//esl_mutex_unlock(gMutex);
			return agent;
		}
		else if (agent->state == AGENT_BUSY)
		{

			//esl_mutex_unlock(gMutex);
			continue;
		}

		if (last_agent_index == last)
		{
			break;
		}
	}
	//esl_mutex_unlock(gMutex);
	return agent;
}
string ManagerDN::GetavailableAgent(int companyid)
{
	if(m_compyidDNrulemap.find(companyid)!=m_compyidDNrulemap.end())
		m_DNChooseRule=	m_compyidDNrulemap[companyid];
	switch(m_DNChooseRule)
	{
	case DNuser::DNRULE_ring_all:
		return GetAgent_robin(companyid);	
	case DNuser::DNRULE_long_idel_agent:
		return GetAgent_idel_agent(companyid);
	case DNuser::DNRULE_select:
		return 	GetAgent_robin(companyid);
	case DNuser::DNRULE_top_down:
		return 	GetAgent_robin(companyid);
	case DNuser::DNRULE_agent_with_least_talk_time:
		return GetAgent_least_talk_time(companyid);
	case DNuser::DNRULE_agent_with_fewest_calls:
		return GetAgent_fewest_calls(companyid);
	case DNuser::DNRULE_sequentially_by_agent_order:
		return GetAgent_robin(companyid);
	}
	return "";
}
//static string GetAgent_ringall();
string ManagerDN::GetAgent_idel_agent(int companyid) //选择空闲时间最长的
{
	bool isFind=false;
	int maxidelminiute=0;
	string dn;
	while(1)
	{
		map<string,DNuser>::iterator ite = Instance()->m_DNmap.begin();
		while (ite!=Instance()->m_DNmap.end())
		{
			if(companyid == ite->second.m_commpanyid)
			{
				if( ite->second.m_agentstatus == DNuser::DN_Waiting_ready)
				{
					isFind=true;
					if(maxidelminiute<ite->second.Getidelminiute())
					{
						maxidelminiute = ite->second.Getidelminiute();
						dn=ite->first;
					}
				}
			}
			ite++;
		}
		if(isFind)
			return dn;
		m_readyDNsema.wait();
	}
}
string ManagerDN::GetAgent_robin(int companyid)
{
	while(1)
	{
		map<string,DNuser>::iterator ite = Instance()->m_DNmap.begin();
		while (ite!=Instance()->m_DNmap.end())
		{
			if(companyid == ite->second.m_commpanyid)
			{
				if( ite->second.m_agentstatus == DNuser::DN_Waiting_ready)
				{
					//ite->second.SetagnetStatus(DNuser::DN_Ringing);
					return ite->first;
				}
			}
			ite++;
		}
		m_readyDNsema.wait();
	}

	return ""; // 找不到空闲坐席
	// m_DNmap
}
//static string GetAgent_Top_down();	//固定的顺序选择
string ManagerDN::GetAgent_least_talk_time(int companyid)//选择通话时间最短的坐席
{
	bool isFind=false;
	int maxidelminiute=999999;
	string dn;
	while(1)
	{
		map<string,DNuser>::iterator ite = Instance()->m_DNmap.begin();
		while (ite!=Instance()->m_DNmap.end())
		{
			if(companyid == ite->second.m_commpanyid)
			{
				if( ite->second.m_agentstatus == DNuser::DN_Waiting_ready)
				{
					isFind=true;
					if(maxidelminiute>ite->second.m_callminiutes)
					{
						maxidelminiute = ite->second.m_idelminiutes	;
						dn=ite->first;
					}
				}
			}
			ite++;
		}
		if(isFind)
			return dn;
		m_readyDNsema.wait();
	}
	return "";
}
string ManagerDN::GetAgent_fewest_calls(int companyid)	 //选择通话次数最少的坐席
{
	bool isFind=false;
	int maxidelminiute=999999;
	string dn;
	while(1)
	{
		map<string,DNuser>::iterator ite = Instance()->m_DNmap.begin();
		while (ite!=Instance()->m_DNmap.end())
		{
			if(companyid == ite->second.m_commpanyid)
			{
				if( ite->second.m_agentstatus == DNuser::DN_Waiting_ready)
				{
					isFind=true;
					if(maxidelminiute>ite->second.m_calltimes)
					{
						maxidelminiute = ite->second.m_idelminiutes	;
						dn=ite->first;
					}
				}
			}
			ite++;
		}
		if(isFind)
			return dn;
		m_readyDNsema.wait();
	}
	return "";
}
//static string GetAgent_agent_order();	 //根据梯队和顺序选择
string ManagerDN::GetcompanyidbyDN(string strDN)
{
	vector<Route>::iterator ite= m_gRoute.begin();
	while(ite!=m_gRoute.end())
	{
		if(ite->agent == strDN)
			return ite->group_id;
		ite++;
	}
	return "";
}
string ManagerDN::GetDNbyagentid(string strid)
{
	map<string,DNuser>::iterator ite= Instance()->m_DNmap.begin();
	while(ite!=Instance()->m_DNmap.end())
	{
		if(ite->second.m_agentid==strid)
			return ite->second.m_DN;
		ite++;
	}
	return "";
}
void *ManagerDN::Inbound_Init(void *arg)
{

	esl_handle_t handle = {{0}};
	esl_status_t status;
	const char *uuid;

	esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

	// status = esl_connect(&handle, "210.21.48.69", 8021, NULL, "tx@infosun");
	status = esl_connect(&handle, m_fsip.c_str(), m_fsport, NULL, m_fspwd.c_str());

	if (status != ESL_SUCCESS)
	{
		esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
		exit(1);
	}
	esl_log(ESL_LOG_INFO, "Connected to FreeSWITCH Inbound_Init\n");
	esl_events(&handle, ESL_EVENT_TYPE_PLAIN,
		"DETECTED_SPEECH RECORD_START RECORD_STOP PLAYBACK_START PLAYBACK_STOP CHANNEL_OUTGOING CHANNEL_PARK CHANNEL_EXECUTE_COMPLETE CHANNEL_ORIGINATE TALK NOTALK PHONE_FEATURE CHANNEL_HANGUP_COMPLETE CHANNEL_CREATE CHANNEL_BRIDGE DTMF CHANNEL_DESTROY CHANNEL_HANGUP CHANNEL_BRIDGE CHANNEL_ANSWER CUSTOM sofia::register sofia::unregister asr");
	esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

	handle.event_lock = 1;
	while ((status = esl_recv_event(&handle, 1, NULL)) == ESL_SUCCESS)
	{
		if (handle.last_ievent)
		{

			{
				process_event(&handle, handle.last_ievent, regInfoMap, m_gRoute);
			}
		}
	}

	esl_disconnect(&handle);

	return (void *)0;
}

//外呼处理
void *ManagerDN::CallOut_Task_Process(void *arg)
{

	esl_handle_t handle = {{0}};
	esl_status_t status;
	char uuid[128]; //从fs中获得的uuid
	//Then running the Call_Task string when added a new Task,then remove it

	esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

	status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "tx@infosun");

	if (status != ESL_SUCCESS)
	{
		esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
		exit(1);
	}

	//为了cps保证30以内，需要每路延时30ms
#ifdef WIN32
	Sleep(30);
#else
	//usleep(3000);
	struct timeval tempval;
	tempval.tv_sec = 0;
	tempval.tv_usec = 30;
	select(0, NULL, NULL, NULL, &tempval);
#endif

	esl_disconnect(&handle);

	return (void *)0;
}

void ManagerDN::nwaycc_callback(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *userData)
{
    esl_log(ESL_LOG_INFO, "nwaycc_callback: \n");
	esl_handle_t *handle=new esl_handle_t;
	string callid;

	bzero(handle, sizeof(esl_handle_t));

	if (ESL_SUCCESS != esl_attach_handle(handle, client_sock, addr)) {
		esl_disconnect(handle);
		if(handle!=NULL)
			delete handle;
		return;
	}
// 	esl_events(&handle, ESL_EVENT_TYPE_PLAIN, " CHANNEL_STATE CHANNEL_CALLSTATE CHANNEL_PROGRESS_MEDIA CHANNEL_OUTGOING SESSION_HEARTBEAT  CHANNEL_ORIGINATE CHANNEL_PROGRESS  "
// 		"   CHANNEL_EXECUTE CHANNEL_EXECUTE_COMPLETE   "
// 		"TALK OUTBOUND_CHAN INBOUND_CHAN  ");


	if (!handle->connected) {
		esl_disconnect(handle);
		if(handle!=NULL)
			delete handle;
		return;
	}
	 esl_log(ESL_LOG_INFO, "nwaycc_callback: 1046\n");
	esl_recv_event_timed(handle,1000, 1, NULL);
	std::string caller_number = esl_event_get_header(handle->info_event, "Caller-Caller-ID-Number") ? esl_event_get_header(handle->info_event, "Caller-Caller-ID-Number") : "";
	//esl_log(ESL_LOG_INFO, "nwaycc_callback:caller_number=%s \n",caller_number.c_str());
    std::string called_number = esl_event_get_header(handle->info_event, "Caller-Destination-Number") ? esl_event_get_header(handle->info_event, "Caller-Destination-Number") : "";
    esl_log(ESL_LOG_INFO, "nwaycc_callback:caller_number=%s,called_number=%s \n",caller_number.c_str(),called_number.c_str());
	esl_log(ESL_LOG_INFO, "nwaycc_callback: 1052\n");
    string companyid = GetCompanyIdFromgate(called_number);
    if(companyid=="")   //说明不属于我们配置的网关
	{
		esl_disconnect(handle);
		if(handle!=NULL)
			delete handle;
		handle=NULL;
        return;
	}
	esl_log(ESL_LOG_INFO, "nwaycc_callback: 1061\n");
	esl_execute(handle, "answer", NULL, NULL);
	esl_execute(handle, "park", NULL, NULL);
	char*fs_resp = esl_event_get_header(handle->last_sr_event, "Reply-Text");
	callid = esl_event_get_header(handle->info_event, "caller-unique-id");
    ivrsession session;
    session.m_uuid=  callid;
    session.m_companyid=atoi(companyid.c_str());
    session.m_currentnodeid=1;
	session.m_newhandle=handle;
    m_ivrmap[callid] = session;
	//esl_execute(&handle, "answer", NULL, NULL);

     t_ivrnode*pnode = Managerivr::Instance()->Getnodeinfo(session.m_companyid,1,IVR_ANS_DFTM);
    if(pnode)
 	{
		m_ivrmap[callid].m_currentnodeid = pnode->node_id;
 		if(pnode->recordfile==IVR_AGNET_MSG) //转人工
 		{
			TransformAgent(handle,session.m_uuid,session);
			return;
		}
 		else
		{
 			PlayBack(handle,pnode->recordfile,session.m_uuid);
			esl_disconnect(handle);
			if(handle!=NULL)
				delete handle;
			handle=NULL;
			return;
		}
 	}
	//esl_execute(&handle, "bridge", "user/1005", NULL);
	//esl_disconnect(&handle);
	esl_log(ESL_LOG_INFO, "nwaycc_callback: 1093\n");
	esl_disconnect(handle);
	if(	handle!=NULL)
		delete handle;
	handle=NULL;
	return;	

}
void *ManagerDN::listenthread_Process(void *arg)
{
	esl_log(ESL_LOG_INFO, "ACD Server listening at localhost :8040....\n");
	esl_listen_threaded("127.0.0.1", 8040, nwaycc_callback, NULL, 100000);
}

void *ManagerDN::Heatbeat_Process(void *arg)
{
	esl_handle_t handle = {{0}};
	esl_status_t status;
	char uuid[128]; //从fs中获得的uuid
	//Then running the Call_Task string when added a new Task,then remove it

	status = esl_connect(&handle, m_fsip.c_str(),m_fsport, NULL, m_fspwd.c_str());
	esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

	if (status != ESL_SUCCESS)
	{
		esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
		exit(1);
	}

	while (true)
	{
		esl_send_recv(&handle, "api create_uuid\n\n");

		if (handle.last_sr_event && handle.last_sr_event->body)
		{
			esl_log(ESL_LOG_INFO, "headbeat uuid:[%s]\n", handle.last_sr_event->body);
		}
		else
		{
			esl_log(ESL_LOG_INFO, "headbeat  last_sr_reply:[%s]\n", handle.last_sr_event->body);
		}
		sleep(60);
	}
}
static void Outcall(int condpayid,string taskid,string phonecall)
{
	//char callCmd[256]={0};
	//sprintf(callCmd,"bgapi originate {ignore_early_media=true,company_id=%d,taskID=%s}sofia/gateway/ingw/88%s &park()",condpayid,taskid.c_str(),phonecall.c_str());
//	esl_send_recv(&handle,callCmd);  
}
void ManagerDN::PlayBack(esl_handle_t *handle, string recorefile, string uuid)
{
	 esl_execute(handle, "break", NULL, uuid.c_str());
     esl_status_t t=esl_execute(handle, "playback", recorefile.c_str(), uuid.c_str());
     esl_log(ESL_LOG_INFO, "PlayBack: %d,recorefile:%s\n", t,recorefile.c_str());
}
void ManagerDN::TransformAgent(esl_handle_t *handle,string uuid,ivrsession session)
{
		session.m_handle=handle;
		session.m_uuid=uuid;  
		printf("in  ManagerDN::TransformAgent in_queue\n");
		Instance()->m_acdqueue.in_queue(session);
}
void ManagerDN::inline_TransformAgent(string strdn,ivrsession session)
{
	printf("in ManagerDN::inline_TransformAgent strdn:%s\n",strdn.c_str());
	Instance()->m_DNmap[strdn].SetagnetStatus(DNuser::DN_Ringing);
	Instance()->m_presisted.Changestate(Instance()->m_DNmap[strdn]);
	char allagent[16]={0};
	sprintf(allagent,"user/%s",strdn.c_str());
	esl_execute(session.m_handle, "break", NULL, session.m_uuid.c_str());
	esl_status_t iret = esl_execute(session.m_handle, "bridge", /*"user/1005"*/allagent,session.m_uuid.c_str());
	printf("allagent=%s,uuid=%s,iret=%d\n",allagent,session.m_uuid.c_str(),iret);
	if(session.m_newhandle!=NULL)
	{
		esl_disconnect(session.m_newhandle);
		delete session.m_newhandle;
		session.m_newhandle=NULL;
	}
}
void ManagerDN::SetDNsemaSignal()
{
	m_readyDNsema.signal();
}
string ManagerDN::GetPrefixnum()
{
	return m_prefix;
}
void ManagerDN::GetFSconfig(string& ip,string& pwd,int& port)
{
	ip=m_fsip;pwd=m_fspwd;port=m_fsport;
}
string ManagerDN::GetCompanyIdFromgate(string gatenum)
{
        vector<Route>::iterator ite = m_gRoute.begin();
        while(ite!=m_gRoute.end())
        {
            if(ite->group == gatenum)
                return ite->group_id;
            ite++;
        }
        return "";
}
int  ManagerDN::GetUserInfolist(string agentid,string department_id,string status,vector<DNuser>&userinfo)
{
	if(Instance()->m_DNmap.find(agentid)==Instance()->m_DNmap.end())
		return DN_INVALIDDN;
	 map<string,DNuser>::iterator ite=Instance()->m_DNmap.begin();
	 while(ite!=Instance()->m_DNmap.end())
	 {
		if(department_id=="")	//获取全部department下的坐席
		{
			if(status=="0")		//获取所有状态的坐席
			{
				userinfo.push_back(ite->second);
			}
			else			//获取对应状态下的坐席
			{
				if(ite->second.m_agentstatus == atoi(status.c_str()))
				{
					userinfo.push_back(ite->second);
				}
			}
		}
		else				//获取对应department的坐席
		{
			if(ite->second.m_department_id == atoi(department_id.c_str()))
			{
				if(status=="0")		//获取所有状态的坐席
				{
					userinfo.push_back(ite->second);
				}
				else			//获取对应状态下的坐席
				{
					if(ite->second.m_agentstatus == atoi(status.c_str()))
					{
						userinfo.push_back(ite->second);
					}
				}
			}

		}
		ite++;
	 }
	 return DN_OPERATOR_SUCCESSED;
}
int ManagerDN::ResetDNinfo(string agentid,int department_id,string department_name,string user_name)
{
	  if(m_DNmap.find(agentid)==m_DNmap.end())
		  return -1;
	  DNuser&user=m_DNmap[agentid];
	  user.m_department_id=department_id;
	  user.m_department_name = department_name;
	  user.m_user_name=user_name;
	  user.m_agentstatus=DNuser::DN_Waiting_ready;

}

int  ManagerDN::GetWaitingDN(int companyid)
{
	int num=0;
	map<string,DNuser>::iterator ite = Instance()->m_DNmap.begin();
	while (ite!=Instance()->m_DNmap.end())
	{
		if(companyid == ite->second.m_commpanyid)
		{
			if( ite->second.m_agentstatus == DNuser::DN_Waiting_ready)
			{
				//ite->second.SetagnetStatus(DNuser::DN_Ringing);
				num++;
			}
		}
		ite++;
	}
	return num;
}
int ManagerDN::Setcalloutsemafree()
{
	m_calloutsema.signal();
}
int ManagerDN::Waitcalloutsema()
{
	m_calloutsema.wait();
}