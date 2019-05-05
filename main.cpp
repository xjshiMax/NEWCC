#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>
#include <pthread.h>
#include "process_event.h"
#include "common/structdef.h"
#include "common/DBOperator.h"
// #include "common/codeHelper.h"
#include <iostream>
#include <fstream>

using namespace std;
pthread_mutex_t agentMutex;     //即时呼叫的互斥变量
pthread_mutex_t infoMutex;      //用于处理callinfo的互斥变量
pthread_mutex_t calloutMutex;   // 外呼任务的互斥变量
pthread_mutex_t configMutex;    //基本配置模块的互斥变量
pthread_mutex_t clickDialMutex; //点击呼叫模块的互斥变量

static esl_mutex_t *gMutex = NULL;
static int last_agent_index;
map<string, reg_info_t> regInfoMap;

map<string, vector<agent_t>> agentRoute;
static vector<Route> gRoute;

void init_agents()
{
    db_operator_t::SelectRouteAgent(agentRoute, gRoute);
}

string getGroupIdMain(const string &agent, const vector<Route> &vRoute)
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

agent_t *find_available_agent(string callInNumber)
{
    esl_mutex_lock(gMutex);

    map<string, vector<agent_t>>::iterator iter;
    iter = agentRoute.find(callInNumber);
    int last;
    int maxAgent = 0;
    if (iter != agentRoute.end())
    {
        maxAgent = (iter->second).size();
        last_agent_index = maxAgent;
        last = last_agent_index;
    }

    agent_t *agent = NULL;

    while (true)
    {
        if (last_agent_index >= maxAgent - 1)
        {
            last_agent_index = 0;
        }
        else
        {
            last_agent_index++;
        }

        agent = &((iter->second).at(last_agent_index));

        if (agent == NULL)
        {
            esl_mutex_unlock(gMutex);

            return agent;
        }

        esl_log(ESL_LOG_INFO, "agent info [%d,%s,%s]\n", last_agent_index, agent->exten, agent->state == AGENT_IDLE ? "IDLE" : "IDLE");
            return agent;
        map<string, reg_info_t>::iterator itReg;
        itReg = regInfoMap.find(agent->exten);

        // if (itReg == regInfoMap.end())
        // {
        //     continue;
        // }

        if (agent->state == AGENT_IDLE)
        {
            agent->state = AGENT_BUSY;
            esl_mutex_unlock(gMutex);
            return agent;
        }
        else if (agent->state == AGENT_BUSY)
        {

            esl_mutex_unlock(gMutex);
            continue;
        }

        if (last_agent_index == last)
        {
            break;
        }
    }
    esl_mutex_unlock(gMutex);
    return agent;
}

void reset_agent(agent_t *agent, agent_state_t state)
{
    esl_mutex_lock(gMutex);
    agent->state = state;
    *agent->uuid = '\0';
    esl_mutex_unlock(gMutex);
}

void *Inbound_Init(void *arg)
{

    esl_handle_t handle = {{0}};
    esl_status_t status;
    const char *uuid;

    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

    // status = esl_connect(&handle, "210.21.48.69", 8021, NULL, "tx@infosun");
    status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "tx@infosun");

    if (status != ESL_SUCCESS)
    {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        exit(1);
    }
    esl_log(ESL_LOG_INFO, "Connected to FreeSWITCH Inbound_Init\n");
    esl_events(&handle, ESL_EVENT_TYPE_PLAIN,
               "DETECTED_SPEECH RECORD_START RECORD_STOP PLAYBACK_START PLAYBACK_STOP CHANNEL_OUTGOING CHANNEL_PARK CHANNEL_EXECUTE_COMPLETE CHANNEL_ORIGINATE TALK NOTALK PHONE_FEATURE CHANNEL_HANGUP_COMPLETE CHANNEL_CREATE CHANNEL_BRIDGE DTMF CHANNEL_DESTROY CHANNEL_HANGUP CHANNEL_BRIDGE CHANNEL_ANSWER CUSTOM sofia::register sofia::unregister asr");
    esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

    handle.event_lock = 1;
    while ((status = esl_recv_event(&handle, 1, NULL)) == ESL_SUCCESS)
    {
        if (handle.last_ievent)
        {

            {
                process_event(&handle, handle.last_ievent, calloutMutex, regInfoMap, gRoute);
            }
        }
    }

    esl_disconnect(&handle);

    return (void *)0;
}

