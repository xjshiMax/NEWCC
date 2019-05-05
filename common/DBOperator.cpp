#include "DBOperator.h"
#include "../database/config/inirw.h"
#include "../database/dbPool.h"

#include <set>

bool db_operator_t::initDatabase()
{
    bool bRes = false;
    inirw *configRead = inirw::GetInstance("/root/txbeta/database.conf");
    char servername[100] = {0};
    configRead->iniGetString("database", "servername", servername, sizeof servername, "0");
    char username[100] = {0};
    configRead->iniGetString("database", "username", username, sizeof username, "0");
    char password[100] = {0};
    configRead->iniGetString("database", "password", password, sizeof password, "0");

    printf("%s\n", username);
    // DBPool::GetInstance()->initPool("127.0.0.1:3306", "root", "123456", 20);
    DBPool::GetInstance()->initPool(servername, username, password, 20);
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
        state->execute("use smartcc");

        char query[1024] = {0};
        snprintf(query, 1024, "INSERT INTO in_and_out_call_info_tbl(group_id,phone_number,unique_uuid,gateway_url,call_start_time,callout_state,agent_number,extension_number,callout_end_time,call_prefix,record_file,call_type) VALUES ('%s', '%s','%s','%s','%ld','%d','%s','%s','%ld','%s','%s','%d')", callInfo.group_id.c_str(), callInfo.number.c_str(), callInfo.unique_uuid.c_str(), callInfo.gateway_url.c_str(), callInfo.call_start_time, callInfo.call_state, callInfo.agent_number.c_str(), callInfo.extension_number.c_str(), callInfo.callout_end_time, callInfo.call_prefix.c_str(), callInfo.record_file.c_str(),callInfo.call_type);
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

bool db_operator_t::SelectRouteAgent(map<string, vector<agent_t>> &agents, vector<Route> &vRoute)
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
        state->execute("use smartcc");

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
        state->execute("use smartcc");

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
