#pragma once
#include "ACDqueue.h"
#include "DNManager.h"
//
//

void ACDqueue::run()
{
	while(m_flag)
	{
		ivrsession tvalue;
		m_q.get(tvalue,INFINITE);
		string stdDN =  ManagerDN::Instance()->GetavailableAgent(tvalue.m_companyid);
		//m_q.get(tvalue,INFINITE);
		printf("ACDqueue::run have get a DN,then turn to inline_TransformAgent\n");
		ManagerDN::Instance()->inline_TransformAgent(stdDN,tvalue);
	}
}
//
////template <class element>
////ivrsession ACDqueue<element>::GetthePriority()
////{
////	// m_eventqueue.
////}
//template <class element>
//int ACDqueue<element>::stopMsgqueue()
//{
//	m_flag=false;
//}
//template <class element>
//void ACDqueue<element>::in_queue(element p_event)
//{
//	m_q.put(p_event);
//	return 0;
//}