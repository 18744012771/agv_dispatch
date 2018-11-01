#include "conflictmanager.h"
#include "../common.h"
#include "mapmanager.h"
#include "../agvmanager.h"

ConflictManager::ConflictManager()
{

}

void ConflictManager::init()
{
    auto mapmanagerptr = MapManager::getInstance();
    auto cc = mapmanagerptr->getConflictPairs();
    for(auto c:cc)
    {
        conflict_pairs.insert(std::make_pair(c->getA(),c->getB()));
    }
    for(auto c:cc)
    {
        conflict_pairs.insert(std::make_pair(c->getB(),c->getA()));
    }
}

bool ConflictManager::checkConflict(int a,int b)
{
    if(a == b)return true;
    auto itlow = conflict_pairs.lower_bound (a);
    auto itup = conflict_pairs.upper_bound (a);

    for (auto it=itlow; it!=itup; ++it)
        if(it->second == b)return true;

    return false;
}

void ConflictManager::calcc(std::vector<int> &agv1spirits,std::vector<int> &agv2spirits,int &i,int &j,std::vector<int> &ic,std::vector<int>&jc)
{
    if(i<agv1spirits.size() && j>=0){
        if(i+1<agv1spirits.size() && j-1>=0 && checkConflict(agv1spirits[i+1],agv2spirits[j-1])){
            ic.push_back(agv1spirits[++i]);
            jc.push_back(agv2spirits[--j]);
            calcc(agv1spirits,agv2spirits,i,j,ic,jc);
        }else if(i+1<agv1spirits.size() && j>=0 && checkConflict(agv1spirits[i+1],agv2spirits[j])){
            ic.push_back(agv1spirits[++i]);
            calcc(agv1spirits,agv2spirits,i,j,ic,jc);
        }else if(j-1>=0  && checkConflict(agv1spirits[i],agv2spirits[j-1])){
            jc.push_back(agv2spirits[--j]);
            calcc(agv1spirits,agv2spirits,i,j,ic,jc);
        }
    }
}

void ConflictManager::addAgvExcuteStationPath(std::vector<int> spirits,int agvId)
{

    stationPathMtx.lock();
    agvStationsPaths[agvId] = spirits;
    stationPathMtx.unlock();

    conflictMtx.lock();
    conflicts.clear();
    conflictMtx.unlock();

    stationPathMtx.lock();
    for(auto itr = agvStationsPaths.begin();itr!=agvStationsPaths.end();++itr)
    {
        for(auto pos = agvStationsPaths.begin();pos != agvStationsPaths.end();++pos)
        {
            if(itr->first == pos->first)continue;
            int agv1 = itr->first;
            int agv2 = pos->first;
            std::vector<int> agv1spirits = itr->second;
            std::vector<int> agv2spirits = pos->second;

            //TODO: fix conflicts bug
            for(int i=0;i<agv1spirits.size();++i)
            {
                for(int j=agv2spirits.size()-1;j>=0;--j)
                {
                    if(checkConflict(agv1spirits[i],agv2spirits[j])){
                        std::vector<int> ic;
                        std::vector<int> jc;
                        ic.push_back(agv1spirits[i]);
                        jc.push_back(agv2spirits[j]);
                        int ii = i;
                        int jj = j;
                        calcc(agv1spirits,agv2spirits,ii,jj,ic,jc);
                        std::reverse(jc.begin(),jc.end());
                        Conflict c(agv1,ic,agv2,jc);
                        conflicts.push_back(c);
                    }
                }
            }
        }
    }
    stationPathMtx.unlock();

    //TODO:
    std::map<int,int> agvoccus;

    auto agvmanagerptr = AgvManager::getInstance();
    auto mapmanagerptr = MapManager::getInstance();

    stationPathMtx.lock();
    for(auto itr = agvStationsPaths.begin();itr!=agvStationsPaths.end();++itr)
    {
        auto agvptr = agvmanagerptr->getAgvById(itr->first);
        if(agvptr==nullptr)continue;
        if(agvptr->getNowStation() >0){
            agvoccus[agvptr->getId()] = agvptr->getNowStation();
        }else{
            auto currentPath = mapmanagerptr->getPathByStartEnd(agvptr->getLastStation(),agvptr->getNextStation());
            if(currentPath==nullptr)continue;
            agvoccus[agvptr->getId()] = currentPath->getId();
        }
    }
    stationPathMtx.unlock();

    for(auto itr = agvoccus.begin();itr!=agvoccus.end();++itr)
    {
        tryAddConflictOccu(itr->second,itr->first);
    }

    //printConflict();
}

