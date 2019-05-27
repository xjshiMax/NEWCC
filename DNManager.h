//坐席管理类和坐席类
#pragma once
#include "WS_ApServer.h"
#include "IVRmanager.h"
#include "ACDqueue.h"
#include "common/structdef.h"
#include "esl/esl.h"
#include <set>
#include<string>
#include<map>
#include <pthread.h>
#include "base/output/include/xEmutex.h"
#include "base/output/include/xsema.h"
#include "base/sqlite3/include/sqlite3.h"
#include "CalloutManager.h"
using namespace std;
using namespace SEABASE;
class DNuser
{
public:
	DNuser(){}
	DNuser(const std::string& agentId,
		const std::string& agentDn, const std::string& agentPwd,
		const int & statusChangetype, bool autoAnswer,
		bool fcSignin, const std::string& skills,
		string peerIP,wsServer *s, websocketpp::connection_hdl hdl,int department_id,string department_name,string user_name):
	m_agentid(agentId),m_DN(agentDn),m_agentPwd(agentPwd),m_agentstatus(statusChangetype),m_autoAnswer(autoAnswer),
		m_fcSignin(fcSignin),m_skills(skills),m_s(s),m_hdl(hdl),m_peerIP(peerIP),m_department_id(department_id),m_department_name(department_name),m_user_name(user_name)
	{
		m_calltimes=0;
		m_callminiutes=0;
		m_idelminiutes=time(NULL);
	}
	virtual ~DNuser(){}
	enum DN_agentStatus
	{
		DN_NoLogin,			//未登录
		DN_Waiting_ready,	//等待呼入
		DN_Ringing,			//正待通话
		DN_Parking,			//小休
		DN_Talking,			//通话
		DN_res1,
		DN_Process,			//处理
		DN_res2,
		DN_Holding,			//保持
		DN_Held,			//被保持
		DN_Dialing,			//拨号
		DN_DialUnderHold,	//保持下拨号


	};
	enum DN_choose_rule
	{
		DNRULE_ring_all=0,
		DNRULE_long_idel_agent,
		DNRULE_select,
		DNRULE_top_down,
		DNRULE_agent_with_least_talk_time,
		DNRULE_agent_with_fewest_calls,
		DNRULE_sequentially_by_agent_order
	} ;
	int m_commpanyid;
	string m_DN;
	string m_agentid;
	string m_reason;
	int m_agentstatus;
	string m_agentPwd;
	bool m_autoAnswer;
	bool m_fcSignin;
	string m_skills;
	string m_peerIP;
	wsServer *m_s;
	websocketpp::connection_hdl m_hdl ;

	int m_department_id;
	string m_department_name;
	string m_user_name;
	void inline_Setcompanyid(int companyid);
	int  inline_Getcompanyid();
	void SetagnetStatus(int agentStatus);
	void GetagnetStatus(int& agentStatus);
	string serialize();
	void Unserialize(string serizestr);
	int Getidelminiute(); //获取空闲总时长

	int m_calltimes;	//接电话次数
	int m_callminiutes;	//通话总时长
	int m_idelminiutes; //登录初始时间

};
class SqlitePersist
{
public:
	SqlitePersist(){};
	void LoadDNSet(map<string,DNuser>&dnset);
	bool CreateDB(string daname);
	bool CloseDB();
	bool CreateTable();
	bool InsertTable(DNuser&user);
	bool DeleteTable(DNuser&user);
//	bool DeleteTable(string stddn);
	bool Changestate(DNuser&user);
	bool IsExistTable();
	sqlite3*m_pdb;
};
class ManagerDN
{
public:
	enum ReturnStatus
	{
		DN_INVALIDDN=1000,
		DN_OPERATOR_SUCCESSED,
		DN_SIGNIN_FAIL,
		DN_HAS_SIGNINED,
		DN_SIGNOUT_FAIL

	};
	ManagerDN();
	virtual ~ManagerDN();
	static ManagerDN* Instance();

