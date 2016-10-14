/******************************************************************* 
 *  summery:        业务处理基类
 *  author:         hejl
 *  date:           2016-09-12
 *  description:    
 ******************************************************************/
#ifndef _DEALBASE_H_
#define _DEALBASE_H_
#include <string>
#include <map>
#include <event2/http.h>
//#include "interface.h" // only test

/* 子类实现说明:
** 1.每个业务类(处理一个path)派生自class DealBase;
** 2.实现static Run()方法, 提供给请求到来时由Server调用;
** 3.要使用http客户端的类,请实现IHttpClientBack接口,配合class HttpClient使用;
** 4.若new产生了DealBase的子类,在业务处理完成后调用AsynDeleteMe()异步释放自身;
*/

using std::string;
using std::map;
struct ThreadData;
struct IHttpClientBack;

class DealBase
{
    const static int asyn_del_instance_time_sec = 4;

public:
    explicit DealBase(struct evhttp_request* req, ThreadData* tdata);
    virtual ~DealBase(void){}

    // 静态配置初始化
    static int Init(void);
    // 加载实例入口
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // 调用postResume()后主event线程调用resume()方法,此作测试,应由子类实现
    virtual void resume(int result);
    
protected:
    // Timer回调,用于异步删除处理对象
    static void AsynDelTimerCB(evutil_socket_t, short, void* arg);

    // 获取http-header值
    static int GetHeadStr(string& val, struct evhttp_request* req, const char* key);
    // 获取http-uri中的查询串key/value
    static int FindQueryStrValue(string& val, const struct evhttp_request* req, const char* key);
    static int ParseQueryMap(map<string,string>& kvmap, const struct evhttp_request* req);

    // 回复数据给客户请求端
    static int SendRespond(struct evhttp_request* req, const string& body);
    static int SendRespondFail(struct evhttp_request* req, const string& reson, int ecode=0);

protected:
    // 发出http请求回调,此作测试,应由子类实现
    //virtual int clireq_callback(int result, const string& body, const string& errmsg);

    // 异步删除自身this, 主要为避免易出错的递归删除
    void postDeleteMe(void);

    // 用于在任务线程切换回主event线程序, 方法立即返回,之后会解发resume()方法;
    int postResume( void );

    int pushICometReq(const string& cname, const string& content, IHttpClientBack* cb);
protected:
    struct evhttp_request* m_mainreq;
    ThreadData* m_tdata;
    string m_name;
    int m_begtime;
    bool m_finish;
    static string s_redis_poolname;
    static const string strnull;
};

#endif