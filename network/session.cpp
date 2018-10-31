#include "session.h"
#include "../common.h"
Session::Session(int sessionId, int acceptId):
    _sessionID(sessionId),
    _acceptID(acceptId),
    timeout(2*60*60),
    t(new TimeUsed)
{
    t->start();
}

Session::~Session()
{

}

double Session::getUsed()
{
    t->end();
    return t->getUsed();
}
