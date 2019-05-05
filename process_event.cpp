#include "process_event.h"
#include <stdio.h>
#include <cstdlib>
#include <iconv.h>
#include <iostream>
#include "common/DBOperator.h"

map<string, callout_info_t> callInfoMap;

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

void process_event(esl_handle_t *handle,
				   esl_event_t *event,
				   pthread_mutex_t &calloutMutex,
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

	string event_subclass, contact, from_user;
	switch (event->event_id)
	{
	case ESL_EVENT_CUSTOM:
	{
		event_subclass = esl_event_get_header(event, "Event-Subclass") ? esl_event_get_header(event, "Event-Subclass") : "";
		contact = esl_event_get_header(event, "contact") ? esl_event_get_header(event, "contact") : "";
		from_user = esl_event_get_header(event, "from-user") ? esl_event_get_header(event, "from-user") : "";

		if (event_subclass == "sofia::register")
		{
			esl_log(ESL_LOG_INFO, "sofia::register  %s, %d event_subclass=%s, contact=%s, from-user=%s\n", __FILE__, __LINE__, event_subclass.c_str(), contact.c_str(), from_user.c_str());
			if (!getGroupId(from_user, vRoute).empty())
			{
				reg_info_t info;
				info.startTime = time((time_t *)NULL);
				regInfoMap[from_user] = info;
				esl_log(ESL_LOG_INFO, "register::%s,%d\n", from_user.c_str(), info.startTime);
			}
		}
		else if (event_subclass == "sofia::unregister")
		{
			if (!getGroupId(from_user, vRoute).empty())
			{
				printf("nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\n");

				map<string, reg_info_t>::iterator iter = regInfoMap.find(from_user);
				if (iter != regInfoMap.end())
				{
					reg_info_t info = iter->second;
					pthread_mutex_lock(&calloutMutex);
					regInfoMap.erase(from_user);
					info.endTime = time((time_t *)NULL);
					string group_id = getGroupId(from_user, vRoute);
					db_operator_t::insertRegInfo(from_user, info, group_id);

					pthread_mutex_unlock(&calloutMutex);
					esl_log(ESL_LOG_INFO, "unregister::%s,%d,%d\n", from_user.c_str(), info.startTime, info.endTime);
				}
			}
		}
		break;
	}
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

		break;
	}
	case ESL_EVENT_CHANNEL_ORIGINATE:
	{
		//������ʼ
		string is_callout;
		strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";

		destination_number = esl_event_get_header(event, "Caller-Destination-Number");
		string createTime = esl_event_get_header(event, "Caller-Channel-Created-Time") ? esl_event_get_header(event, "Caller-Channel-Created-Time") : "";

		esl_log(ESL_LOG_INFO, "ESL_EVENT_CHANNEL_ORIGINATE :%s\n", strUUID.c_str());
		int dl = destination_number.find("+");
		int cl = caller_id.find("+");

		if (dl >= 0 || cl >= 0 || destination_number.length() > 13 || caller_id.length() > 13)
		{
			esl_log(ESL_LOG_INFO, " over length:%s,%s\n",
					destination_number.c_str(), caller_id.c_str());

			return;
		}

		pthread_mutex_lock(&calloutMutex);
		uint64_t startTime = time(NULL);

		callout_info_t callinfo;
		callinfo.call_start_time = startTime;
		callinfo.agent_number = caller_id;
		callinfo.number = destination_number.substr(2);

		callinfo.unique_uuid = strUUID;
		callinfo.call_state = 0;
		callinfo.group_id = getGroupId(caller_id, vRoute);
		callinfo.gateway_url = "1";
		if (callinfo.group_id == "3")
		{
			callinfo.call_prefix = "89";
		}
		else
		{
			callinfo.call_prefix = "83";
		}

		callinfo.extension_number = getGroupNumber(caller_id, vRoute);

		callinfo.record_file = strUUID + getcurrenttime() + ".wav";
		if (destination_number.length() > 5)
		{
			callinfo.call_type = 2;
		}
		else
		{
			callinfo.call_type = 1;
		}

		callInfoMap[strUUID] = callinfo;
		pthread_mutex_unlock(&calloutMutex);

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
			esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_DESTROY :%s\n", strUUID.c_str());

			callout_info_t info = iter->second;
			if (info.extension_number.empty())
			{
				return;
			}

			pthread_mutex_lock(&calloutMutex);
			callInfoMap.erase(strUUID);
			info.callout_end_time = time((time_t *)NULL);

			// info.callout_end_time = atol(hangupTime.c_str()) / 1000000;
			db_operator_t::insertCallInfoSql(info);
			pthread_mutex_unlock(&calloutMutex);
		}

		break;
	}
	case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
	{
		strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
		string billsec = esl_event_get_header(event, "variable_billsec") ? esl_event_get_header(event, "variable_billsec") : "";
			esl_log(ESL_LOG_INFO, "CALL OUT HANGUP_COMPLETE :%s,%s\n", strUUID.c_str(),billsec.c_str());

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

		break;
	}
	case ESL_EVENT_CHANNEL_ANSWER:
	{
		map<string, callout_info_t>::iterator iter = callInfoMap.find(strUUID);
		esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_ANSWER :%s %d\n", strUUID.c_str(), callInfoMap.size());
		if (iter != callInfoMap.end())
		{
			iter->second.call_state = 1;
			esl_log(ESL_LOG_INFO, " ESL_EVENT_CHANNEL_ANSWER  input:%s %d\n", strUUID.c_str(), callInfoMap.size());

			char tmp_cmd[300];
			string filePath;
			filePath = "/home/records/" + iter->second.record_file;
			sprintf(tmp_cmd, "api uuid_record %s start %s 9999 \n\n", strUUID.c_str(), filePath.c_str());
			// sprintf(tmp_cmd, "api uuid_record %s start %s 9999 \n\n", caller_uuid.c_str(), strFullname.c_str());

			esl_send_recv_timed(handle, tmp_cmd, 1000);
		}
		string createTime = esl_event_get_header(event, "Caller-Channel-Created-Time") ? esl_event_get_header(event, "Caller-Channel-Created-Time") : "";

		break;
	}

	case ESL_EVENT_CHANNEL_OUTGOING:
	{
		string is_callout;
		is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; // ����Ϊ1�������������?
			/*printf("body:\n%s\n",eventbody);*/
		break;
	}
	case ESL_EVENT_PLAYBACK_START:
	{

		string is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; // ����Ϊ1�������������?

		{
			esl_log(ESL_LOG_INFO, "CALL IN ESL_EVENT_PLAYBACK_START %s\n", strUUID.c_str());
		}
		break;
	}
	case ESL_EVENT_PLAYBACK_STOP:
	{
		//������������
		destination_number = esl_event_get_header(event, "Caller-Destination-Number");
		strUUID = esl_event_get_header(event, "Caller-Unique-ID") ? esl_event_get_header(event, "Caller-Unique-ID") : "";
		string is_callout, a_leg_uuid;
		is_callout = esl_event_get_header(event, "variable_is_callout") ? esl_event_get_header(event, "variable_is_callout") : ""; // ����Ϊ1�������������?
		{
			esl_log(ESL_LOG_INFO, "CALL IN ESL_EVENT_PLAYBACK_STOP %s\n", strUUID.c_str());
		}
		// esl_execute(handle, "hangup", NULL, strUUID.c_str());

		break;
	}
	case ESL_EVENT_CHANNEL_PROGRESS:
	{
		break;
	}
	case ESL_EVENT_TALK:
	{
		esl_log(ESL_LOG_INFO, "ESL_EVENT_TALK %s\n", strUUID.c_str());

		break;
	}
	}
}
