#include "blockmanager.h"

AgvOccuSpirits::AgvOccuSpirits(int _agvid):
    agvid(_agvid)
{

}

AgvOccuSpirits::AgvOccuSpirits(const AgvOccuSpirits & b):
    agvid(b.agvid)
{
    spirits = b.spirits;
}

AgvOccuSpirits &AgvOccuSpirits::operator =(const AgvOccuSpirits &b)
{
    AgvOccuSpirits c(b.agvid);
    c.spirits = b.spirits;
    return c;
}

void AgvOccuSpirits::addSpirit(int spiritId)
{
    if(std::find( spirits.begin(),spirits.end(),spiritId) == spirits.end())
        spirits.push_back(spiritId);
}

void AgvOccuSpirits::removeSpirit(int spiritId)
{
    for(auto itr = spirits.begin();itr!=spirits.end();){
        if(*itr == spiritId){
            itr = spirits.erase(itr);
        }else
            ++itr;
    }
}

bool AgvOccuSpirits::empty()
{
    return spirits.empty();
}

int AgvOccuSpirits::getAgvid()
{
    return agvid;
}

std::string AgvOccuSpirits::getSpiritsStr()
{
    std::stringstream ss;
    for(auto itr = spirits.begin();itr!=spirits.end();++itr){
        ss<<*itr;
    }
    return ss.str();
}


GGroup::GGroup(int bid):
    group_id(bid)
{
}

GGroup::GGroup(const GGroup & b):
    group_id(b.group_id)
{
    group_id = b.group_id;
    agv_spirits = b.agv_spirits;
}

GGroup &GGroup::operator =(const GGroup &b)
{
    GGroup c(b.group_id);
    c.agv_spirits = b.agv_spirits;
    return c;
}

void GGroup::addOccu(int agvid,int spiritid)
{
    bool alreadAdd = false;
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->addSpirit(spiritid);
            alreadAdd = true;
            break;
        }
    }
    if(!alreadAdd){
        AgvOccuSpirits a(agvid);
        a.addSpirit(spiritid);
        agv_spirits.push_back(a);
    }
}

void GGroup::removeOccu(int agvid,int spiritid)
{
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->removeSpirit(spiritid);
            break;
        }
    }
}

bool GGroup::passable(int agvid)
{
    for(auto aos:agv_spirits){
        if(aos.getAgvid() == agvid)continue;
        if(!aos.empty())return false;
    }
    return true;
}

int GGroup::getGroupId()
{
    return group_id;
}

void GGroup::print()
{
    for(auto aos:agv_spirits){
        if(!aos.empty())
        {
            combined_logger->debug("groupid:{0} agv:{1} station_or_lines:{2}",getGroupId(),aos.getAgvid(),aos.getSpiritsStr());
        }
    }
}

GroupManager::GroupManager()
{

}

bool GroupManager::tryAddGroupOccu(std::vector<int> groups,int agvId,int spiritId)
{
    UNIQUE_LCK(groupMtx);
    if(!noneLockGroupPassable(groups,agvId))return false;
    for(auto group:groups)
    {
        bool alreadAdd = false;
        for(auto itr=ggroups.begin();itr!=ggroups.end();++itr)
        {
            if(itr->getGroupId() == group){
                itr->addOccu(agvId,spiritId);
                alreadAdd = true;
                break;
            }
        }

        if(!alreadAdd){
            GGroup bb(group);
            bb.addOccu(agvId,spiritId);
            ggroups.push_back(bb);
        }
    }

    return true;
}
void GroupManager::freeGroupOccu(std::vector<int> groups,int agvId, int spiritId)
{
    UNIQUE_LCK(groupMtx);
    for(auto group:groups)
    {
        for(auto itr=ggroups.begin();itr!=ggroups.end();++itr)
        {
            if(itr->getGroupId() == group){
                itr->removeOccu(agvId,spiritId);
            }
        }
    }
}

bool GroupManager::groupPassable(std::vector<int> groups, int agvId)
{
    UNIQUE_LCK(groupMtx);
    for(auto group:groups)
    {
        for(auto itr = ggroups.begin();itr!=ggroups.end();++itr){
            if(itr->getGroupId() == group){
                if(!itr->passable(agvId))return false;
            }
        }
    }
    return true;
}

bool GroupManager::noneLockGroupPassable(std::vector<int> groups, int agvId)
{
    for(auto group:groups)
    {
        for(auto itr = ggroups.begin();itr!=ggroups.end();++itr){
            if(itr->getGroupId() == group){
                if(!itr->passable(agvId))return false;
            }
        }
    }
    return true;
}

