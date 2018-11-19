#include "session.h"
#include "./sessionmanager.h"
#include "../common.h"

Session::Session(boost::asio::io_context &_context):
    strand_(_context),
    socket_(_context),
    timeout(90),
    wait_request_timer_(_context)
{
    sessionId = SessionManager::getInstance()->getNextSessionId();
}

void Session::start()
{
    onStart();
    read();
}

boost::asio::ip::tcp::socket& Session::socket()
{
    return socket_;
}

void Session::onTimeOut(const boost::system::error_code& ec)
{
    if (ec.value() == boost::asio::error::operation_aborted) {
        return;
    }

    if (wait_request_timer_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
        combined_logger->info("session id {} time out!",sessionId);
        wait_request_timer_.cancel();
        stop();
    }
}

void Session::read()
{
    wait_request_timer_.expires_from_now(boost::posix_time::seconds(timeout));
    wait_request_timer_.async_wait(strand_.wrap(bind(&Session::onTimeOut, shared_from_this(), _1)));
    socket_.async_read_some(boost::asio::buffer(read_buffer,MSG_READ_BUFFER_LENGTH),
                            boost::asio::bind_executor(strand_,
                                                       boost::bind(&Session::onread, shared_from_this(),
                                                                   boost::asio::placeholders::error,
                                                                   boost::asio::placeholders::bytes_transferred)));
}

void Session::onread(const boost::system::error_code& ec,
                     std::size_t bytes_transferred)
{
    if (!ec) {
        combined_logger->debug("session id {1} read length:{0} :{2} ",bytes_transferred,sessionId,std::string(read_buffer,bytes_transferred));
        wait_request_timer_.cancel();
        buffer.append(read_buffer,bytes_transferred);
        afterread();
        read();
    }
    else if(ec ==  boost::asio::error::operation_aborted){
        //do nothing
    }
    else if(ec == boost::asio::error::eof ){
        combined_logger->debug("session id {} close",sessionId);
        stop();
    }else{
        combined_logger->debug("session id {0} read fail,error:{1}",sessionId,ec.message());
        stop();
    }

}

void Session::send(const Json::Value &json)
{
    std::string msg = json.toStyledString();
    int length = msg.length();
    if(length<=0)return;

    combined_logger->info("SEND! session id {0}  len= {1} " ,sessionId,length);

    char *send_buffer = new char[length + 6];
    memset(send_buffer, 0, length + 6);
    send_buffer[0] = MSG_MSG_HEAD;
    memcpy_s(send_buffer + 1, length+5, (char *)&length, sizeof(length));
    memcpy_s(send_buffer +5, length + 1, msg.c_str(),msg.length());

    write(send_buffer,length+5);

    delete[] send_buffer;
}

void Session::stop()
{
    onStop();
    wait_request_timer_.cancel();
    socket_.close();
}

void Session::write(const char *data,int len)
{
    //TODO:fenbao?

    boost::asio::async_write(socket_, boost::asio::buffer(data, len),
                             boost::asio::bind_executor(strand_,
                                                        boost::bind(&Session::onWrite, shared_from_this(),
                                                                    boost::asio::placeholders::error)));
}

void Session::onWrite(boost::system::error_code ec)
{
    if (ec && ec !=  boost::asio::error::operation_aborted){
        combined_logger->debug("session id {0} write fail,error:{1}",sessionId,ec.message());
        stop();
    }
}
