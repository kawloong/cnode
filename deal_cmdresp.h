/******************************************************************* 
 *  summery:        接受命令请求类
 *  author:         hejl
 *  date:           2016-09-28
 *  description:    访问路径http://host:port/cloud/command/response?kk=v
 ******************************************************************/
#ifndef _DEAL_CMDRESP_H_
#define _DEAL_CMDRESP_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"


class DealCmdResp: public DealBase, public ITaskRun
{
public:
    DealCmdResp(struct evhttp_request* req, ThreadData* tdata);

    // 请求到达入口
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // 静态配置初始化
    static int Init(void);

protected:
    virtual int run_task(void); // -任务线程中执行

    virtual void resume(int result);// -主event线程中执行

private:

private:


};

#endif