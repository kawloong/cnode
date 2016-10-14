/******************************************************************* 
 *  summery:        设置授权业务类
 *  author:         hejl
 *  date:           2016-09-23
 *  description:    访问路径http://host:port/cloud/auth?kk=v
 ******************************************************************/
#ifndef _DEAL_AUTH_H_
#define _DEAL_AUTH_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"

class DealAuth: public DealBase, public ITaskRun
{
public:
    DealAuth(struct evhttp_request* req, ThreadData* tdata);

    // 请求到达入口
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // 静态配置初始化
    static int Init(void);

protected:
    virtual int run_task(int flag); // -任务线程中执行

    virtual void resume(int result);// -主event线程中执行

private:
    static string MakeSession(ThreadData* tdata);

private:
    string device_id;
    string device_mac;
    string device_model;
    string firmware_version;
    string respbody;
    static string s_icomet_jsonstr;

};

#endif