#include "qingdaonodetingput.h"
#include "../common.h"


QingdaoNodeTingPut::QingdaoNodeTingPut(std::vector<std::string> _params):
	AgvTaskNodeDoThing(_params)
{
}


QingdaoNodeTingPut::~QingdaoNodeTingPut()
{
}

void QingdaoNodeTingPut::beforeDoing(AgvPtr agv)
{

}

void QingdaoNodeTingPut::doing(AgvPtr agv)
{
	//���� 

    sleep_for_ms(15000);
	bresult = true;
}


void QingdaoNodeTingPut::afterDoing(AgvPtr agv)
{
}

bool QingdaoNodeTingPut::result(AgvPtr agv)
{
	return bresult;
}
