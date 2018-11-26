#include "dyforklift.h"
#include "../common.h"
#include "../mapmap/mappoint.h"
#include "../device/elevator/elevator.h"
#include "charge/chargemachine.h"
#include "../device/elevator/elevatorManager.h"
#include "../mapmap/blockmanager.h"
#include "../mapmap/conflictmanager.h"
#include "../device/new_elevator/newelevator.h"
#include "../device/new_elevator/newelevatormanager.h"
#include "../msgprocess.h"
#include "../taskmaker.h"
#define RESEND
//#define HEART


DyForklift::DyForklift(int id, std::string name, std::string ip, int port) :
    Agv(id, name, ip, port),
    m_qTcp(nullptr),
    firstConnect(true),
    batteryTemprature(0)
{
    //    startpoint = -1;
    //    actionpoint = -1;
    pauseFlag = false;
    sendPause = false;
    status = Agv::AGV_STATUS_UNCONNECT;
    init();
    //    充电机测试代码
    //    unsigned char charge_id = 1;
    //    chargemachine cm(charge_id, "charge_machine", "127.0.0.1", 5566);
    //    cm.init();
    //    cm.chargeControl(charge_id, CHARGE_START);
    //    cm.chargeQuery(charge_id, 0x01);
}

void DyForklift::init() {

#ifdef RESEND
    g_threads.create_thread([&] {
        while (!g_quit) {
            if (m_qTcp == nullptr || !m_qTcp->alive()) {
                //已经掉线了
                sleep_for_s(1);
                continue;
            }
            unRecvMsgMtx.lock();

            for (auto iter = m_unRecvSend.begin(); iter != m_unRecvSend.end(); iter++)
            {
                //TODO 判断连接是否有效
                if (m_qTcp!=nullptr && m_qTcp->alive())
                {
                    m_qTcp->write(iter->second.c_str(), iter->second.length());
                }
            }
            unRecvMsgMtx.unlock();

            sleep_for_us(1000000);
        }
    });
#endif
}

void DyForklift::onTaskStart(AgvTaskPtr _task)
{
    if (_task != nullptr)
    {
        status = Agv::AGV_STATUS_TASKING;
    }
}

void DyForklift::onTaskFinished(AgvTaskPtr _task)
{

}

bool DyForklift::resend(const std::string &msg) {
    if (msg.length() <= 0)return false;
    return send(msg);
}

bool DyForklift::fork(int params)
{
    m_lift = (params != 0) ? true : false;
    std::stringstream body;
    body << FORKLIFT_FORK;
    body.width(2);
    body.fill('0');
    body << params;
    return resend(body.str());
}

void DyForklift::heart()
{
    std::stringstream body;
    body.fill('0');
    body.width(2);
    body<<FORKLIFT_HEART;
    send(body.str());
}

int DyForklift::nearestStation()
{
    int minDis = -1;
    int min_station = -1;
    auto mapmanager = MapManager::getInstance();
    std::vector<int> stations = mapmanager->getStations(floor);
    for (auto station : stations) {
        MapPoint *point = mapmanager->getPointById(station);
        if (point == nullptr)continue;
        long dis = func_dis(x,y,point->getRealX(),point->getRealY());
        if ((min_station == -1 || minDis > dis) && func_angle(-point->getRealA() / 10, (int)theta) < 20)
        {
            minDis = dis;
            min_station = point->getId();
        }
    }

    if(minDis > PRECISION - ALLOWANVE){
        return -1;
    }

    return min_station;
}

