/******************************************************************* 
 *  summery:        基于libevent的http服务处理类
 *  author:         hejl
 *  date:           2016-09-07
 *  description:    主http流程入口
 ******************************************************************/
#ifndef _SERVER_H_
#define _SERVER_H_
#include <event2/util.h>
#include "comm/public.h"
#include "threaddata.h"

/* server framework:

        + -- thread1(ThreadData)
        |                          +-- base&http 
Server--| -- thread2(ThreadData) --|-- httpclient
        |                          +-- dealer(request handle)
        + -- thread3(ThreadData)

*/

/* exitflag的含义:
** Server:s_exit 收到退出信号后即置位, 不再接受新connect请求;
** ThreadData::exitfg 是通知libevent处理线程退出标记,避免通知多次
*/
 
class Server
{
    // ----- Singleton class define ----- //
public:
    static Server* Instance(void);

private:
    Server();
    ~Server();
    Server(const Server&);           
    Server& operator=(const Server&);
    // ----------------------------------- //


public:
    int Init(int nthread, int port);
    int Run(int process_idx);
    void SetExit(void);
    int Stop(void);

private:
    // 线程入口
    static void* ThreadEntry(void* arg);
    int Run(ThreadData& td);
    int WaitExit(int wait_task_ms);

    // 信号回调,用于处理回主event线程
    static void PipeNotifyCB(evutil_socket_t fd, short events, void* arg);

public:
    static bool s_exit;

private:
    int threadcount;
    int listenfd;
    ThreadData* arrThreaData;

};

/* make sure class Thandl inherit from DealBase */
#define USE_CB(Class) cb_##Class
#define REQUEST_CB(Class) static void cb_##Class(struct evhttp_request *req, void *arg) \
{ ThreadData* td=(ThreadData*)arg; if(!Server::s_exit)Class::Run(req, td);\
else evhttp_send_error(req, HTTP_NOTFOUND, "server shutdown"); }

#endif
 
 