//外呼处理
void *CallOut_Task_Process(void *arg)
{

    esl_handle_t handle = {{0}};
    esl_status_t status;
    char uuid[128]; //从fs中获得的uuid
    //Then running the Call_Task string when added a new Task,then remove it

    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

    status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "tx@infosun");

    if (status != ESL_SUCCESS)
    {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        exit(1);
    }

    //为了cps保证30以内，需要每路延时30ms
#ifdef WIN32
    Sleep(30);
#else
    //usleep(3000);
    struct timeval tempval;
    tempval.tv_sec = 0;
    tempval.tv_usec = 30;
    select(0, NULL, NULL, NULL, &tempval);
#endif

    esl_disconnect(&handle);

    return (void *)0;
}

static void nwaycc_callback(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *userData)
{
    char uuid[128];

    char cmd_tmp[128], dtmf[128];
    esl_handle_t handle = {{0}};

    esl_attach_handle(&handle, client_sock, addr);

    printf("Connected! %d", handle.sock);

    // esl_filter(&handle, "unique-id", esl_event_get_header(handle.info_event, "caller-unique-id"));

    esl_send_recv(&handle, "myevent");

    esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "CHANNEL_DESTROY CHANNEL_STATE CHANNEL_CALLSTATE CHANNEL_PROGRESS_MEDIA CHANNEL_OUTGOING SESSION_HEARTBEAT CHANNEL_ANSWER CHANNEL_ORIGINATE CHANNEL_PROGRESS CHANNEL_HANGUP "
                                              "CHANNEL_BRIDGE CHANNEL_UNBRIDGE CHANNEL_OUTGOING CHANNEL_HANGUP_COMPLETE CHANNEL_EXECUTE CHANNEL_EXECUTE_COMPLETE DTMF CUSTOM "
                                              "TALK OUTBOUND_CHAN INBOUND_CHAN RECORD_START RECORD_STOP");

    //This will send a command and place its response event on handle->last_sr_event and handle->last_sr_reply
    esl_send_recv(&handle, "linger 5"); //Tells FreeSWITCH not to close the socket connect when a channel hangs up. Instead, it keeps the socket connection open until the last event related to the channel has been received by the socket client.
    // esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

    string caller_uuid;
    string content_type;
    string event_name;
    string call_direction;
    string b_uuid;
    string channel_state;
    string caller_destination_number;
    string caller_username;
    string caller_number;
    string channel_call_state;
    string gateway_name = "";
    string is_callout = "";
    int nStatus = 0;
    bool hasAnswer = false;
    //////////////////////////////////////////////////////////////////////////
    //我用的只有origination_uuid、Unique-ID。现在的处理是首先接受channel_create事件，记录下号码跟uuid的对应关系。然后后面接收到progress\answer\hangup的时候，直接匹配Unique-ID，就能对应上了。
    //////////////////////////////////////////////////////////////////////////
    caller_uuid = esl_event_get_header(handle.info_event, "Caller-Unique-ID") ? esl_event_get_header(handle.info_event, "Caller-Unique-ID") : "";
    content_type = esl_event_get_header(handle.info_event, "Content-Type") ? esl_event_get_header(handle.info_event, "Content-Type") : "";
    event_name = esl_event_get_header(handle.info_event, "Event-Name") ? esl_event_get_header(handle.info_event, "Event-Name") : "";
    call_direction = esl_event_get_header(handle.info_event, "Call-Direction") ? esl_event_get_header(handle.info_event, "Call-Direction") : "";
    b_uuid = esl_event_get_header(handle.info_event, "Unique-ID") ? esl_event_get_header(handle.info_event, "Unique-ID") : "";
    channel_state = esl_event_get_header(handle.info_event, "Channel-State") ? esl_event_get_header(handle.info_event, "Channel-State") : "";
    caller_destination_number = esl_event_get_header(handle.info_event, "Caller-Destination-Number") ? esl_event_get_header(handle.info_event, "Caller-Destination-Number") : "";
    caller_number = esl_event_get_header(handle.info_event, "Caller-Caller-ID-Number") ? esl_event_get_header(handle.info_event, "Caller-Caller-ID-Number") : "";
    //////////////////////////////////////////////////////////////////////////
    //处理call info

    // ci.caller_callin_tm = time(NULL);

    //////////////////////////////////////////////////////////////////////////
    esl_log(ESL_LOG_INFO, "has a call in from %s to %s\n", caller_number.c_str(), caller_destination_number.c_str());

    //外线呼入的
    //呼入的，走dialplan匹配
    //加一个主叫长度判断，从外呼入的，需要另外配对，相当于处理did或public.xml中的流程
    int callerNumberLen = caller_number.length();

    if (caller_destination_number.length() > 13)
    {
        goto end;
    }

    if (callerNumberLen > 3 && caller_destination_number.length() > 3)
    {

        esl_execute(&handle, "answer", NULL, NULL);
        char* cc=esl_event_get_header(handle.last_sr_event, "Reply-Text");
            esl_log(ESL_LOG_INFO, "CCCCC-===%s\n", cc);

        esl_execute(&handle, "set", "continue_on_fail=true", NULL);
        esl_execute(&handle, "set", "hangup_after_bridge=true", NULL);
        // esl_execute(&handle, "",playback "local_stream://call", NULL);
        // callout operation
        if (callerNumberLen > 0 && callerNumberLen < 5 && caller_destination_number.length() > 6)
        {
            if (getGroupIdMain(caller_number, gRoute) == "3")
            {
                caller_destination_number = "87" + caller_destination_number;
            }
            else
            {
                caller_destination_number =  caller_destination_number;
            }

            char outDial[3000] = {0};
            sprintf(outDial, "sofia/gateway/reingw/%s", caller_destination_number.c_str());
            // sprintf(outDial, "sofia/gateway/smartcc/%s", caller_destination_number.c_str());
            esl_log(ESL_LOG_INFO, "%s\n", outDial);
            esl_execute(&handle, "bridge", outDial, NULL);
            goto end;
        }
        // callin
        esl_status_t status = ESL_SUCCESS;
        agent_t *agent = NULL;
        while (ESL_SUCCESS == status || ESL_BREAK == status)
        {
            status = esl_recv_timed(&handle, 1000);
            const char *type, *application;
            if (status == ::ESL_BREAK)
            {
                printf("status=%d\n", status);
                if (!agent)
                {
                    esl_log(ESL_LOG_INFO, "caller_destination_number %s\n", caller_destination_number.c_str());

                    agent = find_available_agent(caller_destination_number);

                    if (agent)
                    {
                        char dialStr[2014];

                        sprintf(dialStr, "user/%s", agent->exten);
                        esl_log(ESL_LOG_INFO, "%s\n", dialStr);
                        // esl_execute(&handle, "break", NULL, NULL);
                        esl_execute(&handle, "bridge", dialStr, NULL);

                        esl_log(ESL_LOG_INFO, "Calling:%s\n", dialStr);
                    }
                }

                continue;
            }
            if (!agent)
            {
                continue;
            }

            if (handle.last_ievent)
            {

                switch (handle.last_ievent->event_id)
                {
                case ESL_EVENT_CHANNEL_BRIDGE:
                {
                    set_string(agent->uuid, esl_event_get_header(handle.last_ievent, "Other-Leg-Unique-ID"));
                    esl_log(ESL_LOG_INFO, "bridge to %s\n", agent->exten);

                    break;
                }
                case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
                {
                    esl_log(ESL_LOG_INFO, "caller hang up  to %s\n", agent->exten);
                    if (agent)
                    {
                        reset_agent(agent, AGENT_IDLE);
                    }
                    goto end;
                }
                break;
                case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
                {
                    application = esl_event_get_header(handle.last_ievent, "Application");
                    if (!strcmp(application, "bridge"))
                    {
                        const char *disposition = esl_event_get_header(handle.last_ievent, "variable_originate_disposition");
                        esl_log(ESL_LOG_INFO, "disposition %s\n", disposition);
                        if (!strcmp(disposition, "USER_BUSY") || !strcmp(disposition, "CALL_REJECTED"))
                        {
                            reset_agent(agent, AGENT_IDLE);
                            agent = NULL;
                        }
                        else if (!strcmp(disposition, "USER_NOT_REGISTERED"))
                        {
                            reset_agent(agent, AGENT_FAIL);
                            agent = NULL;
                        }
                    }

                    break;
                }
                default:
                    break;
                }
            }
        }
    }
