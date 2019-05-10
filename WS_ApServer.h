//2019/3/18
//使用websock接口实现客户端对接服务端。
#pragma  once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
//#include "acdcommon.h"
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using namespace std;
using namespace websocketpp;
// pull out the type of messages sent by our config
typedef websocketpp::server<websocketpp::config::asio> wsServer;
typedef wsServer::message_ptr message_ptr;

class WSapserver
{
public:
	enum{
		WSAP_SignIn,WSAP_SignOut,WSAP_SetAgentStatus,WSAP_GetAgentStatus,WSAP_ResetStatuschangetype,WSAP_ResetAutoAnswer,
		WSAP_ResetSkill,WSAP_Reset,
		WSAP_OutboundCall,WSAP_AnswerCall,WSAP_ReleaseCall,WSAP_Hold,WSAP_Retrieve,WSAP_Consult,WSAP_ConsultReconnect,WSAP_ConsultTransfer,WSAP_SingleStepTransfer,WSAP_ConsultConference
		,WSAP_ConferenceJoin,WSAP_SetAssociateData,WSAP_GetAssociateData,WSAP_JumptheQueue,WSAP_ForceSignIn,
		WSAP_ResetConfig,
		WSAP_ERRORTYPE
	};
	WSapserver();
	~WSapserver();
	bool InitApServer(std::string ApListenIp, int32_t ApListenPort, int32_t threadPoolNum);
	bool startServer();
	bool stopServer();
	void on_open(wsServer* s, websocketpp::connection_hdl hdl);
	void on_close(wsServer* s, websocketpp::connection_hdl hdl);
	void on_message(wsServer *s, websocketpp::connection_hdl hdl, message_ptr msg);
	void on_callapclient(wsServer *s, websocketpp::connection_hdl hdl,string applicationcmd);
	int Getcmdvalue(string strcmd);
	int GetcmdType(string cmdfield);
	string Onresponse(int code,string desc,string agentId,string operation);
 	string Onresponse(int code,string desc,string agentId,string strkey2,string param2);
	string Onresponse(int code,string desc,string agentId,string strkey2,string param2,string operation);
	string Onresponse(int code,string desc,string agentId,string strkey2,int param2,string operation);
	string OnSignIn(int code,string desc,string agentId,int64_t handle);
	string OngetAgentStatus(int code,string desc,string agentId,int status);
	string OnparamError(int code,string desc,string agentId);

	static void Sendevent2dn(wsServer* s, websocketpp::connection_hdl hdl,string msg,string agentId,int64_t handle,int code);
private:
	std::string m_ApListenIp;
	int32_t m_ApListenPort;
	wsServer* m_server;
	int32_t m_threadPoolNum;
};