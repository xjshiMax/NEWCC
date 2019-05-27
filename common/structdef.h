#ifndef STRUCT_DEF_H
#define STRUCT_DEF_H

#include <string>
#include<vector>
using namespace std;
#include <string.h>
/**
 * @brief 流程描述
 *   detail description
 *
 */
struct base_script_t
{
	// virtual destruct
	virtual ~base_script_t() {}
	uint32_t voice_version_id; ///
	uint32_t type;			   ///<流程类型
	uint32_t nodeId;		   ///当前结点ID
	string desc;			   ///<流程描述
	uint32_t taskId;		   // 出口ID
	string userWord;		   ///<比较关键�?比如：关键词1:出口1|关键�?:出口2
	string vox_base;		   ///<语音文件根路�?

	std::string bill_info();
};

#define set_string(dest, str) strncpy(dest, str, sizeof(dest) - 1)

typedef enum agent_state_s
{
	AGENT_IDLE,
	AGENT_BUSY,
	AGENT_FAIL
} agent_state_t;

typedef struct agent_s
{
	char exten[20];
	char uuid[50];
	agent_state_t state;
} agent_t;

struct callout_info_t
{
  public:
	int N_user_id;
	int N_enterprose_id;
	int N_company_id;
	int N_department_id;
	string N_user_number;
	string N_customer_number;
	string N_call_id;
	string N_call_record;
	string N_number_attribution;
	string N_gateway_url;
	string N_call_start_time;
	string N_call_end_time;
	int	N_call_duration;
	string N_callstate;
	string N_extension_number;
	string N_call_prefix;
	string N_record_file;
	string N_call_type;
	string N_skill_id;
	int N_delete_status;
	string N_create_at;
	string N_update_at;
	string N_create_persion;
	//////////////////////////////////////////////////////////////////////////
	string group_id;	 // bigint,
	string number;		 //
	string agent_number; //

	int call_state;		// integer DEFAULT 0, 
	string record_file; // character varying(255), 

	//////////////////////////////////////////////////////////////////////////
	string gateway_url; //gateway url
	string call_prefix; //
	string unique_uuid; //

	string extension_number;
	//////////////////////////////////////////////////////////////////////////
	uint64_t call_start_time;
	uint64_t callout_end_time;
	int call_type;
	callout_info_t()
	{
		N_user_id=0;
		N_enterprose_id=0;
		N_company_id=0;
		N_department_id=0;
		N_user_number="";
		N_customer_number="";
		N_call_id="";
		N_call_record="";
		N_number_attribution="";
		N_gateway_url="";
		N_call_start_time="";
		N_call_end_time="";
		N_call_duration=0;
		N_callstate="2";
		N_extension_number="";
		N_call_prefix="";
		N_record_file="";
		N_call_type="";
		N_skill_id="";
		N_delete_status=0;
		N_create_at="";
		N_update_at="";
		N_create_persion="";
	}
	virtual ~callout_info_t() {}
};

struct reg_info_t
{
	virtual ~reg_info_t() {}

	uint64_t startTime;
	uint64_t endTime;
};

struct Route
{
	string group;
	string agent;
	string group_id;
	string gataname;
	string prefix;
};
struct agent_login
{
	string agentid;
	string passwd;
};

struct t_ivrnode
{
	int enterprise_id;
	int company_id;
	int node_id;
	string descript;
	string user_word;
	string recordfile;
	t_ivrnode()
	{
		enterprise_id=0;
		company_id=0;
		node_id=0;
		descript="";
		user_word="";
		recordfile="";
	}
};
struct t_Java_userInfo
{
	 int user_id;
	 string user_name;
	 string nick_name;
	 string password;
	 int department_id;
	 int company_id;
	 int enterprise_id;
	 string skill_id;
	 string mobile;
	 string ext_number;
	 int delete_status;
	 string email;
	 string userable_status;
	 string login_ip;
	 t_Java_userInfo()
	 {
		 user_id=0;
		 user_name="";
		 nick_name="";
		 password="";
		 department_id=0;
		 company_id=0;
		 enterprise_id=0;
		 skill_id="";
		 mobile="";
		 email="";
		 userable_status="";
		 login_ip="";
		 ext_number="";
		 delete_status=0;
	 }
};

struct t_Outcallinfo
{
	 string  phone;
	 string name;
	 string task_id;
	 int company_id;
	 string sex;
	 t_Outcallinfo()
	 {
		 phone="";
		 name="";
		 task_id="";
		 company_id=0;
		 sex="";
	 }
};
#endif
