#include "newelevator.h"
#include "newelevatormanager.h"
#include "../../common.h"
NewElevator::NewElevator(int _id, std::string _name, std::string _ip, int _port, bool _enabled):
    tcp(nullptr),
    occuAgvId(0),
    isEnabled(_enabled),
    isConnected(false),
    id(_id),
    name(_name),
    ip(_ip),
    port(_port),
    firstConnect(true),
    currentOpenDoorFloor(-1),
    resetOk(false)
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

void NewElevator::KeepOpen(int floor)
{
    send_cmd = true;

    g_threads.create_thread([&, floor]() {
        auto elemanagerptr = NewElevatorManager::getInstance();
        unsigned char data[8];
        elemanagerptr->getCmdData(floor, getId(), CallEleENQ, data);

        do
        {
            elemanagerptr->send((char *)data,8);
            sleep(2);
        } while (send_cmd);
    })->detach();
}

void NewElevator::DropOpen()
{
    send_cmd = false;
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

bool NewElevator::isCurrentOpenDoor(int floor)
{
    //查询电梯


}
