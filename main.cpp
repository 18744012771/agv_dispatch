#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "taskmanager.h"
#include "taskmaker.h"
#include "mapmap/mapmanager.h"
#include "agvmanager.h"
#include "usermanager.h"
#include "network/sessionmanager.h"
#include "msgprocess.h"
#include "userlogmanager.h"
#include "common.h"
#include "agvImpl/ros/agv/rosAgv.h"
#include "qunchuang/chipmounter/chipmounter.h"
#include "device/elevator/elevator.h"
#include "device/elevator/elevatorManager.h"
#include "device/devicemanager.h"
#include "device/new_elevator/newelevator.h"
#include "device/new_elevator/newelevatormanager.h"
#include "Dongyao/dyforklift.h"
#include "Dongyao/dytaskmaker.h"
#include "mapmap/blockmanager.h"
#include "mapmap/conflictmanager.h"

#define DY_TEST
void initLog()
{
    //日志文件
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(color_sink);

        //日志文件
        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("agv_dispatch.log", 1024 * 1024 * 20, 5);
        sinks.push_back(rotating);

        combined_logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
        combined_logger->flush_on(spdlog::level::trace);
        combined_logger->set_level(spdlog::level::trace);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void testAGV()
{
    g_threads.create_thread([&] {
        // test ros agv
        //rosAgvPtr agv(new rosAgv(1,"robot_0","192.168.8.206",7070));
        rosAgvPtr agv(new rosAgv(1, "robot_0", "192.168.8.211", 7070));

        std::cout << "AGV init...." << std::endl;

        agv->init();
        chipmounter *chip = new chipmounter(1, "chipmounter", "10.63.39.190", 1000);
        //chipmounter *chip = new chipmounter(1,"chipmounter","192.168.8.101",1024);
        chip->init();
        agv->setChipMounter(chip);

        sleep(20);

        chipinfo info;
        while (chip != nullptr)
        {
            if (!chip->isConnected())
            {
                //chip->init();
                //std::cout << "chipmounter disconnected, need reconnect...."<< std::endl;
            }

            if (chip->getAction(&info))
            {
                std::cout << "new task ...." << "action: " << info.action << "point: " << info.point << std::endl;
                if (agv->isAGVInit())
                {
                    chip->deleteHeadAction();
                    agv->startTask(info.point, info.action);
                    //agv->startTask( "2510", "loading");

                }
            }
            /*else
        {
            std::cout << "new task for test...." << "action: " <<info.action<< "point: " <<info.point<< std::endl;

            agv->startTask( "2511", "unloading");
            //agv->startTask( "", "");
            break;
        }*/

            sleep(1);
        }
    });


    std::cout << "testAGV end...." << std::endl;
}

void getWmsStorageJson()
{
    CppSQLite3DB wms_db;
    try{
        wms_db.open("wms.db");
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("wms db sqlite error {0}:{1};", e.errorCode(), e.errorMessage());
        return ;
    }

    //
    Json::Value fff;
    try{
        CppSQLite3Table table = wms_db.getTable("select storage_no,store_no,map_station_id  from C_STORAGE_DEF_T;");
        if (table.numRows() <= 0 || table.numFields() != 3)
        {
            combined_logger->error("MapManager loadFromDb agv_station error!");
            return ;
        }
        for (int row = 0; row < table.numRows(); row++)
        {
            table.setRow(row);

            std::string storage_no = table.fieldValue(0);
            std::string store_no = table.fieldValue(1);
            if(table.fieldIsNull(2))continue;
            int station_id = atoi(table.fieldValue(2));
            std::stringstream ss;
            ss<<"select realX,realY,realA from agv_station where id=\'"<<station_id<<"\';";
            CppSQLite3Table table_station = wms_db.getTable(ss.str().c_str());
            if (table_station.numRows() <= 0 && table_station.numFields() != 3)
            {
                combined_logger->error("no station with id {0}",station_id);
                continue;
            }

            table_station.setRow(0);
            int realX = atoi(table_station.fieldValue(0));
            int realY = atoi(table_station.fieldValue(1));
            //            int realA = atoi(table_station.fieldValue(2));

            int xx = realX;
            int yy = realY;

            //            if(realA>=-450 && realA<=450){
            //                xx = realX;
            //                yy = realY - 50;
            //            }else if(realA>450 &&  realA<= 1350){
            //                xx = realX - 50;
            //                yy = realY - 100;
            //            }else if(realA>1350 || realA<-1350){
            //                xx = realX - 100;
            //                yy = realY - 50;
            //            }else{
            //                xx = realX - 50;
            //                yy = realY;
            //            }

            Json::Value vv;
            vv["store_no"] = store_no;
            vv["storage_no"] = storage_no;
            vv["storage_x"] = xx;
            vv["storage_y"] = yy;
            vv["current_type"] = "type_null";
            vv["station_id"] = station_id;
            fff.append(vv);
        }

        combined_logger->debug(" jsons ==========={0}",fff.toStyledString());
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("wms db sqlite error {0}:{1};", e.errorCode(), e.errorMessage());
        return ;
    }
}

void quit(int sig)
{
    g_quit = true;
    g_threads.interrupt_all();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, quit);
    std::cout << "start server ..." << std::endl;

    //0.日志输出
    initLog();

    //getWmsStorageJson();

    //1.打开数据库
    try {
        g_db.open(DB_File);
    }
    catch (CppSQLite3Exception &e) {
        combined_logger->error("sqlite error {0}:{1};", e.errorCode(), e.errorMessage());
        return -1;
    }

    //2.载入地图
    combined_logger->debug("map manager load...");
    if (!MapManager::getInstance()->load()) {
        combined_logger->error("map manager load fail");
        return -2;
    }

    combined_logger->debug("ConflictManager init...");
    ConflictManager::getInstance()->init();

    //ConflictManager::getInstance()->test();

    //3.初始化车辆及其链接
    combined_logger->debug("AgvManager init...");
    if (!AgvManager::getInstance()->init()) {
        combined_logger->error("AgvManager init fail");
        return -3;
    }

    //4.初始化任务管理
    combined_logger->debug("TaskManager init...");
    if (!TaskManager::getInstance()->init()) {
        combined_logger->error("TaskManager init fail");
        return -4;
    }

    //5.用户管理
    combined_logger->debug("UserManager init...");
    UserManager::getInstance()->init();

    //6.初始化消息处理
    combined_logger->debug("MsgProcess init...");
    if (!MsgProcess::getInstance()->init()) {
        combined_logger->error("MsgProcess init fail");
        return -5;
    }

    //7.初始化日志发布
    combined_logger->debug("UserLogManager init...");
    UserLogManager::getInstance()->init();

    //8.初始化电梯
    combined_logger->debug("NewElevatorManager init...");
    NewElevatorManager::getInstance()->init();

    //TODO:test 电梯
    //NewElevatorManager::getInstance()->test();

    //9.初始化任务生成
    combined_logger->debug("TaskMaker init...");
    TaskMaker::getInstance()->init();

    //    10.初始化tcp/ip 接口
    //tcpip服务
    combined_logger->debug("SessionManager init...");
    auto sessionmanagerptr = SessionManager::getInstance();
    sessionmanagerptr->startAgvServer(6789);
    sessionmanagerptr->startClientServer(9999);
    combined_logger->info("server init OK!");
    sessionmanagerptr->run();

    g_threads.join_all();

    spdlog::drop_all();

    return 0;
}
