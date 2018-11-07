#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <set>
#include "clientsession.h"
#include "agvsession.h"

class ClientServer;
class AgvServer;

class SessionManager;
using SessionManagerPtr = std::shared_ptr<SessionManager>;

class SessionManager : public boost::noncopyable,public std::enable_shared_from_this<SessionManager>
{
public:

    static SessionManagerPtr getInstance(){
        static SessionManagerPtr m_inst = SessionManagerPtr(new SessionManager());
        return m_inst;
    }

    void startAgvServer(int port);

    void startClientServer(int port);

    void addAgvSession(AgvSessionPtr c);

    void removeAgvSession(AgvSessionPtr c);

    void addClientSession(ClientSessionPtr c);

    void removeClientSession(ClientSessionPtr c);

    void run();

    void stop();

    int getNextSessionId(){return sessionId++;}

    void sendToAllClient(Json::Value &v);

private:

    SessionManager();

    ClientServer *client;
    AgvServer *agv;

    boost::asio::io_context io_context_;

    boost::asio::signal_set signals_;

    int thread_number;

    boost::atomic_int   sessionId;

    std::mutex agvsessoinmtx;
    std::set<AgvSessionPtr> agvsessions;


    std::mutex clientsessoinmtx;
    std::set<ClientSessionPtr> clientsessions;
};

#endif // SESSIONMANAGER_H
