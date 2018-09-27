#ifndef MAPCONFLICTPAIR_H
#define MAPCONFLICTPAIR_H

#include <list>
#include "mapspirit.h"

class MapConflictPair: public MapSpirit
{
public:
    explicit MapConflictPair(int _id, std::string _name,int _a,int _b);
    virtual ~MapConflictPair();
    virtual MapConflictPair *clone() ;
    MapConflictPair(const MapConflictPair& b) = delete;

    void setA(int _a){a=_a;}
    void setB(int _b){b=_b;}

    int getA(){return a;}
    int getB(){return b;}
private:
    int a;
    int b;

};

#endif // MAPCONFLICTPAIR_H
