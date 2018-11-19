#ifndef AGVSERVER_H
#define AGVSERVER_H

#include "./server.h"

class AgvServer : public Server
{
public:
    AgvServer(int _port, boost::asio::io_context &_context);
    virtual void doAccept();
    virtual void onAccept(const boost::system::error_code& e);
private:
    AgvSessionPtr c;
};

#endif // AGVSERVER_H
