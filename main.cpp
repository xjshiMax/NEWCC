#include<stdio.h>
#include "DNManager.h"
using namespace std;

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
int main(int argc, char const *argv[])
{
   
	ManagerDN* pmanager=ManagerDN::Instance();
	pmanager->loaddb();
	pmanager->startServer();


    return 0;
}
