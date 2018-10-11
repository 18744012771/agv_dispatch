#include "newelevator.h"
#include "newelevatormanager.h"
#include "common.h"
NewElevator::NewElevator(int _id, std::string _name, std::string _ip, int _port, bool _enabled):
    tcp(nullptr),
    occuAgvId(0),
    isEnabled(_enabled),
    isConnected(false),
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    firstConnect(true)
{

}
NewElevator::~NewElevator()
{
    if(tcp!=nullptr)delete tcp;
}
void NewElevator::init()
{
    TcpClient::ClientReadCallback onread = std::bind(&NewElevator::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::ClientConnectCallback onconnect = std::bind(&NewElevator::onConnect, this);
    TcpClient::ClientDisconnectCallback ondisconnect = std::bind(&NewElevator::onDisconnect, this);
    tcp = new TcpClient(ip, port, onread, onconnect, ondisconnect);
    tcp->start();
}

bool NewElevator::send(const lynx::elevator::Param &param)
{
    return send((char *)param.serialize().data(),param.serialize().size());
}

void NewElevator::StopSendThread()
{
    send_cmd = false;
}

void NewElevator::StartSendThread(int cmd, int from_floor, int to_floor, int elevator_id, int agv_id)
{
    send_cmd = true;

    std::thread t = std::thread([&,cmd,from_floor,to_floor,elevator_id,agv_id](){
        auto elemanagerptr = NewElevatorManager::getInstance();
        do
        {
            combined_logger->info("Elevator 发送CMD :{0}", cmd);

            auto p = elemanagerptr->create_param(cmd, from_floor, to_floor, elevator_id, agv_id);
            if(cmd == lynx::elevator::TakeEleACK)
            {
                combined_logger->info("AGV发送乘梯应答....", cmd);
            }
            else if(cmd == lynx::elevator::LeftEleCMD)
            {
                combined_logger->info("AGV发送离开指令....", cmd);
            }

            elemanagerptr->notify(p);
            sleep(5);
            combined_logger->info("AGV发送CMD end", cmd);
        }
        while(send_cmd);
    });

    t.detach();
}

bool NewElevator::send(const char *data,int len)
{
    if(tcp==nullptr)return false;
    combined_logger->info("send to Elevator{0} name{1}: {2}",id,
                          name,toHexString(data,len));
    return tcp->sendToServer(data,len);
}
void NewElevator::onRead(const char *data,int len)
{
    NewElevatorManagerPtr p = NewElevatorManager::getInstance();
    p->onRead(id,name,data,len);
}
void NewElevator::onConnect()
{
    if(firstConnect){
        //TODO:reset elevator
        NewElevatorManagerPtr p = NewElevatorManager::getInstance();
        p->resetElevatorState(this->getId());
        firstConnect = false;
    }
    isConnected = true;
}
void NewElevator::onDisconnect()
{
    isConnected = false;
}
