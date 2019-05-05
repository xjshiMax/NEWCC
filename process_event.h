#ifndef __NWAY_PROCESS_EVENT_H__
#define __NWAY_PROCESS_EVENT_H__

#include<stdio.h>
#ifdef WIN32
#include <io.h>
#else
//#include <syswait.h>
#include <unistd.h>
#include <sys/io.h>
#endif

#include <stdlib.h>
#include "esl/esl.h"
#include <string>
#include "common/codeHelper.h"

#include <map>
#include "common/structdef.h"
using namespace std;



void process_event(esl_handle_t *handle,
				   esl_event_t *event,
				   pthread_mutex_t& calloutMutex,
				   map<string, reg_info_t>& regInfoMap,
				   const vector<Route>& vRoute 
				   );


#endif