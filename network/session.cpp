#include "session.h"
#include "../common.h"
Session::Session(int sessionId, int acceptId):
    _sessionID(sessionId),
    _acceptID(acceptId),
    timeout(2*60*60),
    t(new TimeUsed),
    alive(true)
{
    t->start();
}

Session::~Session()
{
    alive = false;
}

double Session::getUsed()
{
    t->end();
    return t->getUsed();
}
