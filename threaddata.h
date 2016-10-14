/******************************************************************* 
 *  summery:        线程私有数据
 *  author:         hejl
 *  date:           2016-09-12
 *  description:    
 ******************************************************************/
#ifndef _THREADDATA_H_
#define _THREADDATA_H_
#include <map>
#include <list>

class HttpClient;
class DealBase;

struct ThreadData
{
    struct event_base *base;
    struct evhttp *http;
    struct event* asyn_timer;
    struct event* notify_event; // 使用USR2信号通知切换回原线程
    //struct event* sigint_event;
    //struct event* sigterm_event;
    HttpClient* icomet_cli;
    int pipefd[2]; // 用于线程间通讯 (目前用在task -> resume)

    std::map<DealBase*, int> dealing; // 正在执行的业务
    std::list<DealBase*> rmdeal; // 待删除的业务
    std::list<DealBase*> resume; // 待Event线程执行的业务; // 属于dealing的子集

    pthread_t threadid;
    pthread_mutex_t m_mutex; // 访问resume时互斥用
    int thidx;
    int processidx;
    int randseed;
    bool exitfg;

    static int s_total_threadcount;

    bool mkpipe(void);

    void pushDeal(DealBase* ptr);
    void delDealer(void);
    void delAllDealer(void);
    bool removeDeal(DealBase* ptr);

    void pushResume(DealBase* ptr);
    DealBase* popResume(void);

    ThreadData();
    ~ThreadData();
};


#endif
