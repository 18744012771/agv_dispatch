#ifndef MSGPROCESS_H
#define MSGPROCESS_H

#include <memory>
#include <list>
#include <mutex>
#include <boost/noncopyable.hpp>
#include "protocol.h"
#include "network/clientsession.h"

class MsgProcess;

using MsgProcessPtr = std::shared_ptr<MsgProcess>;

typedef enum{
    ENUM_NOTIFY_ALL_TYPE_MAP_UPDATE = 0,
    ENUM_NOTIFY_ALL_TYPE_ERROR,
}ENUM_NOTIFY_ALL_TYPE;

class MsgProcess : public boost::noncopyable,public std::enable_shared_from_this<MsgProcess>
{
public:

    static MsgProcessPtr getInstance(){
        static MsgProcessPtr m_inst = MsgProcessPtr(new MsgProcess());
        return m_inst;
    }

    bool init();

	//用户断开连接导致 用户退出登录
	void sessionLogout(int user_id);

    //进来一个消息,分配给一个线程去处理它
    void processOneMsg(const Json::Value &request,ClientSessionPtr session);

    //发布一个日志消息
    void publishOneLog(USER_LOG log);

    //通知所有用户的事件
    void notifyAll(ENUM_NOTIFY_ALL_TYPE type);

    //发生错误，需要告知
    void errorOccur(int code,std::string msg,bool needConfirm);

    //用户接口
    void interAddSubAgvPosition(ClientSessionPtr conn, const Json::Value &request);
    void interAddSubAgvStatus(ClientSessionPtr conn, const Json::Value &request);
    void interAddSubTask(ClientSessionPtr conn, const Json::Value &request);
    void interAddSubLog(ClientSessionPtr conn, const Json::Value &request);
	void interAddSubELE(ClientSessionPtr conn, const Json::Value &request);
    void interRemoveSubAgvPosition(ClientSessionPtr conn, const Json::Value &request);
    void interRemoveSubAgvStatus(ClientSessionPtr conn,const Json::Value &request);
    void interRemoveSubTask(ClientSessionPtr conn, const Json::Value &request);
    void interRemoveSubLog(ClientSessionPtr conn, const Json::Value &request);
	void interRemoveSubELE(ClientSessionPtr conn, const Json::Value &request);


    void onSessionClosed(ClientSessionPtr id);

    void addSubAgvPosition(ClientSessionPtr id);
    void addSubAgvStatus(ClientSessionPtr id);
    void addSubTask(ClientSessionPtr id);
    void addSubLog(ClientSessionPtr id);
    void addSubELE(ClientSessionPtr id);

    void removeSubAgvPosition(ClientSessionPtr id);
    void removeSubAgvStatus(ClientSessionPtr id);
    void removeSubTask(ClientSessionPtr id);
    void removeSubLog(ClientSessionPtr id);
    void removeSubELE(ClientSessionPtr id);
private:

    void publisher_agv_position();

    void publisher_agv_status();

    void publisher_task();

	void publisher_ELE();

    MsgProcess();

    std::mutex psMtx;
    std::list<ClientSessionPtr> agvPositionSubers;

    std::mutex ssMtx;
    std::list<ClientSessionPtr> agvStatusSubers;

    std::mutex tsMtx;
    std::list<ClientSessionPtr> taskSubers;

    std::mutex lsMtx;
    std::list<ClientSessionPtr> logSubers;

	std::mutex eleMtx;
    std::list<ClientSessionPtr> eleSubers;

    std::mutex errorMtx;
    int error_code;
    std::string error_info;
    bool needConfirm;
};



#endif // MSGPROCESS_H
