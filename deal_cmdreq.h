/******************************************************************* 
 *  summery:        接受命令请求类
 *  author:         hejl
 *  date:           2016-09-28
 *  description:    访问路径http://host:port/cloud/command?kk=v
 ******************************************************************/
#ifndef _DEAL_CMDREQ_H_
#define _DEAL_CMDREQ_H_
#include <list>
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"
#include "httpclient.h"


class DealCmdReq: public DealBase, public ITaskRun
{
public:
    DealCmdReq(struct evhttp_request* req, ThreadData* tdata);
    virtual ~DealCmdReq(void);

    // 请求到达入口
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // 静态配置初始化
    static int Init(void);

protected:
    virtual int run_task(int flag); // -任务线程中执行

    virtual void resume(int result);// -主event线程中执行

private:
    int parseReqJson(const char* bodydata);
    int checkOnline(void);
    bool waitPushResp(void);
    int waitRedisResult(void);

    struct RouteMsg: public IHttpClientBack
    {
        string mac;
        string req;
        string rsp;
        string cmd;
        int status; // RouteMsgStatus
        DealCmdReq* parent;

        virtual int clireq_callback(int result, const string& body, const string& errmsg);
    };

    enum RouteMsgStatus
    {
        RMS_SUCESS = 0,
        RMS_INIT,
        RMS_SEND,
        RMS_RESP,
        RMS_OFFLINE = -1,
        RMS_PUSHFAIL = -2,
        RMS_FAIL = -3,
    };

private:
    string m_cmdid;
    std::list<RouteMsg*> m_routemsg_lst;
    
    string m_respbody;
    int m_cmdtype;
    unsigned int m_online_count;
    unsigned int m_waitcmd_sec;
    int m_step;

    static const int WAIT_CMDRESP_TIME_SEC = 10;
};

#endif