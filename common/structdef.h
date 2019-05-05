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
	string userWord;		   ///<比较关键词 比如：关键词1:出口1|关键词2:出口2
	string vox_base;		   ///<语音文件根路径

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
	//////////////////////////////////////////////////////////////////////////
	string group_id;	 // bigint,
	string number;		 //�������
	string agent_number; //��ϯ����

	int call_state;		// integer DEFAULT 0, -- ����״̬
	string record_file; // character varying(255), -- ¼���ļ�

	//////////////////////////////////////////////////////////////////////////
	string gateway_url; //gateway url
	string call_prefix; //����ǰ׺
	string unique_uuid; //����ʱaleg��uuid

	string extension_number;
	//////////////////////////////////////////////////////////////////////////
	uint64_t call_start_time;
	uint64_t callout_end_time;
	int call_type;
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
};


#endif
