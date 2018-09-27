#include "mapconflictpair.h"

MapConflictPair::MapConflictPair(int _id, std::string _name, int _a, int _b):
    MapSpirit(_id,_name,Map_Sprite_Type_Conflict),
    a(_a),
    b(_b)
{

}

MapConflictPair::~MapConflictPair()
{

}
MapConflictPair *MapConflictPair::clone()
{
    MapConflictPair *b = new MapConflictPair(getId(),getName(),getA(),getB());
    return b;
}
