#ifndef CONFLICT_H
#define CONFLICT_H
#include <vector>
#include <atomic>
#include <mutex>

class Conflict
{
public:
    Conflict(int _agvAId, std::vector<int> _agvAspirits, int _agvBId, std::vector<int> _agvBspirits);
    ~Conflict();
    bool tryLock(int agvId, int spirit);
    bool freeLock(int agvId,int spirit);
    bool isAllFree();
    bool passable(int agvId,int spirit);
    void print();
private:
    int agvA;
    int agvB;
    std::vector<int> agvAspirits;
    std::vector<int> agvBspirits;
    int lockedAgv;
    std::mutex lockedAgvMtx;
};

#endif // CONFLICT_H
