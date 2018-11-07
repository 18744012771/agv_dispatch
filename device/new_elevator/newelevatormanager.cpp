#include <iostream>
#include "newelevatormanager.h"
#include "../../common.h"
#include "../../agvmanager.h"
#include "../../agv.h"

NewElevatorManager::NewElevatorManager()
{

}

NewElevatorManager::~NewElevatorManager()
{

}

void NewElevatorManager::KeepOpen(int floor, int elevator_id)
{
    for (auto ele : eles) {
        if (ele->getId() == elevator_id) {
            ele->KeepOpen(floor);
        }
    }
}

void NewElevatorManager::DropOpen(int elevator_id)
{
    for (auto ele : eles) {
        if (ele->getId() == elevator_id) {
            ele->DropOpen();
        }
    }
}

void NewElevatorManager::call(int ele_id, int floor)
{
    unsigned char data[8];
    getCmdData(floor, ele_id, CallEleENQ, data);
    send((char *)data, 8);
}

int NewElevatorManager::getOpenFloor(int ele_id)
{
    int r = -1;
    for (auto ele : eles) {
        if (ele->getId() == ele_id)
            return ele->getCurrentOpenDoorFloor();
    }

    return -1;
}


bool NewElevatorManager::init()
{
    checkTable();
    try {
        CppSQLite3Table table_ele = g_db.getTable("select id,name,ip,port,enabled from agv_elevator;");
        if (table_ele.numRows() > 0 && table_ele.numFields() != 5)
        {
            combined_logger->error("ElevatorManager init agv_elevator table error!");
            return false;
        }
        for (int row = 0; row < table_ele.numRows(); row++)
        {
            table_ele.setRow(row);

            if (table_ele.fieldIsNull(4))
            {
                combined_logger->error("Elevator {0} enabled field is null", row);
                continue;
            }
            if (table_ele.fieldIsNull(0) || table_ele.fieldIsNull(1) || table_ele.fieldIsNull(2) || table_ele.fieldIsNull(3))
                return false;

            int id = atoi(table_ele.fieldValue(0));
            std::string name = std::string(table_ele.fieldValue(1));
            std::string ip = std::string(table_ele.fieldValue(2));
            int port = atoi(table_ele.fieldValue(3));
            bool ele_enabled = true;
            if (!table_ele.fieldIsNull(4))
                ele_enabled = atoi(table_ele.fieldValue(4)) == 1;

            NewElevator *ele = new NewElevator(id, name, ip, port, ele_enabled);
            ele->init();
            eles.push_back(ele);
        }
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        return false;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}", e.what());
        return false;
    }
    return true;
}

