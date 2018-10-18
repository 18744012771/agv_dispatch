#ifndef NEWELEVATORMANAGER_H
#define NEWELEVATORMANAGER_H


#include <vector>
#include <memory>
#include <mutex>
#include "../../utils/noncopyable.h"
#include "../../protocol.h"
#include "newelevator.h"
#include "../../network/session.h"
#include "../elevator/elevator_protocol.h"

class NewElevatorManager;
using NewElevatorManagerPtr = std::shared_ptr<NewElevatorManager>;

class NewElevatorManager : public noncopyable,public std::enable_shared_from_this<NewElevatorManager>
{
public:
    using EleParam = lynx::elevator::Param;

    bool init();

    static NewElevatorManagerPtr getInstance(){
        static NewElevatorManagerPtr m_ins = NewElevatorManagerPtr(new NewElevatorManager());
        return m_ins;
    }

    ~NewElevatorManager();

	void setEnable(int id,bool enable);

    lynx::elevator::Param create_param(int cmd, int from_floor,int to_floor, int elevator_id, int agv_id);

    void resetElevatorState(int elevator_id);
    void onRead(int ele_id, std::string ele_name, const char *data, int len);

    NewElevator *getEleById(int id);

    void notify(const lynx::elevator::Param& p);

    void test();

	std::vector<NewElevator *> getAllEles() { return eles; }

	void interSetEnableELE(SessionPtr conn, const Json::Value &request);
private:

    void ttest(int agv_id,int from_floor,int to_floor,int elevator_id);
    NewElevatorManager();

    void checkTable();
    std::vector<NewElevator *> eles;
};

#endif // NEWELEVATORMANAGER_H
