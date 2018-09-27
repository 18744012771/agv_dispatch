#include "conflictmanager.h"
#include "../common.h"
#include "mapmanager.h"

ConflictManager::ConflictManager()
{

}

void ConflictManager::init()
{
    auto mapmanagerptr = MapManager::getInstance();
    auto cc = mapmanagerptr->getConflictPairs();
    for(auto c:cc)
    {
        conflict_pairs.insert(c->getA(),c->getB());
    }
    for(auto c:cc)
    {
        conflict_pairs.insert(c->getB(),c->getA());
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
        ic.push_back(i);
        jc.push_back(j);
        //check i+1 and j - 1
        if(i+1<agv1spirits.size() && j-1>=0 && checkConflict(agv1spirits[i+1],agv2spirits[j-1])){
            calcc(agv1spirits,agv2spirits,++i,--j,ic,jc);
        }else if(i+1<agv1spirits.size() && j>=0 && checkConflict(agv1spirits[i+1],agv2spirits[j])){
            calcc(agv1spirits,agv2spirits,++i,j,ic,jc);
        }else if(j-1>=0  && checkConflict(agv1spirits[i],agv2spirits[j-1])){
            calcc(agv1spirits,agv2spirits,i,--j,ic,jc);
        }
    }
}


void ConflictManager::addAgvExcuteStationPath(std::vector<int> spirits,int agvId)
{

    UNIQUE_LCK(&stationPathMtx);
    stationsPaths[agvId] = spirits;

    UNIQUE_LCK(conflictMtx);
    conflicts.clear();

    for(auto itr = stationsPaths.begin();itr!=stationsPaths.end();++itr)
    {
        for(auto pos = stationsPaths.begin();pos != stationsPaths.end();++pos)
        {
            if(itr->first == pos->first)continue;
            int agv1 = itr->first;
            int agv2 = pos->first;
            std::vector<int> agv1spirits = itr->second;
            std::vector<int> agv2spirits = itr->second;

            for(int i=0;i<agv1spirits.size();)
            {
                bool hasConflict = false;
                int j=agv2spirits.size() - 1;
                for(;j>=0;--j){
                    if(checkConflict(agv1spirits[i],agv2spirits[j])){
                        hasConflict = true;
                        break;
                    }
                }

                if(hasConflict){
                    std::vector<int> ic;
                    std::vector<int> jc;
                    calcc(agv1spirits,agv2spirits,i,j,ic,jc);
                    Conflict c(agv1,ic,agv2,jc);
                    conflicts.push_back(c);
                }else{
                    ++i;
                }
            }
        }
    }
}

void ConflictManager::removeAgvExcuteStationPath(int spirit,int agvId)
{
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
        if(!itr->tryLock(agvId,spirit))return false;
    }
    return true;
}
void ConflictManager::freeConflictOccu(std::vector<int> spirits,int agvId)
{
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
}

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
