#ifndef CONFLICTMANAGER_H
#define CONFLICTMANAGER_H
#include "../common.h"
#include "../utils/noncopyable.h"
#include "conflict.h"

#include <map>

class ConflictManager;
using ConflictManagerPtr = std::shared_ptr<ConflictManager>;

class ConflictManager: public noncopyable, public std::enable_shared_from_this<ConflictManager>
{
public:
    static ConflictManagerPtr getInstance() {
        static ConflictManagerPtr m_inst = ConflictManagerPtr(new ConflictManager());
        return m_inst;
    }

    //agv current excute stations and paths
    //return path that can go now
    void addAgvExcuteStationPath(std::vector<int> spirits,int agvId);

    //upate agv current stations and paths
    //if agv arrive station,free path
    //if agv leave sation,free station
    void removeAgvExcuteStationPath(int spirit,int agvId);

    //
    bool tryAddConflictOccu(int spirit, int agvId);
    void freeConflictOccu(std::vector<int> spirits,int agvId);

    //
    bool conflictPassable(int spiritId,int agvId);
    void clear();
    void printConflict();
    void test();

    void init();
private:
    ConflictManager();

    void calcc(std::vector<int> &agv1spirits,std::vector<int> &agv2spirits,int &i,int &j,std::vector<int> &ic,std::vector<int>&jc);

    std::mutex conflictMtx;
    std::vector<Conflict> conflicts;

    std::mutex stationPathMtx;
    std::map<int,std::vector<int> > stationsPaths;

    std::multimap<int,int> conflict_pairs;

    bool checkConflict(int a,int b);
};

#endif // CONFLICTMANAGER_H
