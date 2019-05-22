#include "DBOperator.h"
#include "../database/config/inirw.h"
#include "../database/dbPool.h"
#include "../base/inifile/inifile.h"
#include <string>
#include <set>
using namespace inifile;
using namespace std;
//using namespace SAEBASE;
//Mutex _databaselock;
bool db_operator_t::initDatabase()
{
	//bool bRes = false;
	//inirw *configRead = inirw::GetInstance("./database.conf");
	//char servername[100] = {0};
	//configRead->iniGetString("database", "servername", servername, sizeof servername, "0");
	//char username[100] = {0};
	//configRead->iniGetString("database", "username", username, sizeof username, "0");
	//char password[100] = {0};
	//configRead->iniGetString("database", "password", password, sizeof password, "0");
	IniFile IniService;
	IniService.load("database.conf");
	int iret=-1;
	string servername=IniService.getStringValue("database","servername",iret);
	string username=IniService.getStringValue("database","username",iret);
	string password=IniService.getStringValue("database","password",iret);
    DBPool::GetInstance()->initPool(servername.c_str(), username.c_str(), password.c_str(), 20);
    return true;

}

bool db_operator_t::insertCallInfoSql(const callout_info_t &callInfo)
{
    int nSuccess = 0;

    Statement *state;
    Connection *cmd;
    try
    {
        cmd = DBPool::GetInstance()->GetConnection();
        if (cmd == NULL)
        {
            printf("Connection *cmd = dbIn==NULL....\n");
        }

        state = cmd->createStatement();
        state->execute("use master_outdial");
		string tempstr="insert into t_call_info_tbl (user_id,enterprise_id,company_id,department_id,user_number, \
					   customer_number,call_id,call_record,number_attribution,gateway_url,call_start_time,call_end_time \
					   ,call_duration,call_state,extension_number,record_file,call_type,skill_id,create_at) values ";
        char query[4096] = {0};
        snprintf(query, 4096, "%s(%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s',%d,'%s','%s','%s','%s','%s','%s')",tempstr.c_str(),callInfo.N_user_id,callInfo.N_enterprose_id,callInfo.N_company_id,callInfo.N_department_id,
			callInfo.N_user_number.c_str(),callInfo.N_customer_number.c_str(),callInfo.N_call_id.c_str(),callInfo.N_call_record.c_str(),
			callInfo.N_number_attribution.c_str(),callInfo.N_gateway_url.c_str(),callInfo.N_call_start_time.c_str(),callInfo.N_call_end_time.c_str(),
			callInfo.N_call_duration,callInfo.N_callstate.c_str(),callInfo.N_extension_number.c_str(),callInfo.N_record_file.c_str(),callInfo.N_call_type.c_str(),callInfo.N_skill_id.c_str(),callInfo.N_create_at.c_str());
		bool retStatus = state->executeQuery(query);
        printf("insert sql=%s\n", query);
    }
    catch (sql::SQLException &ex)
    {
        printf("SelectSql error:%s\n", ex.what());
        nSuccess = -1;
    }
    delete state;
    DBPool::GetInstance()->ReleaseConnection(cmd);

    return nSuccess;
}
bool db_operator_t::Getagentandpwd(map<string,string>&agents)
{
	int nSuccess = 0;

	Statement *state;
	Connection *cmd;
	ResultSet *result;

	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select * from agenttbl";
		result = state->executeQuery(query);
		while (result->next())
		{
			string agentid = result->getString("agentid");
			string passwd  = result->getString("passwd");
			agents.insert(pair<string,string>(agentid,passwd));
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return nSuccess;
}
bool db_operator_t::GetivrTable(vector<t_ivrnode>&nodetable)
{
	int nSuccess = 0;

	Statement *state;
	Connection *cmd;
	ResultSet *result;

	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select * from ivr_node_flow_tbl";
		result = state->executeQuery(query);
		while (result->next())
		{
			t_ivrnode node;
			node.enterprise_id = result->getInt("enterprise_id");
			node.company_id = result->getInt("company_id");
			node.node_id = result->getInt("node_id");
			node.descript = result->getString("descript");
			node.user_word = result->getString("user_word");
			node.recordfile = result->getString("recordfile");
			nodetable.push_back(node);
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return nSuccess;
}

bool db_operator_t::GetJavaUserInfo(string userid,t_Java_userInfo&userinfo)
{
	//int nSuccess = 0;

	Statement *state;
	Connection *cmd;
	ResultSet *result;
	bool iret =false;
	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select * from t_user_tbl where ext_number = '"+userid;
		query+="' "	;
		result = state->executeQuery(query);
		while (result->next())
		{
			//t_ivrnode node;
			userinfo.enterprise_id = result->getInt("enterprise_id");
			userinfo.company_id = result->getInt("company_id");
			userinfo.email =   result->getString("email");
			userinfo.department_id =  result->getInt("department_id");
			userinfo.login_ip = result->getString("login_ip");
			userinfo.mobile = 	result->getString("mobile");
			userinfo.nick_name	= result->getString("nick_name");
			userinfo.password  = result->getString("password");
			userinfo.skill_id  = result->getString("skill_id");
			userinfo.ext_number = result->getString("ext_number");
			userinfo.delete_status =   result->getInt("delete_status");
			userinfo.user_name =  result->getString("user_name");
			iret=true;
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		//nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return iret;
}
bool db_operator_t::GetjavauserInfoList(vector<t_Java_userInfo>&userlist)
{
	Statement *state;
	Connection *cmd;
	ResultSet *result;
	bool iret =false;
	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select * from t_user_tbl ";
		result = state->executeQuery(query);
		while (result->next())
		{
			t_Java_userInfo userinfo;
			userinfo.enterprise_id = result->getInt("enterprise_id");
			userinfo.company_id = result->getInt("company_id");
			userinfo.email =   result->getInt("email");
			userinfo.department_id =  result->getInt("department_id");
			userinfo.login_ip = result->getInt("department_id");
			userinfo.mobile = 	result->getInt("mobile");
			userinfo.nick_name	= result->getInt("nick_name");
			userinfo.password  = result->getInt("password");
			userinfo.skill_id  = result->getInt("skill_id");
			userlist.push_back(userinfo);
			iret=true;
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		//nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return iret;
}

bool db_operator_t::SelectRouteAgent(map<string, vector<agent_t> > &agents, vector<Route> &vRoute)
{
    int nSuccess = 0;

    Statement *state;
    Connection *cmd;
    ResultSet *result;

    try
    {
        cmd = DBPool::GetInstance()->GetConnection();
        if (cmd == NULL)
        {
            printf("Connection *cmd = dbIn==NULL....\n");
        }
        Route route;
        state = cmd->createStatement();
        state->execute("use master_outdial");

        string query = "select * from call_in_route_tbl";
        result = state->executeQuery(query);
        set<string> groups;
        while (result->next())
        {
            string group = result->getString("group_number");
            groups.insert(group);
            route.group = group;
            route.agent = result->getString("agent_number");
            route.group_id = result->getString("group_id");
            vRoute.push_back(route);
        }

        set<string>::iterator it;
        for (it = groups.begin(); it != groups.end(); it++)
        {
            agent_s ag;
            vector<agent_s> vAg;
            for (size_t i = 0; i < vRoute.size(); i++)
            {
                if (vRoute.at(i).group == *it)
                {
                    set_string(ag.exten, vRoute.at(i).agent.c_str());
                    ag.state = AGENT_IDLE;
                    vAg.push_back(ag);
                }
            }
            if (vAg.size() > 0)
            {
                agents.insert(pair<string, vector<agent_s>>(*it, vAg));
            }
        }
    }
    catch (sql::SQLException &ex)
    {
        printf("SelectSql error:%s\n", ex.what());
        nSuccess = -1;
    }
    delete result;
    delete state;
    DBPool::GetInstance()->ReleaseConnection(cmd);

    return nSuccess;
}

bool db_operator_t::insertRegInfo(const string &agent, const reg_info_t &regInfo, const string &group_id)
{
    int nSuccess = 0;

    Statement *state;
    Connection *cmd;
    try
    {
        cmd = DBPool::GetInstance()->GetConnection();
        if (cmd == NULL)
        {
            printf("Connection *cmd = dbIn==NULL....\n");
        }

        state = cmd->createStatement();
        state->execute("use master_outdial");

        char query[1024] = {0};
        snprintf(query, 1024, "INSERT INTO reg_info_tbl(group_id,reg_state,reg_start,reg_end,agent) VALUES ('%s', '%d','%ld','%ld','%s')", 
        group_id.c_str(), 2, regInfo.startTime, regInfo.endTime, agent.c_str());
        bool retStatus = state->executeQuery(query);
        printf("insert sql=%s\n", query);
    }
    catch (sql::SQLException &ex)
    {
        printf("SelectSql error:%s\n", ex.what());
        nSuccess = -1;
    }
    delete state;
    DBPool::GetInstance()->ReleaseConnection(cmd);

    return nSuccess;
}
bool db_operator_t::Getpermission(string agentid,bool&iSPermited)
{
	Statement *state;
	Connection *cmd;
	ResultSet *result;
	bool iret =false;
	iSPermited=true;
	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select delete_status from t_user_role where user_id='"+agentid;
		query+="' ";
		result = state->executeQuery(query);
		while (result->next())
		{
			// delete_status == 
			
			iret=true;
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		//nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return iret;
}

bool db_operator_t::GetOutcalllist(vector<t_Outcallinfo>&phonelist)
{
	Statement *state;
	Connection *cmd;
	ResultSet *result;
	bool iret =false;
	//iSPermited=true;
	try
	{
		cmd = DBPool::GetInstance()->GetConnection();
		if (cmd == NULL)
		{
			printf("Connection *cmd = dbIn==NULL....\n");
		}
		Route route;
		state = cmd->createStatement();
		state->execute("use master_outdial");

		string query = "select * from outdial_task_tbl ";
		result = state->executeQuery(query);
		while (result->next())
		{
			t_Outcallinfo info;
			info.task_id = 	result->getString("task_id");
			info.sex = 		result->getString("sex");
			info.name = 	result->getString("name");
			info.phone = 	result->getString("phone");
			info.company_id = 	result->getInt("company_id");

			phonelist.push_back(info);
			iret=true;
		}

	}
	catch (sql::SQLException &ex)
	{
		printf("SelectSql error:%s\n", ex.what());
		//nSuccess = -1;
	}
	delete result;
	delete state;
	DBPool::GetInstance()->ReleaseConnection(cmd);

	return iret;
}
