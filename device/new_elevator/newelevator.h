#ifndef NEWELEVATOR_H
#define NEWELEVATOR_H
#include "../../network/tcpclient.h"
#include "../elevator/elevator_protocol.h"

class NewElevator
{
public:
    NewElevator(int _id,std::string _name,std::string _ip,int _port,bool _enabled);
    ~NewElevator();
    void init();
    bool send(const char *data,int len);
    bool send(const lynx::elevator::Param &param);
    void StartSendThread(int cmd, int from_floor, int to_floor, int elevator_id, int agv_id);
    void StopSendThread();


    virtual void onRead(const char *data,int len);
    virtual void onConnect();
    virtual void onDisconnect();

    int getId(){return id;}

    bool getIsEnabled(){return isEnabled;}
    void setIsEnabled(bool _isEnabled){isEnabled = _isEnabled;}

    void setOccuAgvId(int _occuAgvId){occuAgvId = _occuAgvId;}
    int getOccuAgvId(){return occuAgvId;}

    bool getIsConnected(){return isConnected;}

    bool getCallEleACK(){return CallEleACK;}
    bool getIntoEleSet(){return IntoEleSet;}
    bool getLeftCMDSet(){return LeftCMDSet;}
    bool getLeftEleSet(){return LeftEleSet;}
    bool getLeftEleENQ(){return LeftEleENQ;}
    bool getTakeEleENQ(){return TakeEleENQ;}
    bool getIntoEleENQ(){return IntoEleENQ;}

    void setCallEleACK(bool _CallEleACK){CallEleACK = _CallEleACK;}
    void setIntoEleSet(bool _IntoEleSet){IntoEleSet = _IntoEleSet;}
    void setLeftCMDSet(bool _LeftCMDSet){LeftCMDSet = _LeftCMDSet;}
    void setLeftEleSet(bool _LeftEleSet){LeftEleSet = _LeftEleSet;}
    void setLeftEleENQ(bool _LeftEleENQ){LeftEleENQ = _LeftEleENQ;}
    void setTakeEleENQ(bool _TakeEleENQ){TakeEleENQ = _TakeEleENQ;}
    void setIntoEleENQ(bool _IntoEleENQ){IntoEleENQ = _IntoEleENQ;}

    void resetFlags(){
        CallEleACK = false;
        IntoEleSet = false;
        LeftCMDSet = false;
        LeftEleSet = false;
        LeftEleENQ = false;
        TakeEleENQ = false;
        IntoEleENQ = false;
    }


private:
    int id;
    std::string name;
    std::string ip;
    int port;
    bool isConnected;
    bool isEnabled;
    int occuAgvId;
    TcpClient *tcp;

    std::atomic_bool CallEleACK;//【呼梯应答】
    std::atomic_bool IntoEleSet;
    std::atomic_bool LeftCMDSet;
    std::atomic_bool LeftEleSet;
    std::atomic_bool LeftEleENQ;
    std::atomic_bool TakeEleENQ;
    std::atomic_bool IntoEleENQ;

    bool send_cmd;
    bool firstConnect;
};

#endif // NEWELEVATOR_H
