#ifndef AGVSESSION_H
#define AGVSESSION_H

#include "session.h"
#include "../agv.h"
#include "../protocol.h"

class AgvSession;
using AgvSessionPtr = std::shared_ptr<AgvSession>;

class AgvSession : public Session
{
public:
    AgvSession(boost::asio::io_context &service);
    virtual void afterread();
    virtual void onStart();
    virtual void onStop();
private:
    int ProtocolProcess();
    inline void setAGVPtr(AgvPtr agv){_agvPtr = agv;}
    AgvPtr _agvPtr;
};

#endif // AGVSESSION_H
