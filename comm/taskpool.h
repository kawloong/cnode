/******************************************************************* 
 *  summery:        ͬ������������
 *  author:         hejl
 *  date:           2016-09-21
 *  description:    
 ******************************************************************/
#ifndef __TASKPOOL_H_
#define __TASKPOOL_H_
#include <vector>
#include "public.h"
#include "queue.h"

// Ҫʵ�ִ˽ӿڵ�����ܷ���������ش���
struct ITaskRun
{
    // @param flag, �������ô�0, �˳���������
    virtual int run_task(int flag) = 0;
};

class TaskPool
{
    // ��ʵ��������
    SINGLETON_CLASS(TaskPool)

public:
    int init(int threadcount);
    int unInit(void);

    // �����˳���ʶ,���ٽ���������
    void setExit(void);

    // ������������
    int size(void);
    // ������������ // ����addTask(d)��,�´��첽����d->run_task();
    int addTask(ITaskRun* task, int delay_ms = 0);

private:
    // �����߳����
    static void* _ThreadFun(void* arg);

    // �����˳�ʱ����δ��ɵ�����
    static void ClsTask(ITaskRun*& task);

private:
    Queue<ITaskRun*> m_tasks;
    Queue<pthread_t> m_tid;
    int m_threadcount; // ����߳���
    bool m_exit;
};

#endif