void ConflictManager::freeAgvOccu(int agvId,int lastStation,int nowStation,int nextStation)
{
    int occu = -1;
    if(nowStation > 0){
        occu = nowStation;
    }else{
        auto line = MapManager::getInstance()->getPathByStartEnd(lastStation,nextStation);
        if(line!=nullptr)occu = line->getId();
    }

    std::vector<int> nowOccus;
    nowOccus.push_back(occu);

    UNIQUE_LCK(stationPathMtx);
    if(agvStationsPaths.find(agvId) == agvStationsPaths.end()){
        agvStationsPaths[agvId] = nowOccus;
        return ;
    }

    //free conflict
    UNIQUE_LCK(conflictMtx);
    for(auto itr = conflicts.begin();itr!=conflicts.end();){
        itr->freeLockExcept(agvId,occu);
        if(itr->isAllFree()){
            itr = conflicts.erase(itr);
        }else
            ++itr;
    }
}

void ConflictManager::freeConflictOccu(int spirit,int agvId)
{
    UNIQUE_LCK(stationPathMtx);
    if(agvStationsPaths.find(agvId) == agvStationsPaths.end())return ;

    auto spirits = agvStationsPaths[agvId];
    //check if exist
    bool exist = false;
    for(auto pos = spirits.begin();pos!=spirits.end();++pos)
    {
        if(*pos == spirit){
            exist = true;
            break;
        }
    }

    //remove before
    if(exist){
        for(auto pos = spirits.begin();pos!=spirits.end();)
        {
            if(*pos == spirit){
                pos = spirits.erase(pos);
                break;
            }
            pos = spirits.erase(pos);
        }
    }

    agvStationsPaths[agvId] = spirits;

    //free conflict
    UNIQUE_LCK(conflictMtx);
    for(auto itr = conflicts.begin();itr!=conflicts.end();){
        itr->freeLock(agvId,spirit);
        if(itr->isAllFree()){
            itr = conflicts.erase(itr);
        }else
            ++itr;
    }
}

bool ConflictManager::tryAddConflictOccu(int spirit,int agvId)
{
    UNIQUE_LCK(conflictMtx);
    for(auto itr = conflicts.begin();itr!=conflicts.end();++itr){
        if(!itr->checkLock(agvId,spirit))return false;
    }

    for(auto itr = conflicts.begin();itr!=conflicts.end();++itr){
        if(!itr->lock(agvId,spirit))return false;
    }

    return true;
}
//void ConflictManager::freeConflictOccu(std::vector<int> spirits,int agvId)
//{
//    UNIQUE_LCK(conflictMtx);
//    for(auto block:blocks)
//    {
//        for(auto itr=bblocks.begin();itr!=bblocks.end();++itr)
//        {
//            if(itr->getBlockId() == block){
//                itr->removeOccu(agvId,spiritId);
//            }
//        }
//    }
//}

//bool ConflictManager::conflictPassable(std::vector<int> blocks, int agvId)
//{
//    UNIQUE_LCK(conflictMtx);
//    for(auto block:blocks)
//    {
//        for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
//            if(itr->getBlockId() == block){
//                if(!itr->passable(agvId))return false;
//            }
//        }
//    }
//    return true;
//}

void ConflictManager::clear()
{
    UNIQUE_LCK(conflictMtx);
    conflicts.clear();
}

bool ConflictManager::conflictPassable(int spiritId,int agvId)
{
    UNIQUE_LCK(conflictMtx);
    for(auto itr = conflicts.begin();itr!=conflicts.end();++itr){
        if(!itr->passable(agvId,spiritId))return false;
    }
    return true;
}
void ConflictManager::printConflict()
{
    UNIQUE_LCK(conflictMtx);
    combined_logger->debug("CONFLICT===============================================");
    for(auto itr = conflicts.begin();itr!=conflicts.end();++itr){
        itr->print();
    }
    combined_logger->debug("CONFLICT===============================================");
}

void ConflictManager::test()
{

}
