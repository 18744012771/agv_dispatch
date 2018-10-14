#include "newelevatormanager.h"
#include "../../common.h"
#include "../elevator/elevator_protocol.h"
#include "../../agvmanager.h"
#include "../../agv.h"

NewElevatorManager::NewElevatorManager()
{

}

NewElevatorManager::~NewElevatorManager()
{

}

bool NewElevatorManager::init()
{
    checkTable();
    try{
        CppSQLite3Table table_ele = g_db.getTable("select id,name,ip,port,enabled from agv_elevator;");
        if(table_ele.numRows()>0 && table_ele.numFields()!=5)
        {
            combined_logger->error("ElevatorManager init agv_elevator table error!");
            return false;
        }
        for (int row = 0; row < table_ele.numRows(); row++)
        {
            table_ele.setRow(row);

            if(table_ele.fieldIsNull(4))
            {
                combined_logger->error("Elevator {0} enabled field is null", row);
                continue;
            }
            if(table_ele.fieldIsNull(0) ||table_ele.fieldIsNull(1) ||table_ele.fieldIsNull(2)||table_ele.fieldIsNull(3))
                return false;

            int id = atoi(table_ele.fieldValue(0));
            std::string name = std::string(table_ele.fieldValue(1));
            std::string ip = std::string(table_ele.fieldValue(2));
            int port = atoi(table_ele.fieldValue(3));
            bool ele_enabled=true;
            if(!table_ele.fieldIsNull(4))
                ele_enabled = atoi(table_ele.fieldValue(4)) == 1;

            NewElevator *ele = new NewElevator(id,name,ip,port,ele_enabled);
            ele->init();
            eles.push_back(ele);
        }
    }catch(CppSQLite3Exception e){
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return false;
    }catch(std::exception e){
        combined_logger->error("sqlerr code:{0}",e.what());
        return false;
    }
    return true;
}

void NewElevatorManager::checkTable()
{
    //检查表
    try{
        if(!g_db.tableExists("agv_elevator")){
            g_db.execDML("create table agv_elevator(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER,enabled BOOL);");
        }
    }catch(CppSQLite3Exception e){
        combined_logger->error("sqlerr code:{0} msg:{1}",e.errorCode(),e.errorMessage());
        return ;
    }catch(std::exception e){
        combined_logger->error("sqlerr code:{0}",e.what());
        return ;
    }
}

lynx::elevator::Param NewElevatorManager::create_param(int cmd, int from_floor,int to_floor, int elevator_id, int agv_id)
{
    using namespace lynx::elevator;
    return lynx::elevator::Param {
        (Param::byte_t) from_floor,
                (Param::byte_t) to_floor,
                (CMD)           cmd,
                (Param::byte_t) elevator_id,
                (Param::byte_t) agv_id,
    };
}

void NewElevatorManager::resetElevatorState(int elevator_id)
{
    auto p = create_param(lynx::elevator::InitEleENQ, 0x00, 0x00, elevator_id, 0x00);
    notify(p);
    std::cout << "复位电梯状态...." << std::endl;
    return ;
}

void NewElevatorManager::onRead(int ele_id, std::string ele_name, const char *data, int len)
{
    combined_logger->info("ele:{0};name:{1};read:{2}",ele_id,ele_name,toHexString(data,len));


    std::string err;
    using namespace lynx::elevator;

    int templen = len;

    while(templen>=8){
        const char *temp = data+len - templen;
        auto p = Param::parse((Param::byte_t *)temp,templen,err);
        if(!err.empty())break;
        auto eleptr = getEleById(p.elevator_no);
        auto agv = AgvManager::getInstance()->getAgvById(p.robot_no);

        if(agv != nullptr && eleptr!=nullptr && eleptr->getOccuAgvId() == p.robot_no){
            if(p.cmd == TakeEleENQ && p.src_floor == agv->getToFloor()){
                eleptr->setTakeEleENQ(true);
            }else if(p.src_floor == agv->getFromFloor() && p.dst_floor == agv->getToFloor()){
                if(p.cmd == CallEleACK)eleptr->setCallEleACK(true);
                else if(p.cmd == IntoEleSet)eleptr->setIntoEleSet(true);
                else if(p.cmd == LeftCMDSet)eleptr->setLeftCMDSet(true);
                else if(p.cmd == LeftEleSet)eleptr->setLeftEleSet(true);
                else if(p.cmd == LeftEleENQ)eleptr->setLeftEleENQ(true);
                else if(p.cmd == IntoEleENQ)eleptr->setIntoEleENQ(true);
            }

        }
//        if(agv!=nullptr && eleptr!=nullptr&& eleptr->getOccuAgvId() == p.robot_no && p.src_floor == agv->getFromFloor() && p.dst_floor == agv->getToFloor()){
//            if(p.cmd == CallEleACK)eleptr->setCallEleACK(true);
//            else if(p.cmd == IntoEleSet)eleptr->setIntoEleSet(true);
//            else if(p.cmd == LeftCMDSet)eleptr->setLeftCMDSet(true);
//            else if(p.cmd == LeftEleSet)eleptr->setLeftEleSet(true);
//            else if(p.cmd == LeftEleENQ)eleptr->setLeftEleENQ(true);
//            else if(p.cmd == TakeEleENQ)eleptr->setTakeEleENQ(true);
//            else if(p.cmd == IntoEleENQ)eleptr->setIntoEleENQ(true);
//        }
        temp+=8;
        templen -= 8;
    }

    //    std::string err;
    //    using namespace lynx::elevator;
    //    auto p = Param::parse((Param::byte_t *)data, len, err);

    //    if (err.empty()) {
    //        auto eleptr = getEleById(p.elevator_no);
    //        if(eleptr!=nullptr&& eleptr->getOccuAgvId() == p.robot_no){
    //            if(p.cmd == CallEleACK)eleptr->setCallEleACK(true);
    //            else if(p.cmd == IntoEleSet)eleptr->setIntoEleSet(true);
    //            else if(p.cmd == LeftCMDSet)eleptr->setLeftCMDSet(true);
    //            else if(p.cmd == LeftEleSet)eleptr->setLeftEleSet(true);
    //            else if(p.cmd == LeftEleENQ)eleptr->setLeftEleENQ(true);
    //            else if(p.cmd == TakeEleENQ)eleptr->setTakeEleENQ(true);
    //            else if(p.cmd == IntoEleENQ)eleptr->setIntoEleENQ(true);
    //        }
    //    }
}