void NewElevatorManager::setEnable(int id, bool enable)
{
    auto ele = getEleById(id);
    if (ele != nullptr) {
        ele->setIsEnabled(enable);

        char buf[SQL_MAX_LENGTH];
        snprintf(buf, SQL_MAX_LENGTH, "update agv_elevator set enabled=%d where id = %d;", enable ? 1 : 0, id);
        try {
            g_db.execDML(buf);
        }
        catch (CppSQLite3Exception e) {
            combined_logger->error("update ele enable sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        }
        catch (std::exception e) {
            combined_logger->error("update ele enable sqlerr code:{0}", e.what());
        }
    }
}

void NewElevatorManager::checkTable()
{
    //检查表
    try {
        if (!g_db.tableExists("agv_elevator")) {
            g_db.execDML("create table agv_elevator(id INTEGER primary key AUTOINCREMENT, name char(64),ip char(64),port INTEGER,enabled BOOL);");
        }
    }
    catch (CppSQLite3Exception e) {
        combined_logger->error("sqlerr code:{0} msg:{1}", e.errorCode(), e.errorMessage());
        return;
    }
    catch (std::exception e) {
        combined_logger->error("sqlerr code:{0}", e.what());
        return;
    }
}

void NewElevatorManager::send(const char *data, int len)
{
    for (auto ele : eles) {
        ele->send(data, len);
    }
}

void NewElevatorManager::queryElevatorState(int elevator_id)
{
    unsigned char data[8];
    getCmdData(0, elevator_id, StaEleENQ, data);
    send((char *)data, 8);
}

void NewElevatorManager::resetElevatorState(int elevator_id)
{
    unsigned char data[8];
    getCmdData(0, elevator_id, InitEleENQ, data);
    for (auto ele : eles) {
        if (ele->getId() == elevator_id) {
            ele->setResetOk(false);
            ele->setCurrentOpenDoorFloor(-1);
        }
    }
    send((char *)data, 8);
}
void NewElevatorManager::processReadMsg(const char *data, int len)
{
    if (len != 8)return;
    if (data[0] != (char)0xAA)return;
    if (data[1] != (char)0x55)return;

    combined_logger->debug("process read msg========{0}",toHexString(data,len));

    if (data[4] == (char)TakeEleENQ && data[3] == (char)0x01) {
        int eleid = data[5];
        int openfloor = data[2];
        for (auto ele : eles) {
            if (ele->getId() == eleid)ele->setCurrentOpenDoorFloor(openfloor);
        }
    }
    else if (data[4] == (char)StaEleACK && data[3] == (char)0x01) {
        int eleid = data[5];
        int openfloor = data[2];
        for (auto ele : eles) {
            if (ele->getId() == eleid)ele->setCurrentOpenDoorFloor(openfloor);
        }
    }

    else if (data[4] == (char)InitEleACK) {
        int eleid = data[5];
        for (auto ele : eles) {
            if (ele->getId() == eleid)ele->setResetOk(true);
        }
    }
}

void NewElevatorManager::onRead(int ele_id, std::string ele_name, const char *data, int len)
{
    //combined_logger->info("ele:{0};name:{1};read:{2}", ele_id, ele_name, toHexString(data, len));

    buffers[ele_id].append(data, len);

    while (!g_quit && buffers[ele_id].size() >= 8) {
        char head[2] = { (char)0xAA,(char)0x55 };
        int index = buffers[ele_id].indexof(head, 2);
        if (index < 0) {
            //未找到头，丢弃数据
            buffers[ele_id].clear();
        }
        else {
            //丢弃前面不用的数据
            buffers[ele_id].removeFront(index);
            if (buffers[ele_id].size() < 8) {
                //找到包头，却不完整
                return;
            }
            //处理
            processReadMsg(buffers[ele_id].data(index), 8);

            buffers[ele_id].removeFront(index + 8);
        }
    }
}

NewElevator *NewElevatorManager::getEleById(int id)
{
    for (auto ele : eles) {
        if (ele->getId() == id)
            return ele;
    }
    return nullptr;
}

void NewElevatorManager::ttest(int agv_id, int from_floor, int to_floor, int elevator_id)
{

    if (elevator_id == 1)elevator_id = 11;
    else elevator_id = 10;
    auto agv = AgvManager::getInstance()->getAgvById(agv_id);
    if (agv == nullptr) {
        combined_logger->error("agv id not exist!!!!!");
        return;
    }
    //根据电梯等待区设置呼叫某个ID电梯
    NewElevatorManagerPtr elemanagerptr = NewElevatorManager::getInstance();
    NewElevator *elevator = elemanagerptr->getEleById(elevator_id);

    if (elevator == nullptr)
    {
        combined_logger->error("DyForklift,  excutePath, no elevator!!!!!");
        return;
    }

    if (elevator->getOccuAgvId() > 0 && elevator->getOccuAgvId() != agv_id) {
        combined_logger->error("DyForklift, excutePath,elevator  occu!!!!!");
        return;
    }

    elevator->setOccuAgvId(agv_id);

    elevator->setResetOk(false);

    while(!g_quit){
        elemanagerptr->resetElevatorState(elevator_id);

        sleep(3);

        if (elevator->getResetOk())
            break;
    }

    combined_logger->info("DyForklift,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);

    //呼梯
    elevator->setCurrentOpenDoorFloor(-1);
    while (!g_quit) {
        sleep(1);
        elemanagerptr->call(elevator_id, from_floor);
        sleep(1);
        elemanagerptr->queryElevatorState(elevator_id);
        if (elevator->getCurrentOpenDoorFloor() == from_floor)break;
    }

    //进电梯
    combined_logger->debug("==============START go into elevator");
    elemanagerptr->KeepOpen(from_floor,elevator_id);
    sleep(5);
    sleep(10);
    combined_logger->debug("==============FINISH go into elevator");
    //关门
    elemanagerptr->DropOpen(elevator_id);
    sleep(1);
    elemanagerptr->resetElevatorState(elevator_id);
    sleep(5);
    //呼梯
    elemanagerptr->call(elevator_id, to_floor);

    while (!g_quit) {
        sleep(1);
        if (elevator->getCurrentOpenDoorFloor() == to_floor)break;
        sleep(1);
        //重新呼叫
        elemanagerptr->call(elevator_id, to_floor);
    }

    //出电梯
    combined_logger->debug("==============START go out elevator");
    elemanagerptr->KeepOpen(to_floor, elevator_id);
    sleep(5);
    sleep(10);
    combined_logger->debug("==============FINISH go out elevator");
    //关门
    elemanagerptr->DropOpen(elevator_id);
    sleep(1);
    elemanagerptr->resetElevatorState(elevator_id);

    
    elevator->setOccuAgvId(0);

    combined_logger->info("excute path finish");
}

void NewElevatorManager::interSetEnableELE(ClientSessionPtr conn, const Json::Value &request)
{
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;

    int id = request["id"].asInt();
    bool enable = request["enable"].asBool();
    setEnable(id, enable);
    conn->send(response);
}

void NewElevatorManager::test()
{
    while (!g_quit)
    {
        int agv_id;
        int ele_id;
        int from_floor;
        int to_floor;

        while (!g_quit) {
            std::cout << "tell me the agv_id:(1/2/3)" << std::endl;
            std::cin >> agv_id;
            if (agv_id == 1 || agv_id == 2 || agv_id == 3)break;
        }


        while (!g_quit) {
            std::cout << "tell me the elevator id:(1/2)" << std::endl;
            std::cin >> ele_id;
            if (ele_id == 1 || ele_id == 2)break;
        }


        while (!g_quit) {
            std::cout << "tell me the from_floor:(1/2/3)" << std::endl;
            std::cin >> from_floor;
            if (from_floor == 1 || from_floor == 2 || from_floor == 3)break;
        }


        while (!g_quit) {
            std::cout << "tell me the to_floor:(1/2/3)" << std::endl;
            std::cin >> to_floor;
            if (to_floor == 1 || to_floor == 2 || to_floor == 3)break;
        }

        std::cout << "Sure(Y/N)?agvid:" << agv_id << " ele_id:" << ele_id << " from_floor:" << from_floor << " to_floor:" << to_floor;
        std::string str;
        std::cin >> str;
        if (str == "y" || str == "Y") {
            ttest(agv_id, from_floor, to_floor, ele_id);
        }
    }
}

void NewElevatorManager::getCmdData(int floor, int ele_id, int cmd, unsigned char(&data)[8])
{
    //head
    data[0] = 0xAA;
    data[1] = 0x55;

    //from floor
    data[2] = 0x00;

    //to floor
    data[3] = floor & 0xFF;

    //cmd
    data[4] = cmd & 0xFF;

    //elevator_id
    data[5] = ele_id & 0xFF;

    //robot_id
    data[6] = 0x00;

    //check sum
    data[7] = std::accumulate(data, data + 7, 0) & 0xFF;

}


