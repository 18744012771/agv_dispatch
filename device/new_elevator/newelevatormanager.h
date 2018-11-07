#ifndef NEWELEVATORMANAGER_H
#define NEWELEVATORMANAGER_H


#include <vector>
#include <memory>
#include <mutex>
#include <numeric>
#include <boost/noncopyable.hpp>
#include "../../protocol.h"
#include "newelevator.h"
#include "../../network/clientsession.h"
#include "../../qyhbuffer.h"

class NewElevatorManager;
using NewElevatorManagerPtr = std::shared_ptr<NewElevatorManager>;

enum CMD {
    //TestEleCmd = 0x00,
    CallEleENQ = 0x01,
    //CallEleACK = 0x02,
    TakeEleENQ = 0x03,
    //TakeEleACK = 0x04,
    //IntoEleENQ = 0x05,
    //IntoEleACK = 0x06,
    //IntoEleSet = 0x0D,
    //LeftEleENQ = 0x07,
    //LeftEleCMD = 0x0B,
    //LeftCMDSet = 0x0E,
    //LeftEleACK = 0x08,
    //LeftEleSet = 0x0F,
    //FullEleENQ = 0x09,
    //FullEleACK = 0x0A,
    //FloorElErr = 0x0C,
    InitEleENQ = 0x10,
    InitEleACK = 0x11,
    StaEleENQ = 0x12,
    StaEleACK = 0x13,
    //EleLightON = 0x14,
    //EleLitOnSet = 0x15,
    //EleLightOFF = 0x16,
    //EleLitOffSet = 0x17,
    //ExComENQ = 0x20,
    //ExComACK = 0x21,
    //InComENQRobot = 0x22,
    //InComENQServ = 0x23,
    //PassEleACK = 0x1D,
    //PassEleSet = 0x1E,
    //AlignEleENQ = 0x24,
    //AlignEleACK = 0x25,
};

class NewElevatorManager : public boost::noncopyable,public std::enable_shared_from_this<NewElevatorManager>
{
public:

    bool init();

    static NewElevatorManagerPtr getInstance(){
        static NewElevatorManagerPtr m_ins = NewElevatorManagerPtr(new NewElevatorManager());
        return m_ins;
    }

    ~NewElevatorManager();

    void KeepOpen(int floor, int elevator_id);
    
    void DropOpen(int elevator_id);

    void call(int ele_id, int floor);

    int getOpenFloor(int ele_id);

    //重置电梯
    void resetElevatorState(int elevator_id);

    //chaxun dianti zhuangtai
    void queryElevatorState(int elevator_id);

    //电梯使能
    void setEnable(int id, bool enable);

    //接收到数据
    void onRead(int ele_id, std::string ele_name, const char *data, int len);

    void processReadMsg(const char *data, int len);

    //发送数据
    void send(const char *data, int len);

    NewElevator *getEleById(int id);

    void test();

	std::vector<NewElevator *> getAllEles() { return eles; }

    void interSetEnableELE(ClientSessionPtr conn, const Json::Value &request);

    void getCmdData(int floor, int ele_id, int cmd, unsigned char(&data)[8]);
private:

    void ttest(int agv_id,int from_floor,int to_floor,int elevator_id);

    NewElevatorManager();

    void checkTable();
    std::vector<NewElevator *> eles;

    std::map<int, QyhBuffer> buffers;
};

#endif // NEWELEVATORMANAGER_H