//解析小车上报消息
void DyForklift::onRead(const char *data, int len)
{
    if (data == NULL || len < 10)return;

    if (firstConnect)
    {
        firstConnect = false;
        std::stringstream body;
        body << FORKLIFT_MOVE_NOLASER;
        body << FORKLIFT_NOLASER_CLEAR_TASK;
        send(body.str());
    }

    std::string msg(data, len);
    int length = stringToInt(msg.substr(6, 4));
    if (length != len && length < 12)
    {
        return;
    }

    int mainMsg = stringToInt(msg.substr(10, 2));
    std::string body = msg.substr(12);

    int timestamp = stringToInt(msg.substr(0,6));
    if(timestamp%5==0)heart();

    //check received already
    for(auto itr = receivedMsgs.begin();itr!=receivedMsgs.end();++itr){
        if(*itr == timestamp){
            //received already
            return ;
        }
    }

    if (FORKLIFT_POS != mainMsg && FORKLIFT_WARN != mainMsg)
    {
        combined_logger->info("agv{0} recv data:{1}", id, data);

        receivedMsgs.push_back(timestamp);
        while(receivedMsgs.size()>30){
            receivedMsgs.pop_front();
        }
    }

    //解析小车发送的消息
    switch (mainMsg) {
    case FORKLIFT_POS:
        //小车上报的位置信息
    {
        //*1234560031290|0|0|0.1,0.2,0.3,4#
        std::vector<std::string> all = split(body, "|");
        if (all.size() == 4)
        {
            //status = stringToInt(all[0]);
            //任务线程需要根据此状态判断小车是否在执行任务，不能赋值
            std::vector<std::string> temp = split(all[3], ",");
            if (temp.size() == 4)
            {

                double x_new = 100 * stringToDouble(temp[0]);
                double y_new = -100 * stringToDouble(temp[1]);
                double theta_new = -57.3 * stringToDouble(temp[2]);

                if(func_dis(x_new,y_new,x,y)>=100|| func_angle(theta_new,theta)>30){
                    combined_logger->debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!tiaobian agv{0}!!!!!!!!!!!!!!!!!!1",getId());
                    combined_logger->debug("oldX={0},oldy={1},old_theta={2}",x,y,theta);
                    combined_logger->debug("newX={0},newY={1},new_theta={2}",x_new,y_new,theta_new);
                    combined_logger->debug("recv data={}",std::string(data,len));
                    combined_logger->debug("recv data={}",data,len);
                }

                x = x_new;
                y = y_new;
                theta = theta_new;
                floor = stringToInt(temp[3]);

                if (AGV_STATUS_NOTREADY == status)
                {
                    //find nearest station
                    nowStation = nearestStation();
                    if (nowStation != -1) {
                        status = AGV_STATUS_IDLE;
                        onArriveStation(nowStation);
                    }
                }
                else
                {
                    arrve();
                }
            }
        }

        break;
    }
    case FORKLIFT_WARN:
    {
        std::vector<std::string> warn = split(body, "|");
        WarnSt tempWarn;
        if (warn.size() != 14)
        {
            break;
        }
        tempWarn.iLocateSt = stringToInt(warn.at(0));            //定位丢失,1表示有告警，0表示无告警
        tempWarn.iSchedualPlanSt = stringToInt(warn.at(1));             //调度系统规划错误,1表示有告警，0表示无告警
        tempWarn.iHandCtrSt = stringToInt(warn.at(2)); 	            //模式切换（集中调度，手动），0表示只接受集中调度，1表示只接受手动控制
        tempWarn.iObstacleLaserSt = stringToInt(warn.at(3));           //激光避障状态 0： 远 1： 中 2：近
        tempWarn.iScramSt = stringToInt(warn.at(4));           //紧急停止状态 0:非 1：紧急停止
        tempWarn.iTouchEdgeSt = stringToInt(warn.at(5));       //触边状态    0：正常 1：触边
        tempWarn.iTentacleSt = stringToInt(warn.at(6));        //触须状态    0：正常  1：触须
        tempWarn.iTemperatureSt = stringToInt(warn.at(7));     //温度
        tempWarn.iBaffleSt = stringToInt(warn.at(8));          //挡板状态    0：正常  1：异常
        tempWarn.iLocationLaserConnectSt = stringToInt(warn.at(9));         //定位激光连接状态    0： 正常 1:异常
        tempWarn.iObstacleLaserConnectSt = stringToInt(warn.at(10));         //避障激光连接状态    0： 正常 1：异常
        tempWarn.iSerialPortConnectSt = stringToInt(warn.at(11));            //串口连接状态       0： 正常 1：异常
        tempWarn.iBatteryA = stringToInt(warn.at(12));
        tempWarn.iBatteryV = stringToInt(warn.at(13));

        //combined_logger->info("agv{1} warn changed:{0}", tempWarn.toString(), id);
        batteryTemprature = tempWarn.iTemperatureSt;
        isChanging = (tempWarn.iBatteryA > 0);
        isManualControl = (tempWarn.iHandCtrSt ==1);
        isEmergencyStop = (tempWarn.iScramSt == 1);

        static int powerLowTimes = 0;
        static int powerHighTimes = 0;

        if((tempWarn.iBatteryV < 25*(1000))){
            powerLowTimes++;
            powerHighTimes = 0;
        }else{
            powerHighTimes++;
            powerLowTimes = 0;
        }

        if(powerLowTimes>1000){
            powerLowTimes = 50;
        }
        if(powerHighTimes>1000){
            powerHighTimes = 50;
        }

        if(!isPowerLow &&powerLowTimes>=20){
            isPowerLow = true;
        }else if(isPowerLow && powerHighTimes>=20){
            isPowerLow = false;
        }

        //set status
        if(isChanging){
            status = AGV_STATUS_CHARGING;
        }else if(isEmergencyStop){
            status = AGV_STATUS_ESTOP;
        }else if(isManualControl){
            status = AGV_STATUS_HANDING;
        }else if(isPowerLow && status != AGV_STATUS_TASKING && status != AGV_STATUS_GO_CHARGING&&status!=AGV_STATUS_CHARGING){
            status = AGV_STATUS_POWER_LOW;
            //TODO: zidong chongdian
            bool ret = TaskMaker::getInstance()->makeChargeTask(getId());
            if(ret){
                status = AGV_STATUS_GO_CHARGING;
            }
        }else{
            if(status == AGV_STATUS_CHARGING || status == AGV_STATUS_ESTOP || status == AGV_STATUS_HANDING ||status == AGV_STATUS_POWER_LOW){
                //recover status!!!
                if(currentTask!=nullptr && !currentTask->getIsCancel()){
                    status = AGV_STATUS_TASKING;
                }else{
                    status = AGV_STATUS_IDLE;
                }
            }
        }
        break;
    }
    case FORKLIFT_BATTERY:
    {
        //小车上报的电量信息
        if (body.length() > 0)
            m_power = stringToInt(body);
        break;
    }
    case FORKLIFT_FINISH:
    {
        //小车上报运动结束状态或自定义任务状态
        if (body.length() < 2)return;
        if (1 == stringToInt(body.substr(2)))
        {
            //command finish
            UNIQUE_LCK lck(unFinishMsgMtx);
            auto iter = m_unFinishCmd.find(stringToInt(body.substr(0, 2)));
            if (iter != m_unFinishCmd.end())
            {
                m_unFinishCmd.erase(iter);
            }
        }
        break;
    }
    case FORKLIFT_MOVE:
    case FORKLIFT_FORK:
    case FORKLIFT_STARTREPORT:
    case FORKLIFT_MOVEELE:
    case FORKLIFT_CHARGE:
    {
        unRecvMsgMtx.lock();
        //command response
        auto iter = m_unRecvSend.find(stringToInt(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        unRecvMsgMtx.unlock();
        break;
    }
    case FORKLIFT_INITPOS:
    {
        unRecvMsgMtx.lock();
        //command response
        auto iter = m_unRecvSend.find(stringToInt(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        unRecvMsgMtx.unlock();
        if (body.length() <= 0)return;
        if (1 == stringToInt(body))
        {
            MsgProcess::getInstance()->notifyAll(ENUM_NOTIFY_INIT_POSITION_OK);
        }else{
            MsgProcess::getInstance()->notifyAll(ENUM_NOTIFY_INIT_POSITION_FAIL);
        }

        break;
    }
    case FORKLIFT_MOVE_NOLASER:
    {
        unRecvMsgMtx.lock();
        //command response
        auto iter = m_unRecvSend.find(stringToInt(msg.substr(0, 6)));
        if (iter != m_unRecvSend.end())
        {
            m_unRecvSend.erase(iter);
        }
        unRecvMsgMtx.unlock();

        //TODO
        pauseFlag = (bool)sendPause;
        break;
    }
    default:
        break;
    }
}
//判断小车是否到达某一站点
void DyForklift::arrve() {

    auto mapmanagerptr = MapManager::getInstance();

    //1.does leave station
    if (nowStation > 0) {
        //how far from current pos to nowStation
        auto point = mapmanagerptr->getPointById(nowStation);
        if (point != nullptr)
        {
            if (func_dis(x, y, point->getRealX(), point->getRealY()) > PRECISION + ALLOWANVE) {
                //too far leave station
                onLeaveStation(nowStation);
            }
        }
    }

    //2.did agv arrive a station
    int arriveId = -1;
    double minDis = DISTANCE_INFINITY_DOUBLE;

    if(currentTask != nullptr && !currentTask->getIsCancel())
    {
        stationMtx.lock();
        for (auto station : excutestations) {
            MapPoint *point = mapmanagerptr->getPointById(station);
            if (point == nullptr)continue;

            if (mapmanagerptr->getFloor(station) != floor)continue;

            auto dis = func_dis(x, y, point->getRealX(), point->getRealY());
            if (dis < minDis && dis < PRECISION - ALLOWANVE && station != nowStation && func_angle(-point->getRealA() / 10, (int)theta) < ANGLE_PRECISION) {
                minDis = dis;
                arriveId = station;
            }

            if (station == currentEndStation)
            {
                //已抵达当前路径终点，退出判断
                break;
            }
        }
        stationMtx.unlock();
        if (arriveId != -1) {
            onArriveStation(arriveId);
        }
    }else if(status != AGV_STATUS_TASKING){
        int tempStation = nearestStation();
        if(nowStation == tempStation){

        }else{
            nowStation = tempStation;
            combined_logger->debug("========================agv{0} arrive station{1} without task!!!!!!!!!",getId(),nowStation);
            //occu the station and release others!
            if(nowStation>0){
                mapmanagerptr->addOccuStation(nowStation,shared_from_this());
            }
            mapmanagerptr->freeAllStationLines(shared_from_this(),nowStation);
        }
    }



    //3.check conflict occu current station or path!
    if (!sendPause && !pauseFlag){
        if(currentTask != nullptr && !currentTask->getIsCancel()){
            if(nowStation>0){
                if(!ConflictManager::getInstance()->conflictPassable(nowStation,getId())){
                    combined_logger->debug("agv {0} check conflick now Station need paused! nowStation = {1},lastStation = {2},nextStation={3}",getId(),nowStation,lastStation,nextStation);
                    pause();
                }

            }else{
                auto nowPath = mapmanagerptr->getPathByStartEnd(lastStation,nextStation);
                if(nowPath!=nullptr){
                    if(!ConflictManager::getInstance()->conflictPassable(nowPath->getId(),getId())){
                        combined_logger->debug("agv {0} check conflick now Path need paused! nowStation = {1},lastStation = {2},nextStation={3}",getId(),nowStation,lastStation,nextStation);
                        pause();
                    }
                }
            }
        }
    }
}

//执行路径规划结果
void DyForklift::excutePath(std::vector<int> lines)
{
    auto conflictmanagerptr = ConflictManager::getInstance();
    auto mapmanagerptr = MapManager::getInstance();
    combined_logger->info("dyForklift{0} excutePath", id);

    std::vector<int> spirits;
    std::stringstream ss;
    stationMtx.lock();
    excutestations.clear();
    excutespaths.clear();
    excutestations.push_back(nowStation);
    for (auto line : lines) {
        spirits.push_back(line);
        MapPath *path =  mapmanagerptr->getPathById(line);
        if(path == nullptr)continue;
        int endId = path->getEnd();
        spirits.push_back(endId);
        excutestations.push_back(endId);
        excutespaths.push_back(line);
        ss << path->getName() << "  ";
    }
    stationMtx.unlock();

    conflictmanagerptr->addAgvExcuteStationPath(spirits, getId());

    combined_logger->info("excutePath: {0}", ss.str());
    //    actionpoint = 0;
    //MapPoint *startstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.front()));
    //MapPoint *endstation = static_cast<MapPoint *>(MapManager::getInstance()->getMapSpiritById(excutestations.back()));
    //    startpoint = startstation->getId();
    //TODO
    //    startLift = true;

    AgvTaskPtr currentTask = this->getTask();

    task_type = currentTask->getTaskNodes().at(currentTask->getDoingIndex())->getType();
    //    combined_logger->info("taskType: {0}", task_type);

    //    switch (task_type) {
    //    case TASK_PICK:
    //    {
    //        //TODO
    //        //多层楼需要修改

    //        if(func_dis(startstation->getX(), startstation->getY(), endstation->getX(), endstation->getY()) < START_RANGE*2)
    //        {

    //            startLift = true;
    //        }
    //        actionpoint = startstation->getId();

    //        double dis = 0;
    //        for (int i = lines.size()-1; i >= 0; i--) {
    //            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(-lines.at(i));
    //            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_DyPath)continue;

    //            DyMapPath *path = static_cast<DyMapPath *>(spirit);
    //            dis += func_dis(path->getP2x(), path->getP2y(), path->getP1x(), path->getP1y());
    //            if(dis < PRECMD_RANGE)
    //            {
    //                actionpoint = path->getEnd();
    //            }
    //            else
    //            {
    //                if(func_dis(path->getP2x(), path->getP2y(), startstation->getRealX(), startstation->getRealY()) > START_RANGE)
    //                {
    //                    actionpoint = path->getEnd();
    //                }
    //                break;
    //            }
    //        }
    //        combined_logger->info("action station:{0}", actionpoint);

    //        actionFlag = false;
    //        break;
    //    }
    //    default:
    //        actionFlag = true;
    //        break;
    //    }
    //for test
    //    startLift = true;
    //告诉小车接下来要执行的路径
    //    if(0)
    bool needElevator = isNeedTakeElevator(lines);
    if (!needElevator)//不需要乘电梯
    {
        combined_logger->info("DyForklift, excutePath, don't need to take elevator");
        //告诉小车接下来要执行的路径
        goSameFloorPath(lines);
    }//需要乘电梯
    else
    {
        goElevator(lines);
    }
}

void DyForklift::goSameFloorPath(const std::vector<int> lines)
{
    std::vector<int> exelines;
    auto mapmanager = MapManager::getInstance();

    double speed = 0;

    for (auto line : lines) {
        auto path = mapmanager->getPathById(line);
        if (path == nullptr)continue;

        if (speed == 0) {
            exelines.push_back(line);
            speed = path->getSpeed();
        }
        else if (speed * path->getSpeed() < 0) {
            goStation(exelines, true, FORKLIFT_MOVE);
            exelines.clear();
            speed = path->getSpeed();
            exelines.push_back(line);
        }
        else {
            exelines.push_back(line);
        }
    }

    if (exelines.size() > 0) {
        goStation(exelines, true, FORKLIFT_MOVE);
    }
}
//上电梯
void DyForklift::goElevator(const std::vector<int> lines)
{
    auto mapmanagerptr = MapManager::getInstance();
    //duoci chengzuo dianti de wenti s
    int agv_id = getId();
    int index = 0;
    while (!g_quit && currentTask!=nullptr &&  !currentTask->getIsCancel() && index < lines.size())
    {
        std::vector<int> lines_to_elevator;//到达电梯口路径
        std::vector<int> lines_enter_elevator;//进入电梯路径
        std::vector<int> lines_take_elevator;//乘电梯到达楼层
        std::vector<int> lines_out_elevator;//出电梯路径
        std::vector<int> lines_leave_elevator;//离开电梯口去目标路径
        index = getPathToElevator(lines, index, lines_to_elevator);
        index = getPathEnterElevator(lines, index, lines_enter_elevator);
        index = getPathInElevator(lines, index, lines_take_elevator);
        index = getPathOutElevator(lines, index, lines_out_elevator);
        index = getPathLeaveElevator(lines, index, lines_leave_elevator);

        if(lines_take_elevator.size()<=0){
            combined_logger->error("lines_take_elevator size <=0!!!!!");
            return ;
        }

        int from_floor = floor;
        if(from_floor <=0 )
        {
            combined_logger->error("current floor {}<=0, can not take elevator!",from_floor);
            return ;
        }

        int elevator_line = lines_take_elevator.at(0);
        MapPath *elevator_path =  mapmanagerptr->getPathById(elevator_line);
        if(elevator_path == nullptr){
            combined_logger->error("elevator path {} == nullptr, can not take elevator!",elevator_line);
            return ;
        }

        MapPoint * elevator_point = mapmanagerptr->getPointById(elevator_path->getStart());
        if(elevator_point == nullptr){
            combined_logger->error("elevator_point {} == nullptr, can not take elevator!",elevator_path->getStart());
            return ;
        }

        combined_logger->info("DyForklift,  excutePath, elevatorID: {0}", elevator_point->getLineId());


        int elevator_back = lines_take_elevator.back();

        MapPath *elevator_path_back = mapmanagerptr->getPathById(elevator_back);
        if(elevator_path_back == nullptr){
            combined_logger->error("elevator_path_back {} == nullptr, can not take elevator!",elevator_back);
            return ;
        }

        int endId = elevator_path_back->getEnd();

        //根据电梯等待区设置呼叫某个ID电梯
        NewElevatorManagerPtr elemanagerptr = NewElevatorManager::getInstance();
        NewElevator *elevator = elemanagerptr->getEleById(stringToInt(elevator_point->getLineId()));

        if (elevator == nullptr)
        {
            combined_logger->error("DyForklift,  excutePath, no elevator!!!!!");
            return;
        }

        if (elevator->getOccuAgvId() > 0 && elevator->getOccuAgvId() != getId()) {
            combined_logger->error("DyForklift, excutePath,elevator  occu!!!!!");
            return;
        }

        int elevator_id = elevator->getId();

        int to_floor = MapManager::getInstance()->getFloor(endId);

        combined_logger->info("DyForklift,  excutePath, endId: {0}", endId);

        combined_logger->info("DyForklift,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);


        if (lines_to_elevator.size() > 0)
        {
            goSameFloorPath(lines_to_elevator);//到达电梯口
        }

        elevator->setOccuAgvId(agv_id);

        elevator->setResetOk(false);

        while(!g_quit && currentTask != nullptr && !currentTask->getIsCancel()){
            elemanagerptr->resetElevatorState(elevator_id);
            sleep_for_s(3);
            if (elevator->getResetOk())
                break;
        }

        combined_logger->info("DyForklift,  excutePath, from_floor: {0}, to_floor: {1}", from_floor, to_floor);

        //呼梯
        elevator->setCurrentOpenDoorFloor(-1);
        while (!g_quit &&currentTask!=nullptr && !currentTask->getIsCancel()) {
            sleep_for_s(1);
            elemanagerptr->call(elevator_id, from_floor);
            sleep_for_s(1);
            elemanagerptr->queryElevatorState(elevator_id);
            if (elevator->getCurrentOpenDoorFloor() == from_floor)break;
        }
        
        if (g_quit || currentTask == nullptr ||  currentTask->getIsCancel()) return;

        //进电梯
        combined_logger->debug("==============START go into elevator");

        elemanagerptr->KeepOpen(from_floor, elevator_id);

        sleep_for_s(5); //等待电梯门完全打开

        if (lines_enter_elevator.size() > 0)
        {
            goStation(lines_enter_elevator, true, FORKLIFT_MOVEELE);//进入电梯
        }

        combined_logger->debug("==============FINISH go into elevator");

        //关门
        elemanagerptr->DropOpen(elevator_id);
        sleep_for_s(2);

        elemanagerptr->resetElevatorState(elevator_id);
        while (!g_quit && currentTask!=nullptr &&   !currentTask->getIsCancel()) {
            sleep_for_s(2);
            if(elevator->getResetOk())break;
            elemanagerptr->resetElevatorState(elevator_id);
        }

        if (g_quit || currentTask == nullptr|| currentTask->getIsCancel()) return;
        sleep_for_s(5);
        //呼梯
        elemanagerptr->call(elevator_id, to_floor);
        while (!g_quit && currentTask!=nullptr &&   !currentTask->getIsCancel()) {
            sleep_for_s(1);
            if (elevator->getCurrentOpenDoorFloor() == to_floor)break;
            sleep_for_s(1);
            //重新呼叫
            elemanagerptr->call(elevator_id, to_floor);
        }

        if (g_quit || currentTask == nullptr ||  currentTask->getIsCancel()) return;

        tellAgvPos(endId);

        //出电梯
        combined_logger->debug("==============START go out elevator");
        elemanagerptr->KeepOpen(to_floor, elevator_id);
        sleep_for_s(5);//等待电梯门完全打开
        goStation(lines_out_elevator, true, FORKLIFT_MOVEELE);

        combined_logger->debug("==============FINISH go out elevator");

        //关门
        elemanagerptr->DropOpen(elevator_id);
        sleep_for_s(2);
        elemanagerptr->resetElevatorState(elevator_id);

        if (g_quit || currentTask == nullptr ||  currentTask->getIsCancel()) return;

        sleep_for_s(2);

        if (lines_out_elevator.size() > 0)
        {
            int path_id = lines_out_elevator.at(lines_out_elevator.size() - 1);
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(path_id);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            {
                combined_logger->error("DyForklift,  excutePath, !MapSpirit::Map_Sprite_Type_Path");
                return;
            }

            MapPath *path = static_cast<MapPath *>(spirit);

            this->nowStation = path->getEnd();
            combined_logger->debug("DyForklift,  excutePath, AGV出电梯, nowStation: {0}", nowStation);

            this->lastStation = 0;
            this->nextStation = 0;
        }

        elevator->setOccuAgvId(0);

        if (lines_leave_elevator.size() > 0)
        {
            combined_logger->debug("DyForklift,  excutePath, 离开电梯口去目标");
            goSameFloorPath(lines_leave_elevator);//离开电梯口去目标
        }

        if (index + 1 >= lines.size())break;
    }
    combined_logger->info("excute path finish");
}

//移动至指定站点
void DyForklift::goStation(std::vector<int> lines, bool stop, FORKLIFT_COMM cmd)
{
    if (g_quit || currentTask == nullptr || currentTask->getIsCancel())return;

    MapPoint *start;
    MapPoint *end;

    MapManagerPtr mapmanagerptr = MapManager::getInstance();
    auto conflictmanagerptr = ConflictManager::getInstance();
    //BlockManagerPtr blockmanagerptr = BlockManager::getInstance();

    combined_logger->info("dyForklift goStation");
    std::stringstream body;

    int endId = 0;
    bool firstPathEnd = false;
    for (auto line : lines) {
        MapPath *path = mapmanagerptr->getPathById(line);
        if (path == nullptr)continue;
        int startId = path->getStart();
        endId = path->getEnd();
        start = mapmanagerptr->getPointById(startId);
        if (start == nullptr)continue;
        end = mapmanagerptr->getPointById(endId);
        if (end == nullptr)continue;

        if(!firstPathEnd){
            nextStation = endId;
            firstPathEnd = true;
        }

        float speed = path->getSpeed();
        if (!body.str().length())
        {
            if (cmd == FORKLIFT_MOVEELE)
            {
                body << cmd << "|" << speed << "," << end->getRealX() / 100.0 << "," << -end->getRealY() / 100.0 << "," << end->getRealA() / 10.0 << "," << mapmanagerptr->getFloor(endId) << ",";
            }
            else
            {
                //add current pos
                body << cmd << "|" << speed << "," << x/100.0 << "," << y/-100.0 << "," << -theta << "," << floor << ",";
                //            \<<"|"<<speed<<dy_path->getP1x()/100.0<<","<<dy_path->getP1y()/100.0<<","<<dy_path->getP1a()<<","<<dy_path->getP1f()<<","<<dy_path->getPathType();
            }
        }
        int type = 1;
        if (path->getPathType() == MapPath::Map_Path_Type_Quadratic_Bezier
                || path->getPathType() == MapPath::Map_Path_Type_Cubic_Bezier)type = 3;
        if (path->getPathType() == MapPath::Map_Path_Type_Between_Floor)type = 1;
        body << type << "|" << speed << "," << end->getRealX() / 100.0 << "," << -end->getRealY() / 100.0 << "," << end->getRealA() / 10.0 << "," << mapmanagerptr->getFloor(endId) << ",";
    }
    body << "1";

    currentEndStation = endId;
    combined_logger->debug(" agv {0} check can go before go!...",id);
    while (!g_quit && currentTask != nullptr && !currentTask->getIsCancel()) {
        sleep_for_us(500000);
        bool canGo = true;
        if (nowStation > 0) {
            if (!conflictmanagerptr->tryAddConflictOccu(nowStation, getId())) {
                canGo = false;
            }
        }
        else {
            if(lastStation>0){
                auto path = mapmanagerptr->getPathByStartEnd(lastStation, nextStation);
                if (path != nullptr) {
                    if (!conflictmanagerptr->tryAddConflictOccu(path->getId(), getId())) {
                        canGo = false;
                    }
                }else{
                    if (!conflictmanagerptr->tryAddConflictOccu(lastStation, getId())) {
                        canGo = false;
                    }
                }
            }
        }

        if (!canGo)continue;

        //could occu next station or path block?
        int iip;
        if (nowStation > 0) {
            int pId = -1;
            stationMtx.lock();
            for (int i = 0; i < excutespaths.size(); ++i) {
                auto path = mapmanagerptr->getPathById(excutespaths[i]);
                if (path != nullptr && path->getStart() == nowStation) {
                    pId = path->getId();
                    break;
                }
            }
            stationMtx.unlock();
            iip = pId;
        }
        else {
            iip = nextStation;
        }

        if (!conflictmanagerptr->conflictPassable(iip, getId())) {
            canGo = false;
        }
        else {
            canGo = conflictmanagerptr->tryAddConflictOccu(iip, getId());
        }
        if (canGo)
            break;
    }

    if (g_quit || currentTask == nullptr || currentTask->getIsCancel())return;
    combined_logger->debug(" agv {0} check can go finish,GO!...");
    resend(body.str());

    do
    {
        //wait for move finish
        sleep_for_us(50000);

        //如果中途因为block被暂停了，那么就判断block是否可以进入了，如果可以进入，那么久要发送resume
        if (sendPause || pauseFlag)
        {
            sleep_for_s(1);

            bool canResume = true;
            //could occu current station or path block?
            //combined_logger->debug("agv {0} check conflick can resume! nowStation = {1},lastStation = {2},nextStation={3}",getId(),nowStation,lastStation,nextStation);
            if (nowStation > 0) {
                auto nowStationPtr = mapmanagerptr->getPointById(nowStation);
                if (!conflictmanagerptr->tryAddConflictOccu(nowStation, getId())) {
                    combined_logger->debug("agv {0} paused,check current station {1},result={2}!",id,nowStationPtr->getName(),false);
                    canResume = false;
                }else{
                    //combined_logger->debug("agv {0} paused,check current station {1},result={2}!",id,nowStationPtr->getName(),true);
                }
            }
            else {
                if(lastStation>0){
                    auto path = mapmanagerptr->getPathByStartEnd(lastStation, nextStation);
                    if (path != nullptr) {
                        if (!conflictmanagerptr->tryAddConflictOccu(path->getId(), getId())) {
                            combined_logger->debug("agv {0} paused,check current line {1},result={2}!",getId(),path->getName(),false);
                            canResume = false;
                        }else{
                            //combined_logger->debug("agv {0} paused,check current line {1},result={2}!",getId(),path->getName(),true);
                        }
                    }else{
                        auto lastStationPtr = mapmanagerptr->getPointById(lastStation);
                        if (!conflictmanagerptr->tryAddConflictOccu(lastStation, getId())) {
                            combined_logger->debug("agv {0} paused,check last station {1},result={2}!",id,lastStationPtr->getName(),false);
                            canResume = false;
                        }else{
                            //combined_logger->debug("agv {0} paused,check last station {1},result={2}!",id,lastStationPtr->getName(),true);
                        }
                    }
                }
            }

            if (!canResume)continue;

            //could occu next station or path block?
            int iip;
            if (nowStation > 0) {
                int pId = -1;
                stationMtx.lock();
                for (int i = 0; i < excutespaths.size(); ++i) {
                    auto path = mapmanagerptr->getPathById(excutespaths[i]);
                    if (path != nullptr && path->getStart() == nowStation) {
                        pId = path->getId();
                        break;
                    }
                }
                stationMtx.unlock();
                iip = pId;
            }
            else {
                iip = nextStation;
            }

            if (!conflictmanagerptr->conflictPassable(iip, getId())) {
                combined_logger->debug("agv {0} paused,check next station or path {1},result={2}!",id,iip,false);
                canResume = false;
            }
            else {
                //combined_logger->debug("agv {0} paused,check next station {1},result={2}!",id,iip,true);
                canResume = conflictmanagerptr->tryAddConflictOccu(iip, getId());
            }
            if (canResume)
                resume();

            if (g_quit || currentTask == nullptr || currentTask->getIsCancel())return;
        }
        
        if (g_quit || currentTask == nullptr || currentTask->getIsCancel())return;

    } while (this->nowStation != endId || !isFinish());
    combined_logger->info("nowStation = {0}, endId = {1}", this->nowStation, endId);
}

void DyForklift::checkCanGo()
{
    //


}


void DyForklift::checkNeedPause()
{
    //


}


void DyForklift::checkCanResume()
{
    //
}

void DyForklift::setQyhTcp(AgvSessionPtr _qyhTcp)
{
    if (_qyhTcp == nullptr) {
        if(status == AGV_STATUS_NOTREADY|| status == Agv::AGV_STATUS_IDLE || status == Agv::AGV_STATUS_TASKING )
            status = Agv::AGV_STATUS_UNCONNECT;
        m_qTcp = nullptr;
    }else{
        m_qTcp = _qyhTcp;
        if(status == Agv::AGV_STATUS_UNCONNECT){
            if(firstConnect){
                status = Agv::AGV_STATUS_NOTREADY;
            }else if(currentTask == nullptr || currentTask->getIsCancel()){
                status = Agv::AGV_STATUS_IDLE;
            }else{
                status = Agv::AGV_STATUS_TASKING;
            }
        }
    }
}
//发送消息给小车
bool DyForklift::send(const std::string &data)
{
    std::string sendContent = transToFullMsg(data);
    if (sendContent.length() < 13)return false;

    if (FORKLIFT_HEART != stringToInt(sendContent.substr(11, 2)))
    {
        combined_logger->info("send to agv{0}:{1}", id, sendContent);
    }
    if (nullptr != m_qTcp  && m_qTcp->alive())
    {
        m_qTcp->write(sendContent.c_str(), sendContent.length());
    }
    int msgType = stringToInt(sendContent.substr(11, 2));

    if(FORKLIFT_STARTREPORT != msgType && FORKLIFT_HEART != msgType){
        unRecvMsgMtx.lock();
        m_unRecvSend[stoi(sendContent.substr(1, 6))] = sendContent;
        unRecvMsgMtx.unlock();
    }

    if (FORKLIFT_STARTREPORT != msgType && FORKLIFT_HEART != msgType)
    {
        UNIQUE_LCK(unFinishMsgMtx);
        m_unFinishCmd[msgType] = sendContent;
    }
    return false;
}

//开始上报
bool DyForklift::startReport(int interval)
{
    std::stringstream body;
    body << FORKLIFT_STARTREPORT;
    body.fill('0');
    body.width(5);
    body << interval;

    //eg:*12345600172100100#
    return resend(body.str());
}

//结束上报
bool DyForklift::endReport()
{
    std::stringstream body;
    body << FORKLIFT_ENDREPORT;
    //eg:*123456001222#
    return resend(body.str());
}

//判断小车命令是否执行结束
bool DyForklift::isFinish()
{
    UNIQUE_LCK lck(unFinishMsgMtx);
    return !m_unFinishCmd.size();
}

bool DyForklift::isFinish(int cmd_type)
{
    UNIQUE_LCK lck(unFinishMsgMtx);
    auto iter = m_unFinishCmd.find(cmd_type);
    if (iter != m_unFinishCmd.end())
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool DyForklift::charge(int params)
{
    std::stringstream body;
    body << FORKLIFT_CHARGE;
    body << params;
    return resend(body.str());
}

bool DyForklift::tellAgvPos(int station)
{
    auto mapmanagerptr = MapManager::getInstance();

    MapPoint *point = mapmanagerptr->getPointById(station);

    if(point == nullptr)return false;
    std::stringstream body;
    body << FORKLIFT_INITPOS;
    body << point->getRealX() / 100.0 << "," << -point->getRealY() / 100.0 << "," << point->getRealA() / 10.0 / 57.3 << "," << MapManager::getInstance()->getFloor(station);
    return resend(body.str());
}

//只在开机或者任务取消手动操作后，进行初始化位置。因为会释放掉所有线路和点的占用
bool DyForklift::setInitPos(int station)
{
    auto mapmanagerptr = MapManager::getInstance();

    MapPoint *point = mapmanagerptr->getPointById(station);
    if(point == nullptr)return false;

    setPosition(0, station, 0);

    //占据初始位置
    mapmanagerptr->addOccuStation(station, shared_from_this());
    //释放其他所有占用
    mapmanagerptr->freeAllStationLines(shared_from_this(),station);

    std::stringstream body;
    body << FORKLIFT_INITPOS;
    body << point->getRealX() / 100.0 << "," << -point->getRealY() / 100.0 << "," << point->getRealA() / 10.0 / 57.3 << "," << MapManager::getInstance()->getFloor(station);
    return resend(body.str());
}


bool DyForklift::stopEmergency(int params)
{
    std::stringstream body;
    body << FORKLIFT_STOP;
    body << params;
    return resend(body.str());
}


bool DyForklift::isNeedTakeElevator(std::vector<int> lines)
{
    int endId = 0;
    combined_logger->info("DyForklift, isNeedTakeElevator");
    bool needElevator = false;

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        endId = path->getEnd();

        MapSpirit *pointSpirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *station = static_cast<MapPoint *>(pointSpirit);
        if (station == nullptr || station->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            combined_logger->error("DyForklift, station is null or not Map_Sprite_Type_Point!!!!!!!");
            continue;
        }

        if (station->getMapChange())
        {
            needElevator = true;
            break;
        }
    }
    return needElevator;
}

int DyForklift::getPathToElevator(std::vector<int> lines, int index, std::vector<int> &result)
{
    auto mapmanagerptr = MapManager::getInstance();
    int k = index;
    for (int i = index; i < lines.size(); ++i)
    {
        int line = lines[i];
        auto lineptr = mapmanagerptr->getPathById(line);
        if (lineptr == nullptr)continue;
        auto endptr = mapmanagerptr->getPointById(lineptr->getEnd());
        if (endptr == nullptr)continue;

        if (endptr->getMapChange()) {
            k = i;
            break;
        }
        result.push_back(line);
    }

    return k;
}


int DyForklift::getPathLeaveElevator(std::vector<int> lines, int index, std::vector<int> &result)
{
    auto mapmanagerptr = MapManager::getInstance();

    int k = index;
    for (int i = index; i < lines.size(); ++i)
    {
        int line = lines[i];
        auto lineptr = mapmanagerptr->getPathById(line);
        if (lineptr == nullptr)continue;
        auto endptr = mapmanagerptr->getPointById(lineptr->getEnd());
        if (endptr == nullptr)continue;

        k = i;
        if (endptr->getMapChange()) {
            break;
        }
        result.push_back(line);
    }
    return k;
}

int DyForklift::getPathInElevator(std::vector<int> lines, int index, std::vector<int> &result)
{
    auto mapmanagerptr = MapManager::getInstance();

    int k = index;
    for (int i = index; i < lines.size(); ++i)
    {
        int line = lines[i];
        auto lineptr = mapmanagerptr->getPathById(line);
        if (lineptr == nullptr)continue;
        auto startptr = mapmanagerptr->getPointById(lineptr->getStart());
        auto endptr = mapmanagerptr->getPointById(lineptr->getEnd());
        if (startptr == nullptr || endptr == nullptr)continue;

        if (!startptr->getMapChange() || !endptr->getMapChange()) {
            k = i;
            break;
        }
        result.push_back(line);
    }
    return k;
}

int DyForklift::getPathEnterElevator(std::vector<int> lines, int index, std::vector<int> &result)
{
    auto mapmanagerptr = MapManager::getInstance();
    int k = index;
    for (int i = index; i < lines.size(); ++i)
    {
        int line = lines[i];
        auto lineptr = mapmanagerptr->getPathById(line);
        if (lineptr == nullptr)continue;
        auto startptr = mapmanagerptr->getPointById(lineptr->getStart());
        auto endptr = mapmanagerptr->getPointById(lineptr->getEnd());
        if (startptr == nullptr || endptr == nullptr)continue;

        if (!startptr->getMapChange() && endptr->getMapChange()) {
            result.push_back(line);
            k = i + 1;
            break;
        }
    }
    return k;
}

int DyForklift::getPathOutElevator(std::vector<int> lines, int index, std::vector<int> &result)
{
    auto mapmanagerptr = MapManager::getInstance();

    int k = index;
    for (int i = index; i < lines.size(); ++i)
    {
        int line = lines[i];
        auto lineptr = mapmanagerptr->getPathById(line);
        if (lineptr == nullptr)continue;
        auto startptr = mapmanagerptr->getPointById(lineptr->getStart());
        auto endptr = mapmanagerptr->getPointById(lineptr->getEnd());
        if (startptr == nullptr || endptr == nullptr)continue;

        if (startptr->getMapChange() && !endptr->getMapChange()) {
            result.push_back(line);
            k = i + 1;
            break;
        }
    }
    return k;
}

std::vector<int> DyForklift::getPathToElevator(std::vector<int> lines)
{
    std::vector<int> paths;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathToElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;


        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
            continue;

        if (end_point->getMapChange())
        {
            return paths;
        }
        paths.push_back(line);

    }
    return paths;
}

std::vector<int> DyForklift::getPathLeaveElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;
    bool elein = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathLeaveElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if (addPath)
            paths.push_back(line);

        if (start_point->getMapChange() && end_point->getMapChange())
        {
            elein = true;
        }
        if (elein && !(start_point->getMapChange() && end_point->getMapChange()))
        {
            addPath = true;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathInElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathInElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if (start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
        }
        else if (paths.size())
        {
            return paths;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathEnterElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathEnterElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }


        if (!start_point->getMapChange() && end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

std::vector<int> DyForklift::getPathOutElevator(std::vector<int> lines)
{
    std::vector<int> paths;
    bool addPath = false;

    int endId = -1;
    int startId = -1;
    combined_logger->info("DyForklift,  getPathOutElevator");

    for (auto line : lines) {
        MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(line);
        if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Path)
            continue;

        MapPath *path = static_cast<MapPath *>(spirit);
        startId = path->getStart();
        endId = path->getEnd();


        MapSpirit *start_Spirit = MapManager::getInstance()->getMapSpiritById(startId);
        MapPoint *start_point = static_cast<MapPoint *>(start_Spirit);
        if (start_point == nullptr || start_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        MapSpirit *end_Spirit = MapManager::getInstance()->getMapSpiritById(endId);
        MapPoint *end_point = static_cast<MapPoint *>(end_Spirit);
        if (end_point == nullptr || end_point->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
        {
            continue;
        }

        if (start_point->getMapChange() && !end_point->getMapChange())
        {
            paths.push_back(line);
            return paths;
        }
    }
    return paths;
}

bool DyForklift::pause()
{
    combined_logger->debug("==============agv:{0} paused!", getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIST_NOLASER_PAUSE;
    sendPause = true;
    return resend(body.str());
}

bool DyForklift::resume()
{
    combined_logger->debug("==============agv:{0} resume!", getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIFT_NOLASER_RESUME;
    sendPause = false;
    return resend(body.str());
}

void DyForklift::onTaskCanceled(AgvTaskPtr _task)
{
    combined_logger->debug("==============agv:{0} task cancel! clear path!", getId());
    std::stringstream body;
    body << FORKLIFT_MOVE_NOLASER;
    body << FORKLIFT_NOLASER_CLEAR_TASK;
    resend(body.str());

    unRecvMsgMtx.lock();
    m_unRecvSend.clear();
    unRecvMsgMtx.unlock();

    unFinishMsgMtx.lock();
    m_unFinishCmd.clear();
    unFinishMsgMtx.unlock();

    //occu current station or  current path
    auto conflictmanagerptr = ConflictManager::getInstance();
    conflictmanagerptr->freeAgvOccu(getId(), lastStation, nowStation, nextStation);
}