// 通知不必有信息返回
void NewElevatorManager::notify(const lynx::elevator::Param& p)
{
    for(auto ele:eles)ele->send((char *)p.serialize().data(),p.serialize().size());
}


NewElevator *NewElevatorManager::getEleById(int id)
{
    for(auto ele:eles){
        if(ele->getId() == id)
            return ele;
    }
    return nullptr;
}

void NewElevatorManager::ttest(int agv_id, int from_floor, int to_floor, int elevator_id)
{

    if(elevator_id == 1)elevator_id = 11;
    else elevator_id = 10;
    auto agv = AgvManager::getInstance()->getAgvById(agv_id);
    if(agv == nullptr){
        combined_logger->error("agv id not exist!!!!!");
        return ;
    }
    //根据电梯等待区设置呼叫某个ID电梯
    NewElevatorManagerPtr elemanagerptr = NewElevatorManager::getInstance();
    NewElevator *elevator = elemanagerptr->getEleById(elevator_id);

    if(elevator == nullptr)
    {
        combined_logger->error("DyForklift,  excutePath, no elevator!!!!!");
        return;
    }

    if(elevator->getOccuAgvId()>0 && elevator->getOccuAgvId()!=agv_id){
        combined_logger->error("DyForklift, excutePath,elevator  occu!!!!!");
        return;
    }

    elevator->setOccuAgvId(agv_id);

    elemanagerptr->resetElevatorState(elevator_id);

    sleep(10);


    combined_logger->info("DyForklift,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);

    elevator->resetFlags();

    int needResendTimes = 0;

    agv->setFromFloor(0);
    agv->setToFloor(from_floor);

    auto p1 = elemanagerptr->create_param(lynx::elevator::CallEleENQ,0,from_floor,elevator_id,agv_id);
    //TODO: 【呼梯问询】 dispatch --> elevator
    elevator->send(p1);

    //TODO: 【呼梯应答】 elevator --> dispatch
    needResendTimes = 0;
    while(!g_quit){
        if(elevator->getCallEleACK()){
            break;
        }
        ++needResendTimes;
        if(needResendTimes>20)
        {
            //do not recv ack in 2 seconds,resend!
            elevator->send(p1);
            needResendTimes = 0;
        }
        usleep(100000);//100ms
    }

    //TODO: 【乘梯问询】 elevator -->  dispatch
    needResendTimes = 0;
    while(!g_quit )
    {
        if(elevator->getTakeEleENQ()){
            break;
        }
        ++needResendTimes;
        if(needResendTimes>400)
        {
            //do not recv task ele enqueue in 40 seconds,resend!
            elevator->send(p1);
            needResendTimes = 0;
        }
        usleep(100000);//100ms
    }

    if(g_quit) return ;

    //TODO: 【乘梯应答】 dispatch --> elevator


    agv->setFromFloor(from_floor);
    agv->setToFloor(to_floor);

    auto p2 = elemanagerptr->create_param(lynx::elevator::TakeEleACK, from_floor, to_floor, elevator_id, agv_id);
    elevator->send(p2);

    //TODO: 【进入指令】 elevator -->  dispatch
    needResendTimes = 0;
    while(!g_quit )
    {
        if(elevator->getIntoEleENQ()){
            break;
        }

        ++needResendTimes;
        if(needResendTimes>50)
        {
            //do not recv task ele enqueue in 5 seconds,resend!
            elevator->send(p2);
            needResendTimes = 0;
        }
        usleep(100000);//100ms
    }
    if(g_quit) return ;

    //TODO:5s一次 【乘梯应答】 dispatch --> elevator
    elevator->StartSendThread(lynx::elevator::TakeEleACK,from_floor, to_floor, elevator_id, agv_id);

    //TODO: agv go into elevator
    sleep(3); //等待电梯门完全打开

    //go into elevator
    combined_logger->info("AGV START GO INTO ELEVATOR");
    sleep(20);
    combined_logger->info("AGV FINISH GO INTO ELEVATOR");
    elevator->StopSendThread();

    if(g_quit) return ;

    //TODO:agv就位后 【进入电梯应答】 dispatch --> elevator
    auto p3 = elemanagerptr->create_param(lynx::elevator::IntoEleACK, from_floor,to_floor, elevator_id, agv_id);
    elevator->send(p3);
    ////TODO:【进入应答确认】 ignored
    /// TODO:【进入应答确认】elevator -->  dispatch
    //TODO:【到达指令】elevator -->  dispatch
    needResendTimes = 0;
    while(!g_quit )
    {
        if(elevator->getLeftEleENQ()){
            break;
        }

        ++needResendTimes;
        if(needResendTimes>300)
        {
            //do not recv task ele enqueue in 30 seconds,resend!
            elevator->send(p3);
            needResendTimes = 0;
        }
        usleep(100000);//100ms
    }
    if(g_quit) return ;

    //TODO: 每5s发送【离开指令】 dispatch --> elevator
    //TODO: 每次【离开指令确认】 elevator -->  dispatch
    //通知电梯AGV正在出电梯,离开过程中每5s发送一次【离开指令】，要求内呼等待机器人离开
    elevator->StartSendThread(lynx::elevator::LeftEleCMD,from_floor, to_floor, elevator->getId(), agv_id);

    sleep(5);//等待电梯门完全打开

    //moni AGV出电梯
    combined_logger->info("AGV START GO OUT ELEVATOR");
    sleep(20);
    combined_logger->info("AGV FINISH GO OUT ELEVATOR");

    elevator->StopSendThread();//stop send 离开指令


    sleep(2);

    //TODO:离开电梯后后,发送【离开应答】 dispatch --> elevator
    auto p4 = elemanagerptr->create_param(lynx::elevator::LeftEleACK, from_floor, to_floor, elevator_id, agv_id);
    elevator->send(p4);

    //TODO:【离开应答确认】elevator -->  dispatch
    needResendTimes = 0;
    while(!g_quit )
    {
        if(elevator->getLeftEleSet()){
            break;
        }

        ++needResendTimes;
        if(needResendTimes>50)
        {
            //do not recv task ele enqueue in 5 seconds,resend!
            break;
        }
        usleep(100000);//100ms
    }
    if(g_quit) return ;

    elevator->setOccuAgvId(0);

    {
        combined_logger->debug("DyForklift,  excutePath, 离开电梯口去目标");
    }

    agv->setFromFloor(-1);
    agv->setToFloor(-1);

    combined_logger->info("excute path finish");
}

void NewElevatorManager::test()
{
    while(!g_quit)
    {
        int agv_id;
        int ele_id;
        int from_floor;
        int to_floor;

        while(!g_quit){
            std::cout<<"tell me the agv_id:(1/2/3)"<<std::endl;
            std::cin>>agv_id;
            if(agv_id == 1||agv_id == 2||agv_id == 3)break;
        }


        while(!g_quit){
            std::cout<<"tell me the elevator id:(1/2)"<<std::endl;
            std::cin>>ele_id;
            if(ele_id == 1||ele_id == 2)break;
        }


        while(!g_quit){
            std::cout<<"tell me the from_floor:(1/2/3)"<<std::endl;
            std::cin>>from_floor;
            if(from_floor == 1||from_floor == 2||from_floor == 3)break;
        }


        while(!g_quit){
            std::cout<<"tell me the to_floor:(1/2/3)"<<std::endl;
            std::cin>>to_floor;
            if(to_floor == 1||to_floor == 2||to_floor == 3)break;
        }

        std::cout<<"Sure(Y/N)?agvid:"<<agv_id<<" ele_id:"<<ele_id<<" from_floor:"<<from_floor<<" to_floor:"<<to_floor;
        std::string str;
        std::cin>>str;
        if(str == "y" ||str == "Y"){
            ttest(agv_id,from_floor,to_floor,ele_id);
        }
    }
}


