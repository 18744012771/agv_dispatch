#include "agvsession.h"
#include "sessionmanager.h"
#include "../common.h"
#include "../agvmanager.h"
#include "../Dongyao/dyforklift.h"
#include "../Anting/atforklift.h"

AgvSession::AgvSession(boost::asio::io_context &service):
    Session(service),
    _agvPtr(nullptr)
{
    timeout = 5*60;
}

void AgvSession::afterread()
{
    if(_agvPtr == nullptr){
        return ;
    }
    int returnValue;
    do
    {
        returnValue = ProtocolProcess();
    } while (buffer.size() > 12 && (returnValue == 0 || returnValue == 2 || returnValue == 1));
}

int AgvSession::ProtocolProcess()
{
    int StartIndex,EndIndex;
    std::string FrameData;
    int start_byteFlag = buffer.find('*');
    int end_byteFlag = buffer.find('#');
    if((start_byteFlag ==-1)&&(end_byteFlag ==-1))
    {
        buffer.clear();
        return 5;
    }
    else if((start_byteFlag == -1)&&(end_byteFlag != -1))
    {
        buffer.clear();
        return 6;
    }
    else if((start_byteFlag != -1)&&(end_byteFlag == -1))
    {
        buffer.removeFront(start_byteFlag);
        return 7;
    }
    StartIndex = buffer.find('*')+1;
    EndIndex = buffer.find('#');
    if(EndIndex>StartIndex)
    {
        FrameData = buffer.substr(StartIndex,EndIndex-StartIndex);
        if(FrameData.find('*') != std::string::npos)
            FrameData = FrameData.substr(FrameData.find_last_of('*')+1);
        buffer.removeFront(buffer.find('#')+1);
    }
    else
    {
        //remove unuse message
        buffer.removeFront(buffer.find('*'));
        return 2;
    }
    if(FrameData.length()<10)return 1;
    unsigned int FrameLength = stringToInt(FrameData.substr(6,4));
    if(FrameLength == FrameData.length())
    {
        if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING && _agvPtr!=nullptr && _agvPtr->type() == AtForklift::Type)
        {
            std::static_pointer_cast<AtForklift>(_agvPtr)->onRead(FrameData.c_str(), FrameLength);
        }
        else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO  && _agvPtr!=nullptr && _agvPtr->type() == DyForklift::Type)
        {
            std::static_pointer_cast<DyForklift>(_agvPtr)->onRead(FrameData.c_str(), FrameLength);
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

void AgvSession::onStop()
{
    SessionManager::getInstance()->removeAgvSession(shared_from_base<AgvSession>());
}

void AgvSession::onStart()
{
    combined_logger->debug("new agv connection from:{0} with sessionId:{1}",socket_.remote_endpoint().address().to_string(),sessionId);
    if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT)
    {
        auto agv = std::static_pointer_cast<DyForklift>(AgvManager::getInstance()->getAgvByIP(socket_.remote_endpoint().address().to_string()));

        if(agv)
        {
            if(agv->getTask() == nullptr)
                agv->status = Agv::AGV_STATUS_NOTREADY;
            else
                agv->status = Agv::AGV_STATUS_TASKING;

            agv->setQyhTcp(shared_from_base<AgvSession>());
            agv->setPort(socket_.remote_endpoint().port());
            setAGVPtr(agv);
            //start report
            agv->startReport(100);
            SessionManager::getInstance()->addAgvSession(shared_from_base<AgvSession>());
            _agvPtr = agv;
            return ;
        }
        else
        {
            combined_logger->warn("AGV is not in the list::ip={}.", socket_.remote_endpoint().address().to_string());
        }
    }
    else if(AGV_PROJECT_ANTING == GLOBAL_AGV_PROJECT)
    {
        auto agv = std::static_pointer_cast<AtForklift>(AgvManager::getInstance()->getAgvByIP(socket_.remote_endpoint().address().to_string()));
        if(agv!=nullptr){
            agv->status = Agv::AGV_STATUS_NOTREADY;
            agv->setQyhTcp(shared_from_base<AgvSession>());
            agv->setPort(socket_.remote_endpoint().port());
            setAGVPtr(agv);
            //start report
            agv->startReport(100);
            SessionManager::getInstance()->addAgvSession(shared_from_base<AgvSession>());
            _agvPtr = agv;
            return ;
        }else{
            combined_logger->info("there is no agv with this ip:{}",socket_.remote_endpoint().address().to_string());
        }
    }
    stop();
}
