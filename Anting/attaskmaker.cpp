﻿#include "attaskmaker.h"
#include "../taskmanager.h"
#include "../mapmap/mapmanager.h"
#include "../agvmanager.h"
#include "atforkliftthingfork.h"
#include "atforklift.h"
#include "../userlogmanager.h"
#include "../agvtask.h"
#include "../network/tcpclient.h"

#define USETABLE
AtTaskMaker::AtTaskMaker(std::string _ip, int _port) : m_ip(_ip),
                                                       m_port(_port),
                                                       m_connectState(false)
{
    init_station_pos();
}

void AtTaskMaker::init_station_pos()
{
    try
    {
        if (!g_db.tableExists("agv_station_pos"))
        {
            g_db.execDML("CREATE TABLE agv_station_pos (id	INTEGER, posLevel INTEGER,pickPos1	INTEGER, pickPos2 INTEGER, putPos1	INTEGER, putPos2 INTEGER,x INTEGER,y INTEGER,t INTEGER, PRIMARY KEY(id,posLevel))");
        }
        CppSQLite3Table table_station_pos = g_db.getTable("select id, posLevel, pickPos1, pickPos2, putPos1, putPos2,x,y,t from agv_station_pos;");
        if (table_station_pos.numRows() > 0 && table_station_pos.numFields() != 9)
        {
            combined_logger->error("loadFromDb agv_station_pos error!");
            return;
        }
        for (int row = 0; row < table_station_pos.numRows(); row++)
        {
            table_station_pos.setRow(row);
            int id = atoi(table_station_pos.fieldValue(0));
            int level = atoi(table_station_pos.fieldValue(1));
            StationPos pos(id, level, atoi(table_station_pos.fieldValue(2)), atoi(table_station_pos.fieldValue(3)), atoi(table_station_pos.fieldValue(4)), atoi(table_station_pos.fieldValue(5)), atoi(table_station_pos.fieldValue(6)), atoi(table_station_pos.fieldValue(7)), atoi(table_station_pos.fieldValue(8)));
            m_station_pos[std::make_pair(id, level)] = pos;
        }
    }
    catch (CppSQLite3Exception &e)
    {
        combined_logger->error("sqlite error{0}:{1}", e.errorCode(), e.errorMessage());
        return;
    }
    catch (std::exception e)
    {
        combined_logger->error("sqlite error{0}", e.what());
        return;
    }
}

void AtTaskMaker::init()
{
    //接收wms发过来的任务
    TcpClient::ClientReadCallback onread = std::bind(&AtTaskMaker::onRead, this, std::placeholders::_1, std::placeholders::_2);
    TcpClient::ClientConnectCallback onconnect = std::bind(&AtTaskMaker::onConnect, this);
    TcpClient::ClientDisconnectCallback ondisconnect = std::bind(&AtTaskMaker::onDisconnect, this);

    m_wms_tcpClient = new TcpClient(m_ip, m_port, onread, onconnect, ondisconnect);

    combined_logger->info(typeid(TaskMaker::getInstance()).name());
}

void AtTaskMaker::onRead(const char *data, int len)
{
    //receiveTask(data);
}

void AtTaskMaker::onConnect()
{
    //TODO
    m_connectState = true;
    combined_logger->info("wms_connected. ip:{0} port:{1}", m_ip, m_port);
}

void AtTaskMaker::onDisconnect()
{
    //TODO
    m_connectState = false;
    combined_logger->info("wms_disconnected. ip:{0} port:{1}", m_ip, m_port);
}