	bool checkvaildDN(string dnid);
	bool checkPasswd(string dnid,string pwd);
	int Signin(const std::string& agentId,
		const std::string& agentDn, const std::string& agentPwd,
		const int & statusChangetype, bool autoAnswer,
		bool fcSignin, const std::string& skills,int department_id,const std::string department_name,string user_name,
		string peerIP,wsServer *s, websocketpp::connection_hdl hdl);
	int  Signout(const std::string& agentId,
		string peerIP);
	int SetDNstatus(const std::string& agentId,
		const int& agentStatus, const std::string& restReason,
		string peerIP);
	int GetDNstatus(const std::string& agentId, int& agentStatus,
		string peerIP);
	int ResetDNinfo(string agentid,int department_id,string department_name,string user_name);

	int reloaddb();
	int loaddb();
	int loadConfigini();
	int startServer();
	string GetskillIDfromcaller(string callernum);

	static void *Heatbeat_Process(void *arg);
	static void *listenthread_Process(void *arg);
	static void nwaycc_callback(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *userData);
	static void *CallOut_Task_Process(void *arg);
	static void *Inbound_Init(void *arg);
	static void process_event(esl_handle_t *handle,
		esl_event_t *event,
		map<string, reg_info_t>& regInfoMap,
		const vector<Route>& vRoute 
		);
	static agent_t *find_available_agent(string callInNumber);
    static string GetCompanyIdFromgate(string gatenum);
	static string GetavailableAgent(int companyid);
	static string GetcompanyidbyDN(string strDN);
	static string GetDNbyagentid(string strid);
	static void Outcall(int condpayid,string taskid,string phonecall);
    static void PlayBack(esl_handle_t *handle, string recorefile, string uuid);
    static void TransformAgent(esl_handle_t *handle,string uuid,ivrsession session);
	static void inline_TransformAgent(string strdn,ivrsession session);
	static void SetDNsemaSignal();
	string GetPrefixnum();
	void GetFSconfig(string& ip,string& pwd,int& port);
	static string GetAgent_ringall(int companyid);
	static string GetAgent_idel_agent(int companyid); //选择空闲时间最长的
	static string GetAgent_robin(int companyid);		//轮训
	static string GetAgent_Top_down(int companyid);	//固定的顺序选择
	static string GetAgent_least_talk_time(int companyid);//选择通话时间最短的坐席
	static string GetAgent_fewest_calls(int companyid);	 //选择通话次数最少的坐席
	static string GetAgent_agent_order(int companyid);	 //根据梯队和顺序选择

	static int GetUserInfolist(string agentid,string department_id,string status,vector<DNuser>&userinfo);
 	map<string,DNuser> m_DNmap;		//dn
	map<string,string> m_agentloginInfo;	//agentid--pwd
	set<string> m_validDNSet;
	WSapserver m_apserver;

	static map<string, reg_info_t> regInfoMap;
	static map<string, vector<agent_t> > m_agentRoute;	//gatenum---agent_t
	static vector<Route> m_gRoute;		//全部的路由网关号码对应的坐席号码

	static map<string, callout_info_t> callInfoMap;

	//添加ivr功能
	static map<string,ivrsession> m_ivrmap;      //uuid--ivrsession

	//添加acd功能
	ACDqueue m_acdqueue;
	static xEmutex m_agentlock;
	static xSemaphore m_readyDNsema;

	//使用sqlite做持久化
	 SqlitePersist m_presisted;

	 //预测式外呼功能
	  CalloutManager* m_Outcallmanager;
	  static xSemaphore m_calloutsema;			//同步外呼线程和dn管理线程
	  static int GetWaitingDN(int companyid);	//获取准备好的DN数量 
	  static int Setcalloutsemafree();
	  static int Waitcalloutsema();

	 //可配置常量
	  static string m_fsip;
	  static int m_fsport;
	  static string m_fspwd;

	  static string m_mysqlip;
	  static int m_mysqlport;
	  static string m_mysqluser;
	  static string m_mysqlpwd;

	  static string m_prefix;	//拨打电话前缀
	  static int m_tcpbussinessPort;
	  static int m_wsbussinessPort;
	  static string m_ServerIP;
	  static string m_FSXMLPATH;

	  static int m_DNChooseRule;
	  static map<int,int> m_compyidDNrulemap; //companyid---DNchooserule;
};