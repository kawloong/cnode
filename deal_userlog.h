/******************************************************************* 
 *  summery:        数据采集之用户日志
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    访问路径http://host:port/cloud/userlog?kk=v
 ******************************************************************/
#ifndef _DEAL_USERLOG_H_
#define _DEAL_USERLOG_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"

class DealUserlog: public DealBase, public ITaskRun
{
public:
    DealUserlog(struct evhttp_request* req, ThreadData* tdata);

    // 请求到达入口
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // 静态配置初始化
    static int Init(void);

private:
    // 解析请求参数
    int parseRequest(string& errback);
    int _parseRequestBody(const char* ptr, unsigned int len, string& errback);

    // apppend json string
    void appendJsonObject(string& objstr, const string& name,
        const string& val, bool comma, bool quot=true) const;
    int findObjectId( string& objetid ) const;

    // 用户上线处理
    int userOnline(void);
    // 用户下线处理
    int userOffline(void);
    // 用户认证登记
    int userAuth(void);

    // 响应并合理退出
    void respAndExit(void);

protected:
    virtual int run_task(int flag); // -任务线程中执行

    virtual void resume(int result);// -主event线程中执行

    struct UserData // 请求body用户数据
    {
        string time;
        string mac;
        string signal;
        string auth;
        string ip;
        string req_type;
        int status;
    };

private:
    string m_respbody;
    string m_routeid;
    string m_devid;
    string m_devmac;
    string m_session;
    string m_client_addr;
    string m_module;

    int m_step;
    bool m_isauth; // 认证类的上报

    UserData m_udata;

    static string s_mongopool_name;
    static string s_redispool_name;
};

#endif