#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "server.h"
#include "errdef.h"
#include "comm/public.h"
#include "comm/sock.h"
#include "comm/bmsh_config.h"
#include "comm/taskpool.h"
#include "redis/redispooladmin.h"
#include "mongo/mongopooladmin.h"
#include "dealbase.h"
#include "deal_auth.h"
#include "deal_cmdreq.h"
#include "deal_userlog.h"

static int INVALIDFD = -1;
bool Server::s_exit = false;

int Server::Init(int nthread, int port)
{
    if (INVALIDFD == listenfd)
    {
        std::string name;
        listenfd = Sock::create_listen(NULL, port, false); // 创建监听套接字,多进程多线程共用
        name = Sock::sock_name(listenfd, true, false);

        LOGINFO("LISTEN| svr=%s| fd=%d", name.c_str(), listenfd);
    }

    threadcount = nthread;

    TaskPool::Instance()->init(BmshConf::TaskThread());
    return (INVALIDFD==listenfd? ERR_SERVER_LISTEN: 0);
}

Server* Server::Instance( void )
{
    static Server s;
    return &s;
}

Server::Server(): threadcount(0), listenfd(INVALIDFD), arrThreaData(NULL)
{

}

Server::~Server()
{
    IFDELETE_ARR(arrThreaData);
}

void Server::SetExit( void )
{
    s_exit = true;
}


// static
// resume handle
void Server::PipeNotifyCB( evutil_socket_t fd, short events, void* arg )
{
    ThreadData* tdata = (ThreadData*)arg;
    DealBase* deal = NULL;
    int ret;
    bool b_resumecall = false;
    char buff[64] = {0};
    
    ret = read(fd, buff, sizeof(buff));
    for (int i = 0; i < ret && ret > 0; ++i)
    {
        switch (buff[i])
        {
        case 'r':
            if (!b_resumecall) // 每次触发只处理一次
            {
                b_resumecall = true;
                LOGDEBUG("USR2RESUME| msg=pipe notify| td=%p| tidx=%d", arg, tdata->thidx);

                while ( (deal = tdata->popResume()) )
                {
                    deal->resume(0);
                }
            }
            break;

        default:
            LOGERROR("PIPE_NOTIFY| msg=unknow char %c| ret=%d", buff[i], ret);
            break;
        }
    }
}

// 每个路径对应的回调处理类定义(处理类需存在static int Run() 方法)
REQUEST_CB(DealBase);
REQUEST_CB(DealAuth);
REQUEST_CB(DealCmdReq);
REQUEST_CB(DealUserlog);

// public
int Server::Run(int pidx)
{
    int ret = 0;
    int randseed;

    IFDELETE_ARR(arrThreaData);
    randseed = time(NULL)*(pidx+1);
    srand(randseed);

    // redis连接池初始化
    {
        ret = RedisConnPoolAdmin::Instance()->LoadPoolFromFile(BmshConf::RedisPoolConfFile().c_str());
        ERRLOG_IF1(ret, "RUN| msg=load redispool fail| file=%s| ret=%d", BmshConf::RedisPoolConfFile().c_str(), ret);
        if (ret) goto run_end_tag;
#if 0 // not check redis work
        Redis* rds_conn = NULL;
        ret = RedisConnPoolAdmin::Instance()->GetConnect(rds_conn, BmshConf::RedisPoolName().c_str());
        ERRLOG_IF1(ret, "RUN| msg=connect redis fail| name=%s| ret=%d", BmshConf::RedisPoolName().c_str(), ret);
        if (ret) goto  run_end_tag;
        RedisConnPoolAdmin::Instance()->ReleaseConnect(rds_conn);
#endif
    }

    // mongodb连接池初化
    {
        Mongo* mog_conn = NULL;
        ret = MongoConnPoolAdmin::Instance()->LoadPoolFromFile("/etc/bmsh_mongo_connpoll_conf.json");
        ERRLOG_IF1(ret, "RUN| msg=init mongopool fail| ret=%d", ret);
        if (ret) goto  run_end_tag;
        ret = MongoConnPoolAdmin::Instance()->GetConnect(mog_conn, BmshConf::MongoPoolName().c_str());
        ERRLOG_IF1(ret, "RUN| msg=connect mongodb fail| name=%s| ret=%d", BmshConf::MongoPoolName().c_str(), ret);
        if (ret) goto  run_end_tag;
        MongoConnPoolAdmin::Instance()->ReleaseConnect(mog_conn);
    }

    // 业务处理类初始化
    {
        ret = DealBase::Init();
        ERRLOG_IF1(ret, "RUN| msg=DealBase.Init fail| ret=%d", ret);
        if (ret) goto  run_end_tag;

        ret = DealUserlog::Init();
        ERRLOG_IF1(ret, "RUN| msg=DealUserlog.Init fail| ret=%d", ret);
        if (ret) goto  run_end_tag;
    }

    if (threadcount <= 0) threadcount = 1;
    ThreadData::s_total_threadcount = threadcount;

    // 多线程作业创建
    if (threadcount > 0 && threadcount < 1024)
    {
        arrThreaData = new ThreadData[threadcount];
        //evthread_use_pthreads();

        for (int i = 0; i < threadcount; ++i)
        {
            arrThreaData[i].thidx = i;
            arrThreaData[i].processidx = pidx;
            arrThreaData[i].randseed = randseed+i;

            pthread_create(&arrThreaData[i].threadid, NULL, ThreadEntry, &arrThreaData[i]);
        }
    }

    // 主线程监视其他线程运行状况, 等待退出信号
    ret = WaitExit(2000);

    for (int i = 0; arrThreaData && i < threadcount; ++i)
    {
        pthread_join(arrThreaData[i].threadid, NULL);
        arrThreaData[i].threadid = NULL;
    }

run_end_tag:
    MongoConnPoolAdmin::Instance()->DestroyPool(NULL);
    RedisConnPoolAdmin::Instance()->DestroyPool(NULL);
    close(listenfd);
    listenfd = INVALIDFD;
    IFDELETE_ARR(arrThreaData);
    return ret;
}