end:
    esl_disconnect(&handle);
}

void *test_Process(void *arg)
{
    esl_handle_t handle = {{0}};
    esl_status_t status;
    char uuid[128]; //从fs中获得的uuid
    //Then running the Call_Task string when added a new Task,then remove it
    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

    status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "tx@infosun");

    if (status != ESL_SUCCESS)
    {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        exit(1);
    }

    esl_send_recv(&handle, "bgapi originate user/1007 &park()");

    if (handle.last_sr_event && handle.last_sr_event->body)
    {
        printf("[%s]\n", handle.last_sr_event->body);
    }
    else
    {
        printf("[%s] last_sr_reply\n", handle.last_sr_reply);
    }
}

void *Heatbeat_Process(void *arg)
{
    esl_handle_t handle = {{0}};
    esl_status_t status;
    char uuid[128]; //从fs中获得的uuid
    //Then running the Call_Task string when added a new Task,then remove it

    status = esl_connect(&handle, "127.0.0.1", 8021, NULL, "tx@infosun");
    esl_global_set_default_logger(ESL_LOG_LEVEL_INFO);

    if (status != ESL_SUCCESS)
    {
        esl_log(ESL_LOG_INFO, "Connect Error: %d\n", status);
        exit(1);
    }

    while (true)
    {
        esl_send_recv(&handle, "api create_uuid\n\n");

        if (handle.last_sr_event && handle.last_sr_event->body)
        {
            esl_log(ESL_LOG_INFO, "headbeat uuid:[%s]\n", handle.last_sr_event->body);
        }
        else
        {
            esl_log(ESL_LOG_INFO, "headbeat  last_sr_reply:[%s]\n", handle.last_sr_event->body);
        }
        sleep(60);
    }
}

