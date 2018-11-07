#-------------------------------------------------
#
# Project created by QtCreator 2018-04-08T15:53:55
#
#-------------------------------------------------

QT       -= gui core

TARGET = AgvDispatch
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++11
CONFIG += c++14
TEMPLATE = app


unix{
LIBS += -lsqlite3
LIBS += -ljsoncpp
LIBS += -lboost_system
LIBS += -lboost_thread
}

win32{
win32:DEFINES += _CRT_SECURE_NO_WARNINGS
win32:DEFINES += _WINSOCK_DEPRECATED_NO_WARNINGS
INCLUDEPATH+=D:\thirdparty\sqlite\include
INCLUDEPATH+=D:\thirdparty\jsoncpp\include
INCLUDEPATH+=D:\thirdparty\boost\src\boost_1_67_0
LIBS+=D:\thirdparty\sqlite\lib\Debug\sqlite3.lib
LIBS+=D:\thirdparty\jsoncpp\lib\x64\Debug\jsoncpp.lib
LIBS+=D:\thirdparty\boost\install\lib\libboost_system-vc141-mt-gd-x64-1_67.lib
LIBS+=D:\thirdparty\boost\install\lib\libboost_date_time-vc141-mt-gd-x64-1_67.lib
LIBS+=D:\thirdparty\boost\install\lib\libboost_regex-vc141-mt-gd-x64-1_67.lib
}

HEADERS += \
    agv.h \
    agvmanager.h \
    agvtask.h \
    agvtasknode.h \
    agvtasknodedothing.h \
    common.h \
    taskmanager.h \
    sqlite3/CppSQLite3.h \
    protocol.h \
    msgprocess.h \
    usermanager.h \
    userlogmanager.h \
    usermanager.h \
    taskmaker.h \
    mapmap/mapbackground.h \
    mapmap/mapblock.h \
    mapmap/mapfloor.h \
    mapmap/mapgroup.h \
    mapmap/mappath.h \
    mapmap/mappoint.h \
    mapmap/mapspirit.h \
    mapmap/onemap.h \
    base64.h \
    agvImpl/ros/agv/rosAgv.h \
    agvImpl/ros/agv/linepath.h \
    mapmap/mapmanager.h \
    qunchuang/qunchuangtaskmaker.h \
    qunchuang/qunchuangnodethingget.h \
    qunchuang/qunchuangnodetingput.h \
    qunchuang/qunchuangtcsconnection.h \
    device/device.h \
    device/elevator/elevator.h \
    device/elevator/elevator_protocol.h \
    qunchuang/chipmounter/chipmounter.h \
    realagv.h \
    virtualagv.h \
    virtualrosagv.h \
    bezierarc.h \
    qingdao/qingdaonodetingcharge.h \
    qingdao/qingdaonodetingget.h \
    qingdao/qingdaonodetingput.h \
    qingdao/qingdaotaskmaker.h \
    qunchuang/msg.h \
    Dongyao/dyforklift.h \
    Dongyao/dytaskmaker.h \
    Dongyao/dyforkliftupdwms.h \
    Dongyao/dyforkliftthingfork.h \
    Dongyao/charge/chargemachine.h \
    qyhbuffer.h \
    Anting/attaskmaker.h \
    Anting/atforklift.h \
    Anting/atforkliftthingfork.h \
    Anting/station_pos.h \
    Dongyao/dyforkliftthingcharge.h \
    device/devicemanager.h \
    device/elevator/elevatorManager.h \
    mapmap/blockmanager.h \
    qingdao/qingdaonodetingmove.h \
    mapmap/conflict.h \
    mapmap/conflictmanager.h \
    mapmap/mapconflictpair.h \
    device/new_elevator/newelevatormanager.h \
    device/new_elevator/newelevator.h \
    network/agvserver.h \
    network/agvsession.h \
    network/clientserver.h \
    network/clientsession.h \
    network/server.h \
    network/session.h \
    network/sessionmanager.h \
    network/tcpclient.h


SOURCES += \
    agv.cpp \
    agvmanager.cpp \
    common.cpp \
    main.cpp \
    taskmanager.cpp \
    sqlite3/CppSQLite3.cpp \
    msgprocess.cpp \
    usermanager.cpp \
    userlogmanager.cpp \
    taskmaker.cpp \
    mapmap/mapbackground.cpp \
    mapmap/mapblock.cpp \
    mapmap/mapfloor.cpp \
    mapmap/mapgroup.cpp \
    mapmap/mapmanager.cpp \
    mapmap/mappath.cpp \
    mapmap/mappoint.cpp \
    mapmap/mapspirit.cpp \
    mapmap/onemap.cpp \
    base64.cpp \
    agvImpl/ros/agv/rosAgv.cpp \
    qunchuang/qunchuangtaskmaker.cpp \
    qunchuang/qunchuangnodethingget.cpp \
    qunchuang/qunchuangnodetingput.cpp \
    qunchuang/qunchuangtcsconnection.cpp \
    device/device.cpp \
    device/elevator/elevator.cpp \
    qunchuang/chipmounter/chipmounter.cpp \
    qunchuang/msg.cpp \
    device/elevator/elevator_protocol.cpp \
    realagv.cpp \
    virtualagv.cpp \
    virtualrosagv.cpp \
    bezierarc.cpp \
    qingdao/qingdaonodetingcharge.cpp \
    qingdao/qingdaonodetingget.cpp \
    qingdao/qingdaonodetingput.cpp \
    qingdao/qingdaotaskmaker.cpp \
    Dongyao/dyforklift.cpp \
    Dongyao/dytaskmaker.cpp \
    Dongyao/dyforkliftupdwms.cpp \
    Dongyao/dyforkliftthingfork.cpp \
    Dongyao/charge/chargemachine.cpp \
    qyhbuffer.cpp \
    Anting/atforklift.cpp \
    Anting/atforkliftthingfork.cpp \
    Anting/attaskmaker.cpp \
    Dongyao/dyforkliftthingcharge.cpp \
    device/devicemanager.cpp \
    device/elevator/elevatorManager.cpp \
    mapmap/blockmanager.cpp \
    qingdao/qingdaonodetingmove.cpp \
    mapmap/conflict.cpp \
    mapmap/conflictmanager.cpp \
    mapmap/mapconflictpair.cpp \
    device/new_elevator/newelevatormanager.cpp \
    device/new_elevator/newelevator.cpp \
    network/agvserver.cpp \
    network/agvsession.cpp \
    network/clientserver.cpp \
    network/clientsession.cpp \
    network/server.cpp \
    network/session.cpp \
    network/sessionmanager.cpp \
    network/tcpclient.cpp
