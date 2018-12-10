﻿#ifndef DYTASKMAKER_H
#define DYTASKMAKER_H

#include "../taskmaker.h"
#include "../qyhbuffer.h"

class TcpClient;

class DyTaskMaker : public TaskMaker
{
public:
    DyTaskMaker(std::string _ip, int _port);

    void init();

    void makeTask(ClientSessionPtr conn, const Json::Value &request);

    bool makeChargeTask(int agv);

//    void makeTask(std::string from ,std::string to,std::string dispatch_id,int ceid,std::string line_id, int agv_id, int all_floor_info);
    void finishTask(std::string store_no, std::string storage_no, int type, std::string key_part_no, int agv_id);

    void queryTodoTaskCount();

    int getTodoTaskCount(){return todoTaskCount;}
private:
    void onRead(const char *data, int len);
    void onConnect();
    void onDisconnect();
    void receiveTask(std::string str_task);
    int msgProcess();
    TcpClient *m_wms_tcpClient;
    std::string m_ip;
    int m_port;
    bool m_connectState;
    QyhBuffer buffer;
    std::atomic_int todoTaskCount;
};

#endif // DYTASKMAKER_H