int main(int argc, char const *argv[])
{
    bool bSuccess = false;
    pthread_mutex_init(&agentMutex, NULL);
    pthread_mutex_init(&infoMutex, NULL);
    pthread_mutex_init(&calloutMutex, NULL);
    pthread_mutex_init(&configMutex, NULL);
    pthread_mutex_init(&clickDialMutex, NULL);

    db_operator_t::initDatabase();

    int ret = 0;
    pthread_t pthid1, pthid2, pthid3, pthid4, pthid5;

    ret = pthread_create(&pthid1, NULL, Inbound_Init, NULL);
    if (ret) // 非0则创建失败
    {
        perror("createthread 1 failed.\n");
        return 1;
    }
    ret = pthread_create(&pthid2, NULL, Heatbeat_Process, NULL);
    if (ret) // 非0则创建失败
    {
        perror("createthread 3 failed.\n");
        return 1;
    }

    // ret = pthread_create(&pthid3, NULL, test_Process, NULL);
    // if (ret) // 非0则创建失败
    // {
    //     perror("createthread 3 failed.\n");
    //     return 1;
    // }

    esl_mutex_create(&gMutex);

    init_agents();

    esl_log(ESL_LOG_INFO, "ACD Server listening at localhost :8040....\n");
    esl_listen_threaded("127.0.0.1", 8040, nwaycc_callback, NULL, 100000);
    printf("cccc\n");
    esl_mutex_destroy(&gMutex);
    pthread_join(pthid1, NULL);
    pthread_join(pthid2, NULL);
    pthread_join(pthid3, NULL);
    pthread_join(pthid4, NULL);

    pthread_mutex_destroy(&infoMutex);
    pthread_mutex_destroy(&agentMutex);
    pthread_mutex_destroy(&calloutMutex);
    pthread_mutex_destroy(&configMutex);
    pthread_mutex_destroy(&clickDialMutex);

    return 0;
}
