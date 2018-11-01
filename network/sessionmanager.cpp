#include "sessionmanager.h"
#include "../common.h"
#include "tcpsession.h"
#include "tcpacceptor.h"
#include "websocketsession.h"
#include "websocketacceptor.h"

#include "../msgprocess.h"

SessionManager::SessionManager():
    nextAcceptId(0),
    nextSessionId(0)
{
}

int SessionManager::addTcpAccepter(int port)
{
    TcpAcceptorPtr acceptor = std::make_shared<TcpAcceptor>(io_context,port,++nextAcceptId);
    mapAcceptorMtx.lock();
    _mapAcceptorPtr[acceptor->getId()] = acceptor;
    mapAcceptorMtx.unlock();
    return acceptor->getId();
}

void SessionManager::openTcpAccepter(int aID)
{
    mapAcceptorMtx.lock();
    for(auto a = _mapAcceptorPtr.begin();a!=_mapAcceptorPtr.end();++a){
        if(a->first == aID ){
            a->second->start();
            break;
        }
    }
    mapAcceptorMtx.unlock();
}

int SessionManager::addWebSocketAccepter(int port)
{
    WebsocketAcceptorPtr acceptor = std::make_shared<WebsocketAcceptor>(io_context, port, ++nextAcceptId);
    mapAcceptorMtx.lock();
    _mapAcceptorPtr[acceptor->getId()] = acceptor;
    mapAcceptorMtx.unlock();
    return acceptor->getId();
}
void SessionManager::openWebSocketAccepter(int aID)
{
    mapAcceptorMtx.lock();
    for (auto a = _mapAcceptorPtr.begin(); a != _mapAcceptorPtr.end(); ++a) {
        if (a->first == aID) {
            a->second->start();
            break;
        }
    }
    mapAcceptorMtx.unlock();
}

void SessionManager::run()
{
    std::thread([&]
    {
        std::vector<int> toKickSessions;

        while(!g_quit){
            toKickSessions.clear();
            mapSessionMtx.lock();
            for (auto &ms : _mapSessionPtr)
            {
                if(!ms.second->isAlive())continue;
                //combined_logger->debug("session{0} time used={1}",ms.first,ms.second->getUsed());
                if(ms.second->getUsed() > 1.0*ms.second->getTimeOut())
                {
                    toKickSessions.push_back(ms.first);
                }
            }
            mapSessionMtx.unlock();

            for(auto itr = toKickSessions.begin();itr!=toKickSessions.end();++itr){
                kickSession(*itr);
            }
            usleep(100000);
        }
    }).detach();
}

void SessionManager::kickSession(int sID)
{
    UNIQUE_LCK(mapSessionMtx);
    auto iter = _mapSessionPtr.find(sID);
    if (iter == _mapSessionPtr.end())
    {
        combined_logger->info("kickSession NOT FOUND SessionID. SessionID={0}", sID);
        return;
    }
    combined_logger->info("kickSession SessionID. SessionID={0}",sID);
    iter->second->close();
}

void SessionManager::sendSessionData(int sID, const Json::Value &response)
{
    UNIQUE_LCK(mapSessionMtx);
    auto iter = _mapSessionPtr.find(sID);
    if (iter == _mapSessionPtr.end())
    {
        combined_logger->warn("sendSessionData NOT FOUND SessionID.  SessionID={0}",sID);
        return;
    }
    iter->second->send(response);
}

void SessionManager::sendData(const Json::Value &response)
{
    UNIQUE_LCK(mapSessionMtx);
    for (auto &ms : _mapSessionPtr)
    {
        ms.second->send(response);
    }
}

void SessionManager::kickSessionByUserId(int userId)
{
    UNIQUE_LCK(mapSessionMtx);
    for (auto &ms : _mapSessionPtr)
    {
        if (ms.second->getUserId() == userId)
        {
            ms.second->close();
        }
    }
}

void SessionManager::addSession(int id,SessionPtr session)
{
    UNIQUE_LCK(mapSessionMtx);
    _mapSessionPtr[id] = session;
}
void SessionManager::removeSession(SessionPtr session)
{
    if(session==nullptr)return ;
    UNIQUE_LCK(mapSessionMtx);
    _mapSessionPtr.erase(session->getSessionID());
    //取消该session的所有订阅
    MsgProcess::getInstance()->onSessionClosed(session->getSessionID());
}
