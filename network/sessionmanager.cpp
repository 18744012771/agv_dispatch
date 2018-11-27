#include "boost/thread.hpp"
#include "sessionmanager.h"
#include "./agvserver.h"
#include "./clientserver.h"
#include "../msgprocess.h"
#include "../common.h"

#define  THREAD_NUMBER  20

SessionManager::SessionManager():
    thread_number(THREAD_NUMBER),
    sessionId(0),
    signals_(io_context_),
    client(nullptr),
    agv(nullptr)
{
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
    signals_.async_wait(boost::bind(&SessionManager::stop, this));
}

void SessionManager::startAgvServer(int port)
{
    if(agv == nullptr){
        agv = new AgvServer(port,io_context_);
        agv->start();
    }
}

void SessionManager::startClientServer(int port)
{
    if(client == nullptr){
        client = new ClientServer(port,io_context_);
        client->start();
    }
}

void SessionManager::run()
{
    std::vector<boost::shared_ptr<boost::thread> > threads;
    for (std::size_t i = 0; i < thread_number; ++i)
    {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
                                                    boost::bind(&boost::asio::io_context::run, &io_context_)));
        threads.push_back(thread);
    }

    for (std::size_t i = 0; i < threads.size(); ++i)
    {
        threads[i]->join();
    }
}

void SessionManager::stop()
{
    g_quit = true;
    io_context_.stop();
}

void SessionManager::addAgvSession(AgvSessionPtr c)
{
    UNIQUE_LCK(agvsessoinmtx);
    agvsessions.insert(c);
}

void SessionManager::removeAgvSession(AgvSessionPtr c)
{
    UNIQUE_LCK(agvsessoinmtx);
    agvsessions.erase(c);
}

void SessionManager::addClientSession(ClientSessionPtr c)
{
    UNIQUE_LCK(clientsessoinmtx);
    clientsessions.insert(c);
}

void SessionManager::removeClientSession(ClientSessionPtr c)
{
    MsgProcess::getInstance()->onSessionClosed(c);
    UNIQUE_LCK(clientsessoinmtx);
    clientsessions.erase(c);
}

void SessionManager::sendToAllClient(Json::Value &v)
{
    UNIQUE_LCK(clientsessoinmtx);
    for(auto itr = clientsessions.begin();itr!=clientsessions.end();++itr){
        (*itr)->send(v);
    }
}

