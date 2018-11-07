#include "clientserver.h"
#include "./clientsession.h"
#include "../common.h"

ClientServer::ClientServer(int _port, boost::asio::io_context &_context):
    Server(_port,_context),
    c()
{
}

void ClientServer::doAccept() {
    c.reset(new ClientSession(context));
    acceptor.async_accept(c->socket(),
                          boost::bind(&ClientServer::onAccept, this,
                                      boost::asio::placeholders::error));
}

void ClientServer::onAccept(const boost::system::error_code& e)
{
    if (!e)
    {
        c->start();
    }

    doAccept();
}

