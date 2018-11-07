#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include "session.h"

class ClientSession;
using ClientSessionPtr = std::shared_ptr<ClientSession>;

class ClientSession : public Session
{
public:
    ClientSession(boost::asio::io_context &service);
    virtual void afterread() ;
    virtual void onStop();
    virtual void onStart();

    void setUserId(int _user_id){user_id = _user_id;}

    int getUserId(){return user_id;}

    void setUserRole(int _user_role){user_role = _user_role;}

    int getUserRole(){return user_role;}

    void setUserName(std::string _user_name){username = _user_name;}

    std::string getUserName(){return username;}

private:
    void packageProcess();
    int json_len;
    //连接保存一个用户的两个信息
    int user_id = 0;
    int user_role = 0;
    std::string username;
};


#endif // CLIENTSESSION_H
