#ifndef CLIENTSERVER_H
#define CLIENTSERVER_H

#include "./server.h"

class ClientServer : public Server
{
public:
    ClientServer(int _port, boost::asio::io_context &_context);
    virtual void doAccept();
    void onAccept(const boost::system::error_code& e);
private:
    ClientSessionPtr c;
};

#endif // CLIENTSERVER_H
