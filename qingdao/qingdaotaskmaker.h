#pragma once
#include "../taskmaker.h"
#include "../qyhbuffer.h"

class TcpClient;

class QingdaoTaskMaker : public TaskMaker
{
public:
    QingdaoTaskMaker(std::string _ip, int _port);
	virtual ~QingdaoTaskMaker();
	virtual void init();
	virtual void makeTask(ClientSessionPtr conn, const Json::Value &request);

    bool makeChargeTask(int agv);

    void finishTask(std::string store_no, std::string storage_no, int type, std::string key_part_no, int agv_id);
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
};

