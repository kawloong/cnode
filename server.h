/******************************************************************* 
 *  summery:        ����libevent��http��������
 *  author:         hejl
 *  date:           2016-09-07
 *  description:    ��http�������
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

/* exitflag�ĺ���:
** Server:s_exit �յ��˳��źź���λ, ���ٽ�����connect����;
** ThreadData::exitfg ��֪ͨlibevent�����߳��˳����,����֪ͨ���
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
    // �߳����
    static void* ThreadEntry(void* arg);
    int Run(ThreadData& td);
    int WaitExit(int wait_task_ms);

    // �źŻص�,���ڴ������event�߳�
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
 
 