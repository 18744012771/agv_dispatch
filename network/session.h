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

#define SESSION_MSG_MEMORY_LENGTH   1024

class SessionMsg{
public:
    SessionMsg(const char *_data,int _length){
        m_length = _length;
        if(m_length > SESSION_MSG_MEMORY_LENGTH)m_length = SESSION_MSG_MEMORY_LENGTH;
        memcpy(m_data,_data,m_length);
    }
    const char* data() const
    {
        return m_data;
    }

    char* data()
    {
        return m_data;
    }

    size_t length() const
    {
        return m_length;
    }

    void setLength(size_t new_length)
    {
        m_length = new_length;
        if (m_length > SESSION_MSG_MEMORY_LENGTH)
            m_length = SESSION_MSG_MEMORY_LENGTH;
    }
private:
    char m_data[SESSION_MSG_MEMORY_LENGTH];
    size_t m_length;
};


class Session : public std::enable_shared_from_this<Session>,private boost::noncopyable
{
public:
    typedef std::deque<SessionMsg> SendMessageQueue;

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
    bool alive(){return socket_.is_open();}

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

    std::mutex close_mtx;
};

#endif // SESSION_H
