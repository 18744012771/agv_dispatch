#ifndef SESSION_H
#define SESSION_H

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/atomic/atomic.hpp>
#include "../qyhbuffer.h"
#include "../protocol.h"

class SessionManager;
using SessionManagerPtr = std::shared_ptr<SessionManager>;
class Session;
using SessionPtr = std::shared_ptr<Session>;

class Session : public std::enable_shared_from_this<Session>,private boost::noncopyable
{
public:
    explicit Session(boost::asio::io_context &_context);

    virtual void afterread() = 0;
    virtual void onStart() = 0 ;//add session to session manager
    virtual void onStop() = 0 ;//remove session from session manager

    boost::asio::ip::tcp::socket& socket();

    void start();
    void stop();
    void read();
    void send(const Json::Value &json);
    void write(const char *data,int len);
    int getSessionId(){return sessionId;}
    bool alive(){return _alive;}

    virtual void onread(const boost::system::error_code& ec,
                        std::size_t bytes_transferred);
    virtual void onTimeOut(const boost::system::error_code &ec);
    virtual void onWrite(boost::system::error_code ec, char *sendTempPtr);
    int getTimeout(){return timeout;}
    void setTimeout(int _timeout){
        wait_request_timer_.cancel();
        timeout = _timeout;
        wait_request_timer_.expires_from_now(boost::posix_time::seconds(timeout));
    }


protected:
    template <typename Derived>
    std::shared_ptr<Derived> shared_from_base() {
        return std::static_pointer_cast<Derived>(shared_from_this());
    }
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_context::strand strand_;
    boost::asio::deadline_timer wait_request_timer_;

    int sessionId;
    QyhBuffer buffer;
    char read_buffer[MSG_READ_BUFFER_LENGTH];
    int timeout;

    boost::atomic_bool _alive;
};

#endif // SESSION_H
