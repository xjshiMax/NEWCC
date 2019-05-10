#include "IVRmanager.h"
#include "common/DBOperator.h"
#include "base/jsoncpp/json/json.h"
map<int,map<int,t_ivrnode> > Managerivr::m_IVRnodetable;
map<string,t_flownode> Managerivr::m_flowtable;
Managerivr*Managerivr::Instance()
{
	static Managerivr mgr;
	return &mgr;
}
void Managerivr::Init()
{
	vector<t_ivrnode>nodetable;
	db_operator_t::GetivrTable(nodetable);
	Initivrflow(nodetable);
}
void Managerivr::Initivrflow(vector<t_ivrnode>&nodetable)
{
	vector<t_ivrnode>::iterator ite=nodetable.begin();
	vector<t_ivrnode>::iterator sec=ite;
	while(ite!=nodetable.end())
	{
		//vector<t_ivrnode>::iterator sec=ite;
		map<int,t_ivrnode> oneNodeTable;
        t_ivrnode tempnode;
		tempnode.company_id = ite->company_id;
		tempnode.node_id= IVR_AGENT_FLOW_INDEX;
		tempnode.recordfile = IVR_AGNET_MSG;
		tempnode.user_word="";
        oneNodeTable[IVR_AGENT_FLOW_INDEX]=tempnode;

		while(sec->company_id==ite->company_id)
		{
			oneNodeTable[sec->node_id]=*sec;
			sec++;
		}
		m_IVRnodetable[ite->company_id]=oneNodeTable;
		ite=sec;
		
	}
	
	map<int,map<int,t_ivrnode> >::iterator nodeite=m_IVRnodetable.begin();
	for(;nodeite!=m_IVRnodetable.end();nodeite++) //解析同一个company节点话术
	{
		int comyanyid=nodeite->first;
		map<int,t_ivrnode>&ponecpynode=nodeite->second;			//同一个公司，话术节点
		map<int,t_ivrnode>::iterator flownodeite = ponecpynode.begin();
		while(flownodeite!=ponecpynode.end())
		{
			string user_=flownodeite->second.user_word;
			int currentnode=flownodeite->second.node_id;
			t_flownode flownode;
			flownode.current = &(flownodeite->second);
			Json::Reader jsonread;
			Json::Value root;
			char Uniqueid[16]={0};
			if(currentnode==IVR_AGENT_FLOW_INDEX)		//虚拟节点,转人工的节点
				sprintf(Uniqueid,"%d_%d",comyanyid,IVR_AGENT_FLOW_INDEX);
			else
				sprintf(Uniqueid,"%d_%d",comyanyid,currentnode);
			if(jsonread.parse(user_,root))
			{
				Json::Value::iterator jsonite =  root.begin();
				while(jsonite!=root.end())
				{
					int intnextnode=(*jsonite)["node"].asInt();
					string strdtmf= (*jsonite)["dtmf"].asString();
					if(strdtmf=="0") //转人工
					{
//                         t_ivrnode tempnode;
//                         tempnode.company_id=flownode.current->company_id;
//                         tempnode.node_id=-1;

						flownode.next["0"]=&(ponecpynode[IVR_AGENT_FLOW_INDEX]);
					}
					else if(ponecpynode.find(intnextnode)!=ponecpynode.end()) //存在该话术节点
					{
						flownode.next[strdtmf]=&(ponecpynode[intnextnode]);
					}
					else			//指定的话术节点不存在
					{

					}
					//t_ivrnode*pnode =  Getnodeinfo(ponecpynode,strdtmf);
					//m_flowtable
					jsonite++;
				}
			}


			m_flowtable[Uniqueid]=flownode;
			flownodeite++;
		}
	}

}
t_ivrnode* Managerivr::Getnodeinfo(int companyid,int currentnode,string dtmfnum)
{
    map<int,map<int,t_ivrnode> >::iterator ite=m_IVRnodetable.find(companyid);
    if(ite==m_IVRnodetable.end())
    {
        cout<<"can not find the nodetable"<<endl;
        return NULL;
    }
    else
    {
        map<int,t_ivrnode> ptable=m_IVRnodetable[companyid];
        map<int,t_ivrnode>::iterator itenode = ptable.find(currentnode);
        if(itenode==ptable.end())
        {
            cout<<"can not find the current node:"<<currentnode<<endl;
            return NULL;
        }
        else
        {
            char uniqueid[16]={0};
			//if(currentnode==)
            sprintf(uniqueid,"%d_%d",companyid,currentnode);
			printf("Getnodeinfo:uniqueid  %s\n",uniqueid);
            if(dtmfnum==IVR_ANS_DFTM) //起始节点
            {
                return m_flowtable[uniqueid].current;
            }
            else
            {
                map<string,t_ivrnode*>pnextflow = m_flowtable[uniqueid].next;
                if(dtmfnum=="#")
                    return pnextflow[""];
                else
                {
                    map<string,t_ivrnode*>::iterator pnextflowite = pnextflow.find(dtmfnum);
                    if(pnextflowite!=pnextflow.end())
                    {
						 printf("return flow node unique:%d_%d\n",pnextflow[dtmfnum]->company_id,pnextflow[dtmfnum]->node_id);
						 return pnextflow[dtmfnum];
                    }
                    else
                    {
                       // cout<<"do not find the dtmfnum"<<
                        printf("currentid:%d,do not find the dtmfnum:%s\n",currentnode,dtmfnum.c_str());
                        return NULL;
                    }
                }

            }
        }

    }
}