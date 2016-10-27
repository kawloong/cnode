#include <signal.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/event.h>
#include "dealbase.h"
#include "comm/public.h"
#include "comm/bmsh_config.h"
#include "httpclient.h"
#include "threaddata.h"
#include "errdef.h"


const string DealBase::strnull;
string DealBase::s_redis_poolname;

DealBase::DealBase(struct evhttp_request* req, ThreadData* tdata): m_finish(false)
{
    m_mainreq = req;
    m_tdata = tdata;
    m_begtime = (int)time(NULL);
    tdata->pushDeal(this);
}

// static
int DealBase::Init( void )
{
    s_redis_poolname = BmshConf::RedisPoolName();
    return 0;
}

// just test func
// int DealBase::clireq_callback( int result, const string& body, const string& errmsg )
// {
//     struct evbuffer* buf = evhttp_request_get_output_buffer(m_mainreq);
// 
//     evbuffer_add_printf(buf,"recode=%d, body=%s, err=%s", result, body.c_str(), errmsg.c_str());
//     evhttp_send_reply(m_mainreq, 200, "OK", buf);
// 
//     m_finish = true;
//     postDeleteMe();
// 
//     return 0;
// }

// 业务处理完成后调用, 交由异步机制删除
void DealBase::postDeleteMe(void)
{
    m_tdata->removeDeal(this, true);

    WARNLOG_IF1(pthread_self() != m_tdata->threadid,
        "POSTDELME| msg=ex-thread call timer| tidx=%d",
        m_tdata->thidx);

    if (NULL == m_tdata->asyn_timer)
    {
        struct timeval tvl;
        m_tdata->asyn_timer = evtimer_new(this->m_tdata->base, AsynDelTimerCB, m_tdata);

        tvl.tv_sec = asyn_del_instance_time_sec;
        tvl.tv_usec = 0;
        
        evtimer_add(m_tdata->asyn_timer, &tvl);
    }

    m_mainreq = NULL;
    m_tdata = NULL;
}

// 同步, 直接在调用线程上delete deal
void DealBase::ForceDelete( DealBase* deal )
{
    if (deal)
    {
        deal->m_tdata->removeDeal(deal, false);
        delete deal;
    }
}

// 异步回收
void DealBase::AsynDelTimerCB(evutil_socket_t, short, void* arg)
{
    ThreadData* ptd = (ThreadData*)arg;

    LOGDEBUG("ASYNDELTIMER| msg=do delete Dealer| dealing=%d| rmcount=%d| thidx=%d",
        (int)ptd->dealing.size(), (int)ptd->rmdeal.size(), ptd->thidx);

    ptd->delDealer();
    if (ptd->asyn_timer)
    {
        event_free(ptd->asyn_timer);
        ptd->asyn_timer = NULL;
    }
}

// for example, only debug use
int DealBase::Run( struct evhttp_request* req, ThreadData* tdata )
{
    int ret;
#if 0
    const char* uri = NULL;
    const char* query = NULL;
    //struct evkeyvalq qkv;

    uri = evhttp_request_get_uri(req);
    query = evhttp_uri_get_query(evhttp_request_get_evhttp_uri(req));
    //ret = evhttp_parse_query_str(uri, &qkv);

    LOGDEBUG("DEALBASERUN| uri=%s | query=%s", uri, query);

    // first create
    if (NULL == tdata->icomet_cli)
    {
        tdata->icomet_cli = new HttpClient(tdata->base, "www.qq.com", 80); 
        tdata->icomet_cli->setTimeOut(2);
    }

    DealBase* deal = new DealBase(req, tdata);

    ret = tdata->icomet_cli->request("/", deal); // always 0
    LOGDEBUG("DEALRUN| msg=put request result %d| deal=%p", ret, deal);
#endif

    ret = SendRespondFail(req, "undefine path");
    return ret;
}

// 向icomet发http请求
int DealBase::pushICometReq(const string& cname, const string& content, IHttpClientBack* cb)
{
    int ret;
    char* encode_pa = NULL;
    string query;

    IFRETURN_N(cname.empty()||content.empty()||NULL==cb, ERR_SYSBUG);

    // first create
    if (NULL == m_tdata->icomet_cli)
    {
        string icomet_host = BmshConf::IcometIHost();
        int icomet_adminport = BmshConf::IcometAdminPort();
        m_tdata->icomet_cli = new HttpClient(m_tdata->base, icomet_host.c_str(), icomet_adminport); 
        m_tdata->icomet_cli->setTimeOut(2);
    }

    encode_pa = evhttp_uriencode(content.c_str(), content.length(), 0);
    IFRETURN_N(!encode_pa, ERR_SYSTEM);
    query = "/push?cname="+cname + "&content="+encode_pa;
    free(encode_pa);
    ret = m_tdata->icomet_cli->request(query, cb);
    
    return ret;
}

