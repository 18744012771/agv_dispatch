#pragma once

#include <boost/noncopyable.hpp>
#include "common.h"
#include "protocol.h"
#include "network/clientsession.h"

class UserManager;
using UserManagerPtr = std::shared_ptr<UserManager>;

class UserManager :public boost::noncopyable, public std::enable_shared_from_this<UserManager>
{
public:
    static UserManagerPtr getInstance()
	{
        static UserManagerPtr m_inst = UserManagerPtr(new UserManager());
		return m_inst;
	}

    void init();

    void interLogin(ClientSessionPtr conn, const Json::Value &request);

    void interLogout(ClientSessionPtr conn, const Json::Value &request);

    void interChangePassword(ClientSessionPtr conn, const Json::Value &request);

    void interList(ClientSessionPtr conn, const Json::Value &request);

    void interRemove(ClientSessionPtr conn, const Json::Value &request);

    void interAdd(ClientSessionPtr conn, const Json::Value &request);

    void interModify(ClientSessionPtr conn, const Json::Value &request);

	virtual ~UserManager();
private:
	UserManager();

    void checkTable();
};