#ifndef USETABLE
void AtTaskMaker::makeTask(ClientSessionPtr conn, const Json::Value &request)
{
    AgvTaskPtr task(new AgvTask());

    //1.指定车辆
    int agvId = request["agv"].asInt();
    task->setAgv(agvId);

    //2.优先级
    int priority = request["priority"].asInt();
    task->setPriority(priority);

    //3.执行次数
    if (!request["runTimes"].isNull())
    {
        int runTimes = request["runTimes"].asInt();
        task->setRunTimes(runTimes);
    }

    //3.额外的参数
    if (!request["extra_params"].isNull())
    {
        Json::Value extra_params = request["extra_params"];
        Json::Value::Members mem = extra_params.getMemberNames();
        for (auto iter = mem.begin(); iter != mem.end(); iter++)
        {
            task->setExtraParam(*iter, extra_params[*iter].asString());
        }
    }

    std::string task_describe;
    //4.节点
    if (!request["nodes"].isNull())
    {
        Json::Value nodes = request["nodes"];
        for (int i = 0; i < nodes.size(); ++i)
        {
            Json::Value one_node = nodes[i];
            int station = one_node["station"].asInt();
            int doWhat = one_node["dowhat"].asInt();
            std::string node_params_str = one_node["params"].asString();
            if (!node_params_str.length())
            {
                node_params_str = "20;20";
            }
            std::vector<std::string> node_params = split(node_params_str, ";");
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
                continue;
            MapPoint *point = static_cast<MapPoint *>(spirit);

            task_describe.append(point->getName());
            //根据客户端的代码，
            //dowhat列表为
            // 0 --> pick
            // 1 --> put
            // 2 --> charge
            AgvTaskNodePtr node_node(new AgvTaskNode());
            node_node->setStation(station);
            node_node->setParams(node_params_str);
            std::vector<AgvTaskNodeDoThingPtr> doThings;

            if (doWhat == 0)
            {

                task_describe.append("[↑] ");
                //liftup
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("11");
                //                _paramsfork.push_back(task->getExtraParam("moveHeight"));
                //                _paramsfork.push_back(task->getExtraParam("finalHeight"));
                if (node_params.size() >= 2)
                {
                    _paramsfork.push_back(node_params[0]);
                    _paramsfork.push_back(node_params[1]);
                }

                //                task->setExtraParam("moveHeight", "1000");
                //                task->setExtraParam("finalHeight", "1100");

                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));

                node_node->setTaskType(TASK_PICK);
                node_node->setDoThings(doThings);
            }
            else if (doWhat == 1)
            {

                task_describe.append("[↓] ");

                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");

                if (node_params.size() >= 2)
                {
                    _paramsfork.push_back(node_params[0]);
                    _paramsfork.push_back(node_params[1]);
                }
                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));
                node_node->setTaskType(TASK_PUT);
                node_node->setDoThings(doThings);
            }
            else if (doWhat == 2)
            {
                //                AgvTaskNodeDoThingPtr chargeThing(new QingdaoNodeTingCharge(node_params));
                //                node_node->push_backDoThing(chargeThing);
            }
            else if (doWhat == 3)
            {
                task_describe.append("[--] ");

                //DONOTHING,JUST MOVE
                node_node->setTaskType(TASK_MOVE);
            }
            task->push_backNode(node_node);
        }
    }

    //5.产生时间
    task->setProduceTime(getTimeStrNow());
    task->setDescribe(task_describe);

    combined_logger->info(" getInstance()->addTask ");

    TaskManager::getInstance()->addTask(task);

    combined_logger->info("makeTask");

    //TODO:创建任务//TODO:回头再改
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
}
#else
void AtTaskMaker::makeTask(ClientSessionPtr conn, const Json::Value &request)
{
    AgvTaskPtr task(new AgvTask());

    //1.指定车辆
    int agvId = request["agv"].asInt();
    task->setAgv(agvId);

    //2.优先级
    int priority = request["priority"].asInt();
    task->setPriority(priority);

    //3.执行次数
    if (!request["runTimes"].isNull())
    {
        int runTimes = request["runTimes"].asInt();
        task->setRunTimes(runTimes);
    }

    //4.额外的参数
    if (!request["extra_params"].isNull())
    {
        Json::Value extra_params = request["extra_params"];
        Json::Value::Members mem = extra_params.getMemberNames();
        for (auto iter = mem.begin(); iter != mem.end(); iter++)
        {
            task->setExtraParam(*iter, extra_params[*iter].asString());
        }
    }

    std::string task_describe;
    //4.节点
    if (!request["nodes"].isNull())
    {
        Json::Value nodes = request["nodes"];
        for (int i = 0; i < nodes.size(); ++i)
        {
            Json::Value one_node = nodes[i];
            int station = one_node["station"].asInt();
            int doWhat = one_node["dowhat"].asInt();
            std::string node_params_str;
            int level = 1;
            level = stringToInt(one_node["params"].asString());
            if (level == 0 || level == -1)
                level = 1;
            MapSpirit *spirit = MapManager::getInstance()->getMapSpiritById(station);
            if (spirit == nullptr || spirit->getSpiritType() != MapSpirit::Map_Sprite_Type_Point)
                continue;
            MapPoint *point = static_cast<MapPoint *>(spirit);

            task_describe.append(point->getName());
            task_describe.append("["+intToString(level)+"]");
            //根据客户端的代码，
            //dowhat列表为
            // 0 --> pick
            // 1 --> put
            // 2 --> charge
            // 3 --> move
            AgvTaskNodePtr node_node(new AgvTaskNode());
            node_node->setStation(station);
            std::vector<AgvTaskNodeDoThingPtr> doThings;

            if (doWhat == 0)
            {

                task_describe.append("[↑] ");
                //liftup
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("11");
                StationPos pos = m_station_pos[std::make_pair(station, level)];
                _paramsfork.push_back(intToString(pos.m_pickPos1));
                _paramsfork.push_back(intToString(pos.m_pickPos2));
                node_params_str.append(intToString(pos.m_pickPos1))
                    .append(";")
                    .append(intToString(pos.m_pickPos2))
                    .append(";")
                    .append(intToString(pos.m_x))
                    .append(";")
                    .append(intToString(pos.m_y))
                    .append(";")
                    .append(intToString(pos.m_t));
                node_node->setParams(node_params_str);

                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));

                node_node->setTaskType(TASK_PICK);
                node_node->setDoThings(doThings);
            }
            else if (doWhat == 1)
            {

                task_describe.append("[↓] ");

                //setdown
                std::vector<std::string> _paramsfork;
                _paramsfork.push_back("00");

                StationPos pos = m_station_pos[std::make_pair(station, level)];
                _paramsfork.push_back(intToString(pos.m_putPos1));
                _paramsfork.push_back(intToString(pos.m_putPos2));
                node_params_str.append(intToString(pos.m_putPos1))
                    .append(";")
                    .append(intToString(pos.m_putPos2))
                    .append(";")
                    .append(intToString(pos.m_x))
                    .append(";")
                    .append(intToString(pos.m_y))
                    .append(";")
                    .append(intToString(pos.m_t));
                node_node->setParams(node_params_str);
                doThings.push_back(AgvTaskNodeDoThingPtr(new AtForkliftThingFork(_paramsfork)));
                node_node->setTaskType(TASK_PUT);
                node_node->setDoThings(doThings);
            }
            else if (doWhat == 2)
            {
                //                AgvTaskNodeDoThingPtr chargeThing(new QingdaoNodeTingCharge(node_params));
                //                node_node->push_backDoThing(chargeThing);
            }
            else if (doWhat == 3)
            {
                task_describe.append("[--] ");

                //DONOTHING,JUST MOVE
                node_node->setTaskType(TASK_MOVE);
            }
            task->push_backNode(node_node);
        }
    }

    //5.产生时间
    task->setProduceTime(getTimeStrNow());
    task->setDescribe(task_describe);

    combined_logger->info(" getInstance()->addTask ");

    TaskManager::getInstance()->addTask(task);

    combined_logger->info("makeTask");

    //TODO:创建任务//TODO:回头再改
    Json::Value response;
    response["type"] = MSG_TYPE_RESPONSE;
    response["todo"] = request["todo"];
    response["queuenumber"] = request["queuenumber"];
    response["result"] = RETURN_MSG_RESULT_SUCCESS;
}
#endif

void AtTaskMaker::finishTask(std::string store_no, std::string storage_no, int type, std::string key_part_no, int agv_id)
{
    std::stringstream body;
    if (type == 1)
    {
        body << "72" << store_no << "|" << storage_no << "|" << agv_id << "|" << key_part_no;
    }
    else
    {
        body << "73" << store_no << "|" << storage_no << "|" << agv_id;
    }
    std::stringstream ss;
    ss << "*";
    //时间戳
    ss.fill('0');
    ss.width(6);
    time_t TimeStamp = clock() % 1000000;
    ss << TimeStamp;
    //长度
    ss.fill('0');
    ss.width(4);
    ss << (body.str().length() + 10);
    ss << body.str();
    ss << "#";

    combined_logger->info("sendToWMS:{0}", ss.str().c_str());

    m_wms_tcpClient->sendToServer(ss.str().c_str(), ss.str().length());
}