void GroupManager::clear()
{
    UNIQUE_LCK(groupMtx);
    ggroups.clear();
}

void GroupManager::print()
{
    UNIQUE_LCK(groupMtx);
    combined_logger->debug("GROUP===============================================");
    for(auto itr = ggroups.begin();itr!=ggroups.end();++itr){
        itr->print();
    }
    combined_logger->debug("GROUP===============================================");
}

BBlock::BBlock(int bid):
    block_id(bid)
{
}

BBlock::BBlock(const BBlock & b):
    block_id(b.block_id)
{
    block_id = b.block_id;
    agv_spirits = b.agv_spirits;
}

BBlock &BBlock::operator =(const BBlock &b)
{
    BBlock c(b.block_id);
    c.agv_spirits = b.agv_spirits;
    return c;
}

void BBlock::addOccu(int agvid,int spiritid)
{
    bool alreadAdd = false;
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->addSpirit(spiritid);
            alreadAdd = true;
            break;
        }
    }
    if(!alreadAdd){
        AgvOccuSpirits a(agvid);
        a.addSpirit(spiritid);
        agv_spirits.push_back(a);
    }
}

void BBlock::removeOccu(int agvid,int spiritid)
{
    for(auto itr = agv_spirits.begin();itr!=agv_spirits.end();++itr){
        if(itr->getAgvid() == agvid){
            itr->removeSpirit(spiritid);
            break;
        }
    }
}

bool BBlock::passable(int agvid)
{
    for(auto aos:agv_spirits){
        if(aos.getAgvid() == agvid)continue;
        if(!aos.empty())return false;
    }
    return true;
}

int BBlock::getBlockId()
{
    return block_id;
}

void BBlock::print()
{
    for(auto aos:agv_spirits){
        if(!aos.empty())
        {
            combined_logger->debug("blockid:{0} agv:{1} station_or_lines:{2}",getBlockId(),aos.getAgvid(),aos.getSpiritsStr());
        }
    }
}

BlockManager::BlockManager()
{

}

bool BlockManager::tryAddBlockOccu(std::vector<int> blocks,int agvId,int spiritId)
{
    UNIQUE_LCK(blockMtx);
    if(!blockPassable(blocks,agvId))return false;
    for(auto block:blocks)
    {
        bool alreadAdd = false;
        for(auto itr=bblocks.begin();itr!=bblocks.end();++itr)
        {
            if(itr->getBlockId() == block){
                itr->addOccu(agvId,spiritId);
                alreadAdd = true;
                break;
            }
        }

        if(!alreadAdd){
            BBlock bb(block);
            bb.addOccu(agvId,spiritId);
            bblocks.push_back(bb);
        }
    }

    return true;
}
void BlockManager::freeBlockOccu(std::vector<int> blocks,int agvId, int spiritId)
{
    UNIQUE_LCK(blockMtx);
    for(auto block:blocks)
    {
        for(auto itr=bblocks.begin();itr!=bblocks.end();++itr)
        {
            if(itr->getBlockId() == block){
                itr->removeOccu(agvId,spiritId);
            }
        }
    }
}

bool BlockManager::blockPassable(std::vector<int> blocks, int agvId)
{
    UNIQUE_LCK(blockMtx);
    for(auto block:blocks)
    {
        for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
            if(itr->getBlockId() == block){
                if(!itr->passable(agvId))return false;
            }
        }
    }
    return true;
}

bool BlockManager::noneLockBlockPassable(std::vector<int> blocks, int agvId)
{
    for(auto block:blocks)
    {
        for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
            if(itr->getBlockId() == block){
                if(!itr->passable(agvId))return false;
            }
        }
    }
    return true;
}

void BlockManager::clear()
{
    UNIQUE_LCK(blockMtx);
    bblocks.clear();
}

void BlockManager::printBlock()
{
    UNIQUE_LCK(blockMtx);
    combined_logger->debug("BLOCK===============================================");
    for(auto itr = bblocks.begin();itr!=bblocks.end();++itr){
        itr->print();
    }
    combined_logger->debug("BLOCK===============================================");
}

void BlockManager::test()
{
    std::vector<int> bs;
    bs.push_back(101);
    bs.push_back(102);
    bs.push_back(103);

    std::vector<int> bss;
    bss.push_back(101);
    bss.push_back(103);

    tryAddBlockOccu(bs,1,1001);
    printBlock();
    tryAddBlockOccu(bss,1,1002);
    printBlock();
    freeBlockOccu(bs,1,1002);
    printBlock();
    freeBlockOccu(bss,1,1001);
    printBlock();
}

