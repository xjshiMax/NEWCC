#ifndef __DBOPERATOR_H
#define __DBOPERATOR_H

#include <map>
#include <vector>
#include "structdef.h"
#include <iostream>
#include  <stdio.h>
using namespace std;

class db_operator_t {
public:
    static bool initDatabase();
    static bool insertCallInfoSql(const callout_info_t& callInfo);
    static bool SelectRouteAgent(map<string,vector<agent_t> >& agents,vector<Route>& vRoute);
	static bool Getagentandpwd(map<string,string>&agents);
	static bool GetDNchooserule(map<int,int>&DNrule);
	static bool GetivrTable(vector<t_ivrnode>&nodetable);
	static bool GetJavaUserInfo(string userid,t_Java_userInfo&userinfo);
	static bool GetjavauserInfoList(vector<t_Java_userInfo>&userlist);
	//static bool SelectJavauserInfo(string userid,);
    static bool insertRegInfo(const string& agent,const reg_info_t& regInfo,const string& group_id);
	static bool Getpermission(string agentid,bool&iSPermited);

	static bool GetOutcalllist(vector<t_Outcallinfo>&phonelist);	//获取预测式外呼的名单
};

#endif

