#include "conflict.h"
#include "../common.h"

Conflict::Conflict(int _agvAId, std::vector<int> _agvAspirits, int _agvBId, std::vector<int> _agvBspirits):
    lockedAgv(0),
    agvA(_agvAId),
    agvAspirits(_agvAspirits),
    agvB(_agvBId),
    agvBspirits(_agvBspirits)
{

}

Conflict::Conflict(const Conflict&b)
{
    lockedAgv = b.lockedAgv;
    agvA = b.agvA;
    agvB = b.agvB;
    agvAspirits = b.agvAspirits;
    agvBspirits = b.agvBspirits;
}

Conflict &Conflict::operator = (const Conflict&b)
{
    lockedAgv = b.lockedAgv;
    agvA = b.agvA;
    agvB = b.agvB;
    agvAspirits = b.agvAspirits;
    agvBspirits = b.agvBspirits;
}

Conflict::~Conflict()
{

}

bool Conflict::passable(int agvId,int spirit)
{
    if(isAllFree())return true;
    if(agvId!=agvA && agvId!=agvB)return true;
    if(agvId == agvA && std::find(agvAspirits.begin(),agvAspirits.end(),spirit)==agvAspirits.end())return true;
    if(agvId == agvB && std::find(agvBspirits.begin(),agvBspirits.end(),spirit)==agvBspirits.end())return true;
    if(lockedAgv == 0 || lockedAgv == agvId)return true;
    return false;
}

void Conflict::print()
{
    std::stringstream ssa;
    std::stringstream ssb;
    for(auto i:agvAspirits)ssa<<i<<",";
    for(auto i:agvBspirits)ssb<<i<<",";
    combined_logger->info("agv{0} {1} ; agv{2} {3} conflict:",agvA,ssa.str(),agvB,ssb.str());
}

bool Conflict::tryLock(int agvId,int spirit)
{
    if(isAllFree())return true;
    if(agvId!=agvA && agvId!=agvB)return true;
    if(agvId == agvA && std::find(agvAspirits.begin(),agvAspirits.end(),spirit)==agvAspirits.end())return true;
    if(agvId == agvB && std::find(agvBspirits.begin(),agvBspirits.end(),spirit)==agvBspirits.end())return true;
    UNIQUE_LCK(lockedAgvMtx);
    if(lockedAgv == agvId)return true;
    if(lockedAgv!=0 && lockedAgv!=agvId)return false;
    lockedAgv = agvId;
    return true;
}

bool Conflict::freeLockExcept(int agvId,int spirit)
{
    UNIQUE_LCK(lockedAgvMtx);
    if(agvId == agvA){
        for(auto itr = agvAspirits.begin();itr!=agvAspirits.end();){
            if(*itr == spirit){
                ++itr;
            }else{
                itr = agvAspirits.erase(itr);
            }
        }

        if(agvAspirits.empty()){
            lockedAgv = 0;
        }
    }else if(agvId == agvB){
        for(auto itr = agvBspirits.begin();itr!=agvBspirits.end();){
            if(*itr == spirit){
                ++itr;
            }else{
                itr = agvBspirits.erase(itr);
            }
        }

        if(agvBspirits.empty()){
            lockedAgv = 0;
        }
    }

    return true;
}

bool Conflict::freeLock(int agvId, int spirit)
{
    UNIQUE_LCK(lockedAgvMtx);
    if(lockedAgv!=agvId)return false;
    if(lockedAgv == agvA){
        for(auto itr = agvAspirits.begin();itr!=agvAspirits.end();){
            if(*itr == spirit){
                itr = agvAspirits.erase(itr);
            }else{
                ++itr;
            }
        }

        if(agvAspirits.empty()){
            lockedAgv = 0;
        }
    }else if(lockedAgv == agvB){
        for(auto itr = agvBspirits.begin();itr!=agvBspirits.end();){
            if(*itr == spirit){
                itr = agvBspirits.erase(itr);
            }else{
                ++itr;
            }
        }

        if(agvBspirits.empty()){
            lockedAgv = 0;
        }
    }

    return true;
}

bool Conflict::isAllFree()
{
    UNIQUE_LCK(lockedAgvMtx);
    return agvAspirits.empty() || agvBspirits.empty();
}
