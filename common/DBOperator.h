#ifndef __DBOPERATOR_H
#define __DBOPERATOR_H

#include <map>
#include<vector>
#include "structdef.h"

using namespace std;

class db_operator_t {
public:
    static bool initDatabase();
    static bool insertCallInfoSql(const callout_info_t& callInfo);
    static bool SelectRouteAgent(map<string,vector<agent_t>>& agents,vector<Route>& vRoute);
    static bool insertRegInfo(const string& agent,const reg_info_t& regInfo,const string& group_id);
};

#endif

