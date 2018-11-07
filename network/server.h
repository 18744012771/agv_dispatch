#ifndef SERVER_H
#define SERVER_H
#include <boost/noncopyable.hpp>

#include "./sessionmanager.h"

class Server : private boost::noncopyable
{
public:
    Server(int _port,boost::asio::io_context &_service);
    void start();
    virtual void doAccept() = 0;
    virtual void onAccept(const boost::system::error_code& e) = 0;
protected:
    boost::asio::ip::tcp::acceptor acceptor;
    int port;
    boost::asio::io_context &context;
};

#endif // SERVER_H
