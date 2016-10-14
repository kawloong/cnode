/******************************************************************* 
 *  summery:        �߳�˽������
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
    struct event* notify_event; // ʹ��USR2�ź�֪ͨ�л���ԭ�߳�
    //struct event* sigint_event;
    //struct event* sigterm_event;
    HttpClient* icomet_cli;
    int pipefd[2]; // �����̼߳�ͨѶ (Ŀǰ����task -> resume)

    std::map<DealBase*, int> dealing; // ����ִ�е�ҵ��
    std::list<DealBase*> rmdeal; // ��ɾ����ҵ��
    std::list<DealBase*> resume; // ��Event�߳�ִ�е�ҵ��; // ����dealing���Ӽ�

    pthread_t threadid;
    pthread_mutex_t m_mutex; // ����resumeʱ������
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
