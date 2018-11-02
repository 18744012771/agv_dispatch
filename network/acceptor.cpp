#include "acceptor.h"
#include "../common.h"
Acceptor::Acceptor(boost::asio::io_context &io_context, short port, int _id)
    :_io_context(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      id(_id)
{

}

Acceptor::~Acceptor()
{
    acceptor_.close();
    if(t && t->joinable())
        t->join();
}

void Acceptor::doAccept()
{
    while(acceptor_.is_open())
    {

        try{

            tcp::socket socket{_io_context};

            acceptor_.accept(socket);

            onAccept(socket);
        }
        catch(std::exception const& e)
        {
            break;
        }
    }
}

//启动一个线程进行监听
void Acceptor::start()
{
    t = g_threads.create_thread(std::bind(&Acceptor::doAccept,this));
}

void Acceptor::close()
{
    acceptor_.close();
}
