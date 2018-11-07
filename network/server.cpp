#include "server.h"

Server::Server(int _port, boost::asio::io_context &_context):
    context(_context),
    port(_port),
    acceptor(_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port))
{

}

void Server::start()
{
    doAccept();
}

