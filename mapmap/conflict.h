#ifndef CONFLICT_H
#define CONFLICT_H
#include <vector>
#include <atomic>
#include <mutex>

class Conflict
{
public:
    Conflict(int _agvAId, std::vector<int> _agvAspirits, int _agvBId, std::vector<int> _agvBspirits);
    Conflict(const Conflict&b);
    ~Conflict();
    bool checkLock(int agvId, int spirit);
    bool lock(int agvId, int spirit);
    bool freeLock(int agvId,int spirit);
    bool freeLockExcept(int agvId,int spirit);
    bool isAllFree();
    bool passable(int agvId,int spirit);
    void print();
    Conflict &operator = (const Conflict&b);
    int getAgvA(){return agvA;}
    int getAgvB(){return agvB;}
    std::vector<int> getAspirits(){return agvAspirits;}
    std::vector<int> getBspirits(){return agvBspirits;}
    bool operator ==(const Conflict &b){
        return agvA == b.agvA
                && agvB == b.agvB
                && agvAspirits == b.agvAspirits
                && agvBspirits == b.agvBspirits;
    }
private:
    int agvA;
    int agvB;
    std::vector<int> agvAspirits;
    std::vector<int> agvBspirits;
    int lockedAgv;
    std::mutex lockedAgvMtx;
};

#endif // CONFLICT_H
