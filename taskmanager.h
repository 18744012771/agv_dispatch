#ifndef TASKMANAGER_H
#define TASKMANAGER_H
#include <mutex>
#include <map>
#include <atomic>
#include <boost/noncopyable.hpp>
#include "agvtask.h"
#include "network/session.h"

class TaskManager;

using TaskManagerPtr = std::shared_ptr<TaskManager>;

class TaskManager :  public boost::noncopyable,public std::enable_shared_from_this<TaskManager>
{
public:
    static TaskManagerPtr getInstance(){
        static TaskManagerPtr m_inst = TaskManagerPtr(new TaskManager());
        return m_inst;
    }
    bool init();

    bool hasTaskDoing();

    //添加任务
    bool addTask(AgvTaskPtr task);

    //查询未执行的任务
    AgvTaskPtr queryUndoTask(int taskId);

    //查询正在执行的任务
    AgvTaskPtr queryDoingTask(int taskId);

    //查询完成执行的任务
    AgvTaskPtr queryDoneTask(int taskId);

    //返回task的状态。
    int queryTaskStatus(int taskId);

    //取消一个任务
    int cancelTask(int taskId);

    //完成了一个任务
    void finishTask(AgvTaskPtr task);

    //执行一个任务
    void excuteTask(AgvTaskPtr task);

	std::vector<AgvTaskPtr> getCurrentTasks();

    //用户接口
    void interCreate(ClientSessionPtr conn, const Json::Value &request);
    void interQueryStatus(ClientSessionPtr conn, const Json::Value &request);
    void interCancel(ClientSessionPtr conn, const Json::Value &request);
    void interListUndo(ClientSessionPtr conn, const Json::Value &request);
    void interListDoing(ClientSessionPtr conn, const Json::Value &request);
    void interListDoneToday(ClientSessionPtr conn, const Json::Value &request);
    void interListDuring(ClientSessionPtr conn, const Json::Value &request);
protected:
    TaskManager();
private:

    //return true if task is distributed
    bool distributeTask(AgvTaskPtr task);

    void checkTable();
    //未分配的任务[优先级/任务列表]
    std::map<int, std::vector<AgvTaskPtr> ,std::greater<int> > toDistributeTasks;
    std::mutex toDisMtx;

    //已分配的任务
    std::vector<AgvTaskPtr> doingTask;
    std::mutex doingTaskMtx;

	//已完成的任务
	std::vector<AgvTaskPtr> doneTask;
	std::mutex doneTaskMtx;

    bool saveTask(AgvTaskPtr task);

    std::atomic_int node_id;
    std::atomic_int thing_id;
    std::atomic_int task_id;
};

#endif // TASKMANAGER_H