// @summery: 转由原Event线程执行处理, 一般地在task线程中执行阻塞耗时任务,
//    在任务完成后, 执行流程转回原event线程处理响应消息工作;
//    此方法应在task线程中执行; 最后在event线程调用到resume()方法;
int DealBase::postResume( void )
{
    int ret = 0;

    m_tdata->pushResume(this);
    //pthread_kill(m_tdata->threadid, SIGUSR2);
    write(m_tdata->pipefd[1], "r", 1);

    return ret;
}

void DealBase::resume( int result )
{
    LOGERROR("BASERESUME| msg=you may overload resume at drived class");
}

// static
int DealBase::GetHeadStr(string& val, struct evhttp_request* req, const char* key)
{
    const char* str;
    struct evkeyvalq* kv_str;

    IFRETURN_N(NULL==req || NULL==key, 1);
    kv_str = evhttp_request_get_input_headers(req);
    IFRETURN_N(kv_str, 3);

    str = evhttp_find_header(kv_str, key);
    val = str? str: strnull;
    return 0;
}

// static
int DealBase::FindQueryStrValue(string& val, const struct evhttp_request* req, const char* key)
{
    int ret;
    const char* requri;
    const char* str;
    struct evkeyvalq query_kv_str;

    IFRETURN_N(NULL==req || NULL==key, 1);

    requri = evhttp_request_get_uri(req);
    IFRETURN_N(NULL==requri, 2);

    ret = evhttp_parse_query_str(requri, &query_kv_str);
    IFRETURN_N(ret, 3);

    str = evhttp_find_header(&query_kv_str, key);
    val = str? str: strnull;

    evhttp_clear_headers(&query_kv_str);
    return 0;
}

// static
// 解析http请求的query查询字符串输出到map中
int DealBase::ParseQueryMap(map<string,string>& kvmap, const struct evhttp_request* req)
{
    int ret;
    const struct evhttp_uri * requri;
    const char* query_str;
    struct evkeyvalq query_kv_str;

    IFRETURN_N(NULL==req, 1);

    requri = evhttp_request_get_evhttp_uri(req);
    IFRETURN_N(NULL==requri, 2);

    query_str = evhttp_uri_get_query(requri);
    IFRETURN_N(NULL==query_str, 3);

    ret = evhttp_parse_query_str(query_str, &query_kv_str);
    IFRETURN_N(ret, 4);

    for (struct evkeyval* kv = query_kv_str.tqh_first;
        kv;  kv = kv->next.tqe_next)
    {
        if (kv->key && kv->value)
        {
            kvmap[kv->key] = kv->value;
        }
    }

    evhttp_clear_headers(&query_kv_str);
    return ret;
}

// static
// 回复数据给客户请求端
int DealBase::SendRespond( struct evhttp_request* req, const string& body )
{
    int ret = 0;
    struct evbuffer* outbuf = evhttp_request_get_output_buffer(req);

    ret = evbuffer_add_printf(outbuf, "%s", body.c_str());
    evhttp_send_reply(req, 200, "OK", NULL);
    return ret;
}

// static
// 回复Json格式的失败消息
int DealBase::SendRespondFail( struct evhttp_request* req, const string& reson, int ecode )
{
    int ret = 0;
    struct evbuffer* outbuf = evhttp_request_get_output_buffer(req);

    if (ecode)
    {
        ret =evbuffer_add_printf(outbuf, "{\"status\": \"FAIL\", \"code\": %d, \"reason\": \"%s\"}",
            ecode, reson.c_str());
    }
    else
    {
        ret =evbuffer_add_printf(outbuf, "{\"status\": \"FAIL\", \"reason\": \"%s\"}", reson.c_str());
    }

    evhttp_send_reply(req, 200, "OK", NULL);
    return ret;
}

// static
// 相当于FCGX_GetParam("REMOTE_ADDR", request->envp);
int DealBase::GetRemoteAddr( string& addr, struct evhttp_request* req )
{
    int ret = -1;

    struct evhttp_connection* conn = evhttp_request_get_connection(req);
    if (conn)
    {
        char* pstr = NULL;
        uint16_t port;
        evhttp_connection_get_peer(conn, &pstr, &port);
        if (pstr)
        {
            addr = pstr;
            ret = 0;
        }
    }

    return ret;
}


