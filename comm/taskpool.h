/******************************************************************* 
 *  summery:        同步类任务工作池
 *  author:         hejl
 *  date:           2016-09-21
 *  description:    
 ******************************************************************/
#ifndef __TASKPOOL_H_
#define __TASKPOOL_H_
#include <vector>
#include "public.h"
#include "queue.h"

// 要实现此接口的类才能放入置任务池处理
struct ITaskRun
{
    // @param flag, 正常调用传0, 退出清理传其他
    virtual int run_task(int flag) = 0;
};

class TaskPool
{
    // 单实例对象定义
    SINGLETON_CLASS(TaskPool)

public:
    int init(int threadcount);
    int unInit(void);

    // 设置退出标识,不再接受新任务
    void setExit(void);

    // 返回任务数量
    int size(void);
    // 添加任务进队列 // 调用addTask(d)后,下次异步进入d->run_task();
    int addTask(ITaskRun* task, int delay_ms = 0);

private:
    // 任务线程入口
    static void* _ThreadFun(void* arg);

    // 程序退出时清理未完成的任务
    static void ClsTask(ITaskRun*& task);

private:
    Queue<ITaskRun*> m_tasks;
    Queue<pthread_t> m_tid;
    int m_threadcount; // 最大线程数
    bool m_exit;
};

#endif
