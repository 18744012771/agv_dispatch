#include "dyforkliftthingcharge.h"
#include <cassert>
#include "../common.h"
#include "dyforklift.h"

DyForkliftThingCharge::DyForkliftThingCharge(std::vector<std::string> _params):
    AgvTaskNodeDoThing(_params)
{
    if(params.size()>=3){
        charge_id = stringToInt(params[0]);
        ip = params[1];
        port = stringToInt(params[2]);
        cm.setParams(charge_id, "charge_machine", ip, port,COMM_TIMEOUT);
    }
}

void DyForkliftThingCharge::beforeDoing(AgvPtr agv)
{
    bresult = false;
    //充电桩初始化
    if(cm.init())
    {
        cm.start();
    }
    else
    {
        combined_logger->error("init chargemachine error");
    }
}

void DyForkliftThingCharge::doing(AgvPtr agv)
{
    if(!cm.checkConnection())
        return;

    //auto task  = agv->getTask();
    if(agv->getTask() == nullptr){
        combined_logger->debug("no charge task found!");
        return;
    }

    if(agv->type() != DyForklift::Type){
        bresult = true;
        return;
    }
    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);


    //panduan wendu
    while(forklift->getTask()!=nullptr && !forklift->getTask()->getIsCancel()){
        if(forklift->getBatteryTemprature()>0){
            break;
        }
    }

    //叉尺抬升
    combined_logger->info("charge fork up start");
    bool sendResult = forklift->fork(FORKLIFT_UP);

    if(!sendResult)
    {
        combined_logger->info("send fork up error");
        return;
    }
    do
    {
        sleep_for_s(1);
    }while(!forklift->isFinish(FORKLIFT_FORK)&&agv->getTask()!=nullptr && !agv->getTask()->getIsCancel());

    if(agv->getTask()==nullptr || agv->getTask()->getIsCancel()){
        combined_logger->debug("charge task is cancel!");
        return ;
    }

    combined_logger->info("charge fork up end");
    //发送给小车充电
    sendResult = forklift->charge(FORKLIFT_START_CHARGE);
    do
    {
        sleep_for_s(1);
    }while(!forklift->isFinish(FORKLIFT_CHARGE)&&agv->getTask()!=nullptr && !agv->getTask()->getIsCancel());

    if(agv->getTask()==nullptr || agv->getTask()->getIsCancel()){
        combined_logger->debug("charge task is cancel!");
        return ;
    }

    combined_logger->info("dothings-charge start");

    //开始充电
    int retry = 20;
    do
    {
        cm.chargeControl(charge_id, CHARGE_START);
        retry--;
        sleep_for_s(2);
    }while(CHARGE_ING != cm.getStatus()&& retry>0&&agv->getTask()!=nullptr && !agv->getTask()->getIsCancel());

    while(CHARGE_ING == cm.getStatus()&&agv->getTask()!=nullptr && !agv->getTask()->getIsCancel())
    {
        sleep_for_s(2);
        cm.chargeControl(charge_id, CHARGE_START);
    }

}


void DyForkliftThingCharge::afterDoing(AgvPtr agv)
{
    if(agv->type() != DyForklift::Type){
        //other agv,can not excute dy forklift charge
        bresult = true;
        return ;
    }

    //通知充电桩停止充电
    //TODO 低温保护需退出
    cm.chargeControl(charge_id, CHARGE_STOP);
    do
    {
        sleep_for_s(1);
        cm.chargeControl(charge_id, CHARGE_STOP);
    }while(CHARGE_IDLE != cm.getStatus());


    //通知小车退出充电
    DyForkliftPtr forklift = std::static_pointer_cast<DyForklift>(agv);

    bool sendResult = forklift->charge(FORKLIFT_END_CHARGE);
    do
    {
        sleep_for_s(1);
    }while(!forklift->isFinish(FORKLIFT_CHARGE));


    //叉尺下降
    combined_logger->info("charge fork down start");
    sendResult = forklift->fork(FORKLIFT_DOWN);

    if(!sendResult)
    {
        combined_logger->info("send fork down error");
        return;
    }
    do
    {
        sleep_for_s(1);
    }while(!forklift->isFinish(FORKLIFT_FORK));

    combined_logger->info("charge fork down end");
    combined_logger->info("dothings-charge end");
    bresult = true;
}

bool DyForkliftThingCharge::result(AgvPtr agv)
{
    //    Q_UNUSED(agv)
    return bresult;
}
