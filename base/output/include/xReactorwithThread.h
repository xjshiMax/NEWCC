//2019/1/6 带线程的reactor 
/*
1） 在些实际应用的时候发现，如果reactor不带线程，那么每次在外部调用start都会阻塞。
2） 给复用加一个自己的工作线程，start（）以后，主线程仍然能正常执行，
*/
#pragma once
#include <stdio.h>
#include "xthreadbase.h"
#include "xReactor.h"
namespace SEABASE{
class xReactorwithThread:public xReactor,public Threadbase
{
public:
	virtual void run()
	{
		xReactor::start();
	}
	xReactorwithThread(){}
	~xReactorwithThread()
	{
		destory();
		printf("~xReactorwithThread\n");
	}
	void startReactorWithThread()
	{
		Threadbase::start();
	}
};
}