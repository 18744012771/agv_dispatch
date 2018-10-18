#ifndef NEWELEVATOR_H
#define NEWELEVATOR_H
#include "../../network/tcpclient.h"

class NewElevator
{
public:
    NewElevator(int _id,std::string _name,std::string _ip,int _port,bool _enabled);
    ~NewElevator();

    void init();
    bool send(const char *data,int len);
    
    void KeepOpen(int floor);

    void DropOpen();
         
    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();

    int getId(){return id;}

    bool getIsEnabled(){return isEnabled;}
    void setIsEnabled(bool _isEnabled){isEnabled = _isEnabled;}

    void setOccuAgvId(int _occuAgvId){occuAgvId = _occuAgvId;}
    int getOccuAgvId(){return occuAgvId;}

    bool getIsConnected(){return isConnected;}

    int getCurrentOpenDoorFloor(){return currentOpenDoorFloor;}

    void setCurrentOpenDoorFloor(int _currentOpenDoorFloor){ currentOpenDoorFloor = _currentOpenDoorFloor;}
    
    void setResetOk(bool _ok) { resetOk = _ok; }

    bool getResetOk() { return resetOk; }

private:
    int id;
    std::string name;
    std::string ip;
    int port;
    bool isConnected;
    bool isEnabled;
    int occuAgvId;
    TcpClient *tcp;

    std::atomic_int currentOpenDoorFloor;//电梯当前在哪层楼开门状态
    std::atomic_bool resetOk;//复位成功与否


    bool send_cmd;
    bool firstConnect;
};

#endif // NEWELEVATOR_H
