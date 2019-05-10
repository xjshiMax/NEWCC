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
		string peerIP,wsServer *s, websocketpp::connection_hdl hdl):
	m_agentid(agentId),m_DN(agentDn),m_agentPwd(agentPwd),m_agentstatus(statusChangetype),m_autoAnswer(autoAnswer),
		m_fcSignin(fcSignin),m_skills(skills),m_s(s),m_hdl(hdl),m_peerIP(peerIP)
	{

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
	void inline_Setcompanyid(int companyid);
	int  inline_Getcompanyid();
	void SetagnetStatus(int agentStatus);
	void GetagnetStatus(int& agentStatus);
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
		bool fcSignin, const std::string& skills,
		string peerIP,wsServer *s, websocketpp::connection_hdl hdl);
	int  Signout(const std::string& agentId,
		string peerIP);
	int SetDNstatus(const std::string& agentId,
		const int& agentStatus, const std::string& restReason,
		string peerIP);
	int GetDNstatus(const std::string& agentId, int& agentStatus,
		string peerIP);

	int reloaddb();
	int loaddb();
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
    static void PlayBack(esl_handle_t *handle, string recorefile, string uuid);
    static void TransformAgent(esl_handle_t *handle,string uuid,ivrsession session);
	static void inline_TransformAgent(string strdn,ivrsession session);
	static void SetDNsemaSignal();
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
};