// private: run in each thread
int Server::Run(ThreadData& td)
{
    int ret;

    do 
    {
        td.~ThreadData();
        td.base = event_base_new();
        td.http = evhttp_new(td.base);
        ERRLOG_IF1BRK(NULL==td.base||NULL==td.http, ERR_SERVER_EVNEW,
            "CREATEHTTPSVR| event_base_new=%p| evhttp_new=%p", td.base, td.http);

        ret = evhttp_accept_socket(td.http, listenfd);
        ERRLOG_IF1BRK(ret, ERR_SERVER_ACCEPT, "CREATEHTTPSVR| msg=accept fail %d", ret);

        ERRLOG_IF0BRK(td.mkpipe(), ERR_SERVER_PIPE, "CREATEHTTPSVR| msg=pipe %d| fd=%d", ret, td.pipefd[0]);

        // 注册USR2信号
        td.notify_event = event_new(td.base, td.pipefd[0], EV_READ|EV_PERSIST, PipeNotifyCB, &td);
        evsignal_add(td.notify_event, NULL);

        evhttp_set_cb(td.http, "/cloud/auth", USE_CB(DealAuth), &td);
        evhttp_set_cb(td.http, "/cloud/command", USE_CB(DealCmdReq), &td);
        evhttp_set_cb(td.http, "/cloud/userlog", USE_CB(DealUserlog), &td);
        evhttp_set_gencb(td.http, USE_CB(DealBase), &td);

        LOGDEBUG("CREATESVR| thread-%d.base=%p", td.thidx, td.base);
        ret = event_base_dispatch(td.base);
    }
    while (0);

    LOGINFO("RUNEND| msg=thread-%d ending| ret=%d", td.thidx, ret);
    return ret;
}

// 线程入口
void* Server::ThreadEntry( void* arg )
{
    ThreadData* ptd = (ThreadData*)arg;
/*
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_BLOCK, NULL, &set);
    //sigaddset(&set, SIGUSR1);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGTERM);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
*/
    LOGINFO("THREADRUN| thread-%d", ptd->thidx);
    Instance()->Run(*ptd);
    return 0;
}

int Server::WaitExit( int wait_task_ms )
{
    // 主线程等待
    int ret = 0;
    int sret, signo;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sret = sigwait(&set, &signo);

    ERRLOG_IF0(SIGINT==signo||SIGTERM==signo, "RUN| msg=unknow signal recv| sig=%d", signo);

    // 1. 拒绝新请求到来
    s_exit = true;

    // 2. 等待Task任务完成
    const int wait_sleep_step_ms = 300;
    int tskcnt = TaskPool::Instance()->size();
    for (int tms = 0; tskcnt > 0 && tms < wait_task_ms;)
    {
        usleep(wait_sleep_step_ms);
        tms += wait_sleep_step_ms;
        tskcnt = TaskPool::Instance()->size();
    }

    TaskPool::Instance()->setExit();
    usleep(1); // noop
    ERRLOG_IF1(tskcnt>0, "RUN| msg=still %d task undeal", TaskPool::Instance()->size());
    // to do: force end task here
    TaskPool::Instance()->unInit();

    // 3. 等待Event线程完成
    for (int i = 0; arrThreaData && i < threadcount; ++i)
    {
        ret = pthread_kill(arrThreaData[i].threadid, SIGUSR1);
        LOGDEBUG("STOP| msg=kill| thread-%d |base=%p| ret=%d", i, arrThreaData[i].base, ret);
    }

    return 0;
}

// @summery: 收到SIGINT或SIGTREM信号，打断event_loop，可靠退出程序
int Server::Stop( void )
{
    int ret = 0;
    pthread_t ctid = pthread_self();

    if (arrThreaData)
    {
        struct timeval tv;
        tv.tv_sec = 3; // shutdown after 3 sec // work good
        tv.tv_usec = 0;

        for (int i = 0; i < threadcount; ++i)
        {
            if (ctid == arrThreaData[i].threadid)
            {
                arrThreaData[i].exitfg = true;
                ret = event_base_loopexit(arrThreaData[i].base, &tv);

                LOGDEBUG("STOP| msg=call loopbreak base=%p| thread-%d| ret=%d", arrThreaData[i].base, i, ret);
            }
        }
    }

    return 0;
}
