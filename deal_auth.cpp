#include "deal_auth.h"
#include "errdef.h"
#include "threaddata.h"
#include "comm/public.h"
#include "comm/md5.h"
#include "comm/strparse.h"
#include "comm/bmsh_config.h"
#include "redis/redispooladmin.h"


static const string g_str_secret = "100MSH";
string DealAuth::s_icomet_jsonstr;

DealAuth::DealAuth( struct evhttp_request* req, ThreadData* tdata ): DealBase(req, tdata)
{
}

// static
// xxx/cloud/auth请求入口
int DealAuth::Run( struct evhttp_request* req, ThreadData* tdata )
{
    int ret;
    std::map<string, string> querykv;
    string device_id;
    string device_mac;
    string device_model("H351");
    string firmware_version("Unknown");
    string random;
    string md5;
    string err_reason;

// 参数提取, 必选时reason是出错提示, 可选参数用reason=0
#define PickStrFromMap(name, reason) { \
std::map<string, string>::iterator it = querykv.find(#name); \
if(querykv.end() != it){ name = it->second;}\
else if(reason){ret = ERR_PARAMETER; err_reason=(const char*)reason; break;}\
else{}}

    do 
    {
        ret = ParseQueryMap(querykv, req);
        PickStrFromMap(device_id, "leak of device_id");
        PickStrFromMap(device_mac, "leak of device_mac");
        PickStrFromMap(md5, "leak of md5");
        PickStrFromMap(device_model, 0);
        PickStrFromMap(firmware_version, 0);
        PickStrFromMap(random, 0);

#if 0 // 调试时关闭校验
        // MD5签名合法性认证
        {
            MD5 calcmd5(device_mac + random + g_str_secret);
            if (md5 != calcmd5.toString())
            {
                ret = ERR_MD5INVALID;
                err_reason = "invalid md5";
                LOGERROR("AUTHRUN| msg=invalid md5| calc_md5=%s| param_md5=%s",
                    calcmd5.toString().c_str(), md5.c_str());
                break;
            }
        }
#endif

        // 创建实例,加入任务队列
        {
            DealAuth* deal = new DealAuth(req, tdata);
            deal->device_id = device_id;
            deal->device_mac = device_mac;
            deal->device_model = device_model;
            deal->firmware_version = firmware_version;

            ret = TaskPool::Instance()->addTask(deal); // 调用addTask(d)后,下次异步进入d->run_task();
            if (ret)
            {
                delete deal;
                err_reason = "system error";
                break;
            }
        }
    }
    while (0);

    if (ret) // 参数不合法的响应
    {
        ret = SendRespondFail(req, err_reason, ret);
    }
    
    
    return ret;
}

// 产生一个session随机数
string DealAuth::MakeSession( ThreadData* tdata )
{
    static int s_int = 0;
    int idx = tdata->processidx * tdata->s_total_threadcount + tdata->thidx;
    int r = rand_r((unsigned int*)&tdata->randseed)%100000;
    char buff[32];

    snprintf(buff, sizeof(buff), "%05d%d%03d", r, idx, ++s_int%1000);
    return buff;
}

// 执行耗时任务
int DealAuth::run_task( int flag )
{
    int ret = 0;
    Redis* rds_conn = NULL;
    const static char* token = "3103353c70da0ca679704eaa5633b346";
    const char* reason = "";
    string strsession = MakeSession(m_tdata);
    string device_key = "device:" + device_mac;
    map<string, string> kvstr;
    

    do
    {
        kvstr["id"] = device_id;
        kvstr["keepalive"] = StrParse::Itoa(m_begtime);
        kvstr["version"] = firmware_version;
        kvstr["model"] = device_model;
        kvstr["session"] = strsession;

        ret = RedisConnPoolAdmin::Instance()->GetConnect(rds_conn, s_redis_poolname.c_str());
        reason = "redis connect fail";
        ERRLOG_IF1BRK(ret, ERR_REDIS, "AUTHRUN_TASK| msg=get conn fail| ret=%d| mac=%s", ret, device_mac.c_str());

        ret = rds_conn->hmset(device_key.c_str(), kvstr);
        reason = "hmset fail";
        ERRLOG_IF1BRK(ret, ERR_REDIS, "AUTHRUN_TASK| msg=redis hmset fail| ret=%d| mac=%s", ret, device_mac.c_str());

        reason = "ok";
        StrParse::AppendFormat(respbody, "{\"status\": \"SUCCESS\", "
            "\"token\": \"%s\", \"session\": \"%s\", %s }",
            token, strsession.c_str(), s_icomet_jsonstr.c_str());
        ret = 0;
    }
    while (0);
    
    if (rds_conn)
    {
        RedisConnPoolAdmin::Instance()->ReleaseConnect(rds_conn);
        rds_conn = NULL;
    }

    if (ret)
    {
        StrParse::AppendFormat(respbody, "{\"status\": \"FAIL\", \"reason\": \"%s\" }", reason);
    }
    
    ret = this->postResume();

    return ret;
}

// event线程执行,响应消息
void DealAuth::resume( int result )
{
    int ret = SendRespond(m_mainreq, respbody);
    if (ret <= 0)
    {
        LOGERROR("AUTH_RESP| msg=SendRespond fail| ret=%d| mac=%s", ret, device_mac.c_str());

        evhttp_connection_free(evhttp_request_get_connection(m_mainreq)); // 不能发送,提前关闭
        m_mainreq = NULL;
    }

    m_finish = true;
    postDeleteMe();
}

int DealAuth::Init( void )
{
    int ret;

    ret = StrParse::AppendFormat(s_icomet_jsonstr,
        "\"server\": \"%s\", \"port\": %d,"
        "\"interval\": %d,"
        "\"noop\": %d, \"log_server\": \"%s\"",
        BmshConf::IcometEHost().c_str(), BmshConf::IcometFrontPort(),
        BmshConf::IcometInterval(),
        30, BmshConf::LogServer().c_str());

    return ret;
}






