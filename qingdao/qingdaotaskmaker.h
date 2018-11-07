#pragma once
#include "../taskmaker.h"
class QingdaoTaskMaker : public TaskMaker
{
public:
	QingdaoTaskMaker();
	virtual ~QingdaoTaskMaker();
	virtual void init();
	virtual void makeTask(ClientSessionPtr conn, const Json::Value &request);
};

