#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H
#include "../common.h"
#include <boost/noncopyable.hpp>

class BlockManager;
using BlockManagerPtr = std::shared_ptr<BlockManager>;
class GroupManager;
using GroupManagerPtr = std::shared_ptr<GroupManager>;

class AgvOccuSpirits{
public:
    AgvOccuSpirits(int _agvid);
    AgvOccuSpirits(const AgvOccuSpirits & b);

    AgvOccuSpirits &operator =(const AgvOccuSpirits &b);

    void addSpirit(int spiritId);
    void removeSpirit(int spiritId);

    bool empty();
    int getAgvid();
    std::string getSpiritsStr();
private:
    int agvid;
    std::vector<int> spirits;
};

class GGroup{
public:
    GGroup(int bid);
    GGroup(const GGroup & b);

    GGroup &operator =(const GGroup &b);

    void addOccu(int agvid,int spiritid);
    void removeOccu(int agvid,int spiritid);

    bool passable(int agvid);

    int getGroupId();

    void print();
private:
    int group_id;
    std::vector<AgvOccuSpirits> agv_spirits;
};


class GroupManager: public boost::noncopyable, public std::enable_shared_from_this<GroupManager>
{
public:
    static GroupManagerPtr getInstance() {
        static GroupManagerPtr m_inst = GroupManagerPtr(new GroupManager());
        return m_inst;
    }

    bool tryAddGroupOccu(std::vector<int> groups,int agvId,int spiritId);
    void freeGroupOccu(std::vector<int> groups,int agvId, int spiritId);

    bool groupPassable(std::vector<int> groups, int agvId);
    void clear();
    void print();
    //void test();
private:
    GroupManager();
    bool noneLockGroupPassable(std::vector<int> groups, int agvId);
    std::mutex groupMtx;
    std::vector<GGroup> ggroups;
};



class BBlock{
public:
    BBlock(int bid);
    BBlock(const BBlock & b);

    BBlock &operator =(const BBlock &b);

    void addOccu(int agvid,int spiritid);
    void removeOccu(int agvid,int spiritid);

    bool passable(int agvid);

    int getBlockId();

    void print();
private:
    int block_id;
    std::vector<AgvOccuSpirits> agv_spirits;
};

class BlockManager: public boost::noncopyable, public std::enable_shared_from_this<BlockManager>
{
public:
    static BlockManagerPtr getInstance() {
        static BlockManagerPtr m_inst = BlockManagerPtr(new BlockManager());
        return m_inst;
    }

    bool tryAddBlockOccu(std::vector<int> blocks,int agvId,int spiritId);
    void freeBlockOccu(std::vector<int> blocks,int agvId, int spiritId);

    bool blockPassable(std::vector<int> blocks, int agvId);
    void clear();
    void printBlock();
    void test();
private:
    BlockManager();
    bool noneLockBlockPassable(std::vector<int> blocks, int agvId);
    std::mutex blockMtx;
    std::vector<BBlock> bblocks;
};

#endif // BLOCKMANAGER_H
