#ifndef ELEVATORMANAGER_H
#define ELEVATORMANAGER_H

#include <vector>
#include <memory>
#include <mutex>
#include <boost/noncopyable.hpp>
#include "../../protocol.h"
#include "../../network/clientsession.h"
#include "elevator.h"

class Elevator;
using ElevatorPtr = std::shared_ptr<Elevator>;

class ElevatorManager;
using ElevatorManagerPtr = std::shared_ptr<ElevatorManager>;

class ElevatorManager: public boost::noncopyable,public std::enable_shared_from_this<ElevatorManager>
{
public:
    bool init();

    static ElevatorManagerPtr getInstance(){
        static ElevatorManagerPtr m_ins = ElevatorManagerPtr(new ElevatorManager());
        return m_ins;
    }

    ElevatorPtr getElevatorById(int id);

    ElevatorManager();
    ~ElevatorManager();

private:
    void checkTable();
    std::vector<ElevatorPtr> elevators;
};

#endif // ELEVATORMANAGER_H

