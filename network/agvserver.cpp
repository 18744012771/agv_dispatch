#include "agvserver.h"
#include "./agvsession.h"
#include "../common.h"

AgvServer::AgvServer(int _port, boost::asio::io_context &_context):
    Server(_port,_context),
    c()
{

}

void AgvServer::doAccept() {
    c.reset(new AgvSession(context));
    acceptor.async_accept(c->socket(),
                          boost::bind(&AgvServer::onAccept, this,
                                      boost::asio::placeholders::error));
}

void AgvServer::onAccept(const boost::system::error_code& e)
{
    if (!e)
    {
        c->start();
    }

    doAccept();
}
