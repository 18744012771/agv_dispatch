﻿#ifndef ATFORKLIFT_H
#define ATFORKLIFT_H

#include "../agv.h"
#include "../agvtask.h"
#include "../network/sessionmanager.h"
#include "../network/agvsession.h"

#define AT_PRECISION 60
#define AT_MOVE_HEIGHT 400
//距离起点AT_PRECMD_RANGE开始抬升至MOVE_HEIGHT高度　距离终点AT_PRECMD_RANGE时调整至最后动作附近高度
#define AT_PRECMD_RANGE 800
#define AT_START_RANGE 200
#define MAX_WAITTIMES 10
class AtForklift;
using AtForkliftPtr = std::shared_ptr<AtForklift>;

enum ATFORKLIFT_COMM
{
    ATFORKLIFT_HEART = 9,
    ATFORKLIFT_STARTREPORT = 21,
    ATFORKLIFT_ENDREPORT = 22,
    ATFORKLIFT_BATTERY = 25,
    ATFORKLIFT_FINISH = 26,
    ATFORKLIFT_POS = 29,
    ATFORKLIFT_TURN = 30,
    ATFORKLIFT_MOVE = 35,
    ATFORKLIFT_MOVE_NOLASER = 37,
    ATFORKLIFT_FORK_ADJUST = 38,
    ATFORKLIFT_FORK_LIFT = 67,
    ATFORKLIFT_FORK_ADJUSTALL = 68
};

enum ATFORKLIFT_FORKPARAMS
{
    ATFORKLIFT_ADJUST_STRENCH = 1,
    ATFORKLIFT_ADJUST_RETRACT = 2,
    ATFORKLIFT_ADJUST_UP = 3,
    ATFORKLIFT_ADJUST_DOWN = 4
};

enum ATFORKLIFT_FORKALLPARAMS
{
    ATFORKLIFT_ADJUSTALL_UP = 1,
    ATFORKLIFT_ADJUSTALL_DOWN = 2
};
class AtForklift : public Agv
{
public:
    AtForklift(int id,std::string name,std::string ip,int port);

    enum { Type = Agv::Type+3 };

    int type(){return Type;}

    void onRead(const char *data,int len);

    void excutePath(std::vector<int> lines);
    void goStation(std::vector<int> lines,  bool stop);

    void setQyhTcp(AgvSessionPtr _qyhTcp);

    bool startReport(int interval);
    bool endReport();
    bool turn(float speed, float angle);    //车辆旋转//返回发送是否成功
    bool waitTurnEnd(int waitMs);    //等待旋转结束，如果接收失败，返回false。返回接受的结果
    bool fork(int params); //1-liftup 0-setdown
    bool forkAdjust(int params);
    bool forkAdjustAll(int params, int final_height);
    bool move(float speed, float distance);//进电梯时用
    bool heart();
    bool isFinish();
    bool isFinish(int cmd_type);

    Pose4D getPos();
    int nearestStation(int x, int y, int a, int floor);
    void arrve(int x,int y);

    void onTaskStart(AgvTaskPtr _task);
    void onTaskFinished(AgvTaskPtr _task);

    ~AtForklift(){}
    virtual bool pause();
    virtual bool resume();
private:
    static const int maxResendTime = 10;

    bool resend(const std::string &msg);
    bool send(const std::string &msg);

    void init();
    bool turnResult;

    Pose4D m_currentPos;
    int m_power;
    int m_lift_height;

    int m_fix_x;
    int m_fix_y;
    int m_fix_a;

    std::map<int,  DyMsg> m_unRecvSend;
    std::map<int,  DyMsg> m_unFinishCmd;
    std::mutex msgMtx;

    int actionpoint, startpoint;
    bool actionFlag = false;
    bool startLift = false;
    int task_type;

    AgvSessionPtr m_qTcp;

    bool pausedFlag;//是否暂停了
    bool sendPause;//发送的是暂停的指令，还是继续的指令
};

#endif // ATFORKLIFT_H
