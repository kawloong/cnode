#include <event2/buffer.h>
#include "deal_userlog.h"
#include "errdef.h"
#include "threaddata.h"
#include "idmanage.h"
#include "comm/public.h"
#include "comm/bmsh_config.h"
#include "comm/strparse.h"
#include "rapidjson/json.hpp"
#include "redis/redispooladmin.h"
#include "mongo/mongopooladmin.h"

string DealUserlog::s_mongopool_name;
string DealUserlog::s_redispool_name;

DealUserlog::DealUserlog( struct evhttp_request* req, ThreadData* tdata ): DealBase(req, tdata)
{
    m_udata.req_type = "unknow";
}

int DealUserlog::Init( void )
{
    int ret;
    int hostid = BmshConf::NodeHostID();

    s_mongopool_name = BmshConf::MongoPoolName();
    s_redispool_name = BmshConf::RedisPoolName();

    if (hostid <= 0 || s_mongopool_name.empty())
    {
        LOGERROR("USERLOGINIT| msg=userlog init fail| hostid=%d| mongopool=%s",
            hostid, s_mongopool_name.c_str());
        ret = -1;
    }
    else
    {
        ret = IdMgr::Instance()->init(BmshConf::NodeIpcKEY(), hostid);
    }

    return ret;
}

// static
// xxx/cloud/auth请求入口
int DealUserlog::Run( struct evhttp_request* req, ThreadData* tdata )
{
    int ret;
    string err_reason;
    DealUserlog* deal = NULL;

    do 
    {
        // 创建实例,加入任务队列
        deal = new DealUserlog(req, tdata);

        ret = deal->parseRequest(err_reason);
        IFBREAK(ret);

        ret = TaskPool::Instance()->addTask(deal); // 调用addTask(d)后,下次异步进入d->run_task();
    }
    while (0);

    if (ret) // 参数不合法的响应
    {
        LOGERROR("USERLOGPARSE| msg=parse body fail| ret=%d| err=%s| i=%s",
            ret, err_reason.c_str(), deal?deal->m_name.c_str(): "@");

        ret = SendRespondFail(req, err_reason, ret);
        ForceDelete(deal);
        ret = 0;
    }

    return ret;
}

int DealUserlog::parseRequest( string& err_reason )
{
    int ret;
    std::map<string, string> querykv;
    char* bodyheap = NULL;

// 参数提取, 必选时reason是出错提示, 可选参数用reason=0
#define PickStrFromMap(valname, key, reason) { \
std::map<string, string>::iterator it = querykv.find(key); \
if(querykv.end() != it){ valname = it->second;}\
else if(reason){ret = ERR_PARAMETER; err_reason=(const char*)reason; break;}\
else{}}
#define PARSEERR_BREAK(exp, n, str) if( (exp) ){ret=n; err_reason=str; break;}

    do 
    {
        GetHeadStr(m_client_addr, m_mainreq, "X-Forwarded-For");
        if (m_client_addr.empty())
        {
            GetRemoteAddr(m_client_addr, m_mainreq);
        }

        m_name = m_client_addr;
        ret = ParseQueryMap(querykv, m_mainreq);
        PickStrFromMap(m_routeid, "device_id", "leak of device_id");
        PickStrFromMap(m_devmac, "device_mac", "leak of device_mac");
        PickStrFromMap(m_session, "session", NULL);


        m_name = m_devmac + "@" + m_client_addr;
        m_step = 1;

        // http-body json parse
        {
            char strbodytmp[1024];
            size_t sztmp;
            evbuffer* bodybuff = evhttp_request_get_input_buffer(m_mainreq);
            size_t bodylen = bodybuff? evbuffer_get_length(bodybuff): 0;
            PARSEERR_BREAK(bodylen <= 0, ERR_JSON_FORMAT, "body empty");
            if (bodylen <= sizeof(strbodytmp)-1)
            {
                sztmp = evbuffer_remove(bodybuff, strbodytmp, bodylen);
                PARSEERR_BREAK(sztmp != bodylen, ERR_SYSBUG, "evbuffer_remove ret unmatch");
                strbodytmp[bodylen] = 0;
                ret = _parseRequestBody(strbodytmp, bodylen, err_reason);
            }
            else
            {
                bodyheap = (char*)malloc(bodylen + 1);
                sztmp = evbuffer_remove(bodybuff, bodyheap, bodylen);
                PARSEERR_BREAK(sztmp != bodylen, ERR_SYSBUG, "evbuffer_remove ret unmatch");
                bodyheap[bodylen] = 0;
                ret = _parseRequestBody(bodyheap, bodylen, err_reason);
            }
        }
    }
    while(0);

    IFFREE(bodyheap);
    return ret;
}

int DealUserlog::_parseRequestBody( const char* ptr, unsigned int len, string& errback )
{
    int ret = 1; // test

#undef PARSEERR_BREAK
#define PARSEERR_BREAK(exp, errmsg) if( (exp) ){ret=ERR_JSON_FORMAT; errback=errmsg; break;}

    do 
    {
        Document doc;
        const Value* val = NULL;

        if (doc.ParseInsitu((char*)ptr).HasParseError())
        {
            StrParse::AppendFormat(errback, "json-doc err %d", (int)doc.GetParseError());
            LOGERROR("PARSEUSERLOG| body=%s", ptr);
            ret = ERR_JSON_FORMAT;
            break;
        }

        // 解析json http体内容
        ret = Rjson::GetStr(m_module, "module", &doc);
        PARSEERR_BREAK(ret, "module param");
        ret = Rjson::GetObject(&val, "data", &doc);
        PARSEERR_BREAK(ret || NULL==val, "data{} param");

        ret = Rjson::GetStr(m_udata.mac, "mac", val);
        PARSEERR_BREAK(ret, "no mac param");
        ret = Rjson::GetStr(m_udata.time, "time_sec", val);
        PARSEERR_BREAK(ret, "no mac time");
        ret = Rjson::GetInt(m_udata.status, "status", val);
        PARSEERR_BREAK(ret || (m_udata.status < 0 && m_udata.status > 2), "status param");

        Rjson::GetStr(m_udata.signal, "signal", val);
        Rjson::GetStr(m_udata.auth, "auth", val);
        Rjson::GetStr(m_udata.ip, "ip", val);
        m_isauth = !m_udata.auth.empty();

        ret = 0;
    }
    while (0);
    
    return ret;
}



// 执行耗时任务
int DealUserlog::run_task( int flag )
{
    int ret = ERR_UNDEFINE;

    if (/*m_udata.ip.empty() && */m_udata.auth.empty())
    {
        if (m_udata.status)
        {
            m_udata.req_type = "offline";
            ret = userOffline();
        }
        else
        {
            m_udata.req_type = "online";
            ret = userOnline();
        }
    }
    else
    {
        StrParse::AppendFormat(m_udata.req_type, "auth%d", m_udata.status);
        ret = userAuth();
    }

    m_respbody = "{";
    if (ret)
    {
        appendJsonObject(m_respbody, "status", "FAIL", true);
        appendJsonObject(m_respbody, "reason", StrParse::Itoa(ret), false);
    }
    else
    {
        appendJsonObject(m_respbody, "status", "SUCCESS", false);
    }
    
    m_respbody += "}";

    ret = postResume();
    
    return ret;
}

// 用户上线 (运行在task线程)
int DealUserlog::userOnline( void )
{
    int ret;
    bool btmp;
    Redis* redis = NULL;

    do 
    {
        string objid;
        string mongocmd;
        string devkey;

        ret = RedisConnPoolAdmin::Instance()->GetConnect(redis, s_redispool_name.c_str());
        ERRLOG_IF1BRK(ret, ERR_REDIS, "USERLOGONLINE| msg=get redis pool fail| ret=%d| poolname=%s",
            ret, s_redispool_name.c_str());

        /*               获取devid                  */
        devkey = "device_id_mac:" + m_devmac;
        ret = redis->get(m_devid, devkey.c_str());
        ERRLOG_IF1BRK(ret, ERR_SYSTEM, "USERLOGONLINE| msg=deviceid no found| ret=%d| i=%s", ret, m_name.c_str());

        /*               产生objectid               */ 
        ret = IdMgr::Instance()->getObjectId(objid);
        WARNLOG_IF1(ret, "USERLOGONLINE| msg=make objectid fail| ret=%d| i=%s", ret, m_name.c_str());
        if (objid.empty())
        {
            StrParse::AppendFormat(objid, "%x%x%x", (int)time(NULL),
                m_tdata->thidx,  rand()%10000);
        }

        /*             构造mongo插入语句             */
        mongocmd = "{";
        btmp = !m_udata.signal.empty();
        appendJsonObject(mongocmd, "_id", objid, true);
        appendJsonObject(mongocmd, "devid", m_devid, true);
        appendJsonObject(mongocmd, "devmac", m_devmac, true);
        appendJsonObject(mongocmd, "mac", m_udata.mac, true);
        //appendJsonObject(mongocmd, "ip", (m_udata.ip.empty()? m_client_addr: m_udata.ip), true);
        appendJsonObject(mongocmd, "uptime", m_udata.time, btmp, false);

        if (btmp)
        {
            appendJsonObject(mongocmd, "signal", m_udata.signal, false);
        }
        mongocmd += "}";

        /*             向mongodb发出命令             */
        {
            Mongo* mongo = NULL;
            ret = MongoConnPoolAdmin::Instance()->GetConnect(mongo, s_mongopool_name.c_str());
            ERRLOG_IF1BRK(ret, ERR_MONGO, "USERLOGONLINE| msg=get mongo connect fail| ret=%d| i=%s",
                ret, m_name.c_str());

            ret = mongo->insert(mongocmd);
            MongoConnPoolAdmin::Instance()->ReleaseConnect(mongo);
            ERRLOG_IF1BRK(ret, ERR_MONGO_INSERT, "USERLOGONLINE| msg=mongo insert fail| ret=%d| i=%s| err=%s",
                ret, m_name.c_str(), mongo->geterr().c_str());
        }

        /*             插入一条redis记录             */
        {
            string key = "objid:" + m_udata.mac;
            ret = redis->set(key.c_str(), objid.c_str());
            WARNLOG_IF1(ret, "USERLOGONLINE| msg=redis set fail| key=%s| value=%s",
                key.c_str(), objid.c_str());
            ret = 0;
        }

    }
    while (0);

    if (redis)
    {
        RedisConnPoolAdmin::Instance()->ReleaseConnect(redis);
    }

    return ret;
}

// 用户下线 (运行在task线程)
int DealUserlog::userOffline( void )
{
    int ret;

    do 
    {
        string objid;
        string mongofind, mongofield;
        string mongoset;
        string result;

        ret = findObjectId(objid);
        IFBREAK(ret);

        /*             向mongodb发出命令             */
        {
            Mongo* mongo = NULL;
            ret = MongoConnPoolAdmin::Instance()->GetConnect(mongo, s_mongopool_name.c_str());
            ERRLOG_IF1BRK(ret, ERR_MONGO, "USERLOGOFFLINE| msg=get mongo connect fail| ret=%d| i=%s",
                ret, m_name.c_str());
            
            /*             构造mongo更新语句             */
            mongofield = "{\"_id\": \"" + objid + "\"}";
            mongoset = "{ \"$set\": {\"downtime\":" + m_udata.time + "} }";
            ret = mongo->update(mongofield, mongoset, false, false);
            if (ret)
            {
                LOGERROR("USERLOGOFFLINE| msg=mongo update fail| ret=%d| i=%s| err=%s",
                    ret, m_name.c_str(), mongo->geterr().c_str());
                ret = ERR_MONGO_UPDATE;
            }
            
            MongoConnPoolAdmin::Instance()->ReleaseConnect(mongo);
            IFBREAK(ret);
        }

        /*             删除redis key记录             */
        {
            Redis* redis = NULL;
            string key = "objid:" + m_udata.mac;

            ret = RedisConnPoolAdmin::Instance()->GetConnect(redis, s_redispool_name.c_str());
            WARNLOG_IF1BRK(ret, 0, "USERLOGONLINE| msg=get redis pool fail| ret=%d| poolname=%s",
                ret, s_redispool_name.c_str());
            ret = redis->del(key.c_str());
            RedisConnPoolAdmin::Instance()->ReleaseConnect(redis);
            WARNLOG_IF1(ret, "USERLOGONLINE| msg=redis del fail| key=%s| value=%s",
                key.c_str(), objid.c_str());
        }

    }
    while (0);

    return ret;
}

// 用户认证
int DealUserlog::userAuth( void )
{
    int ret;
    Mongo* mongo = NULL;

    do 
    {
        string objid;
        string mongofind, mongofield;
        string mongoset;
        string result;

        ret = findObjectId(objid);
        IFBREAK(ret);

        /*             向mongodb发出命令             */
        ret = MongoConnPoolAdmin::Instance()->GetConnect(mongo, s_mongopool_name.c_str());
        ERRLOG_IF1BRK(ret, ERR_MONGO, "USERLOGOFFLINE| msg=get mongo connect fail| ret=%d| i=%s",
            ret, m_name.c_str());

        /*             构造mongo更新语句             */
        mongofield = "{\"_id\": \"" + objid + "\"}";
        mongoset = "{ \"$set\": {";

        if (0 == m_udata.status) // 认证
        {
            appendJsonObject(mongoset, "ip", m_udata.ip, true);
            appendJsonObject(mongoset, "auth", m_udata.auth, true, false);
            appendJsonObject(mongoset, "verify", m_udata.time, false, false);
        }
        else // 反认证
        {
            appendJsonObject(mongoset, "unverify", m_udata.time, true, false);
            appendJsonObject(mongoset, "unverfg", StrParse::Itoa(m_udata.status), false, false);
        }
        
        mongoset += "} }";

        ret = mongo->update(mongofield, mongoset, false, false);
        if (ret)
        {
            LOGERROR("USERLOGOFFLINE| msg=mongo update fail| ret=%d| i=%s| err=%s",
                ret, m_name.c_str(), mongo->geterr().c_str());
            ret = ERR_MONGO_UPDATE;
        }

        MongoConnPoolAdmin::Instance()->ReleaseConnect(mongo);
        IFBREAK(ret);
    }
    while (0);

    return ret;
}

// 根据m_udata.mac到redis中查找对应objectid, 若找不到再往mongodb中查找
int DealUserlog::findObjectId( string& objetid ) const
{
    int ret;
    Mongo* mongo = NULL;

    do 
    {
        bool find_id_from_mongo = false;
        string mongofind, mongofield;
        string result;

        IFBREAK_N(m_udata.mac.empty(), ERR_SYSBUG);

        /*               获取objectid               */
        Redis* redis = NULL;
        string key = "objid:" + m_udata.mac;

        ret = RedisConnPoolAdmin::Instance()->GetConnect(redis, s_redispool_name.c_str());
        if (0 == ret)
        {
            ret = redis->get(objetid, key.c_str());
            RedisConnPoolAdmin::Instance()->ReleaseConnect(redis);
            WARNLOG_IF1(ret, "FINDOBJID| msg=redis get fail| key=%s|", key.c_str());
        }
        else
        {
            LOGWARN("FINDOBJID| msg=get redis pool fail| ret=%d| poolname=%s| i=%s",
                ret, s_redispool_name.c_str(), m_name.c_str());
        }

        if (objetid.empty()) // 如果缓存中无记录, 则要向mongodb中查询
        {
            StrParse::AppendFormat(mongofind,
                "{ \"$query\": { \"mac\": \"%s\", \"devmac\": \"%s\"}, \"$orderby\": {\"uptime\": -1} }",
                m_udata.mac.c_str(), m_devmac.c_str());
            mongofield = "{ \"downtime\":1 }"; // 未考虑同一user mac在同时在多设备登录的情况
            find_id_from_mongo = true;
        }
        else // 成功找到
        {
            ret = 0;
            break;
        }


        /*             向mongodb发出命令             */
        ret = MongoConnPoolAdmin::Instance()->GetConnect(mongo, s_mongopool_name.c_str());
        ERRLOG_IF1BRK(ret, ERR_MONGO, "FINDOBJID| msg=get mongo connect fail| ret=%d| i=%s",
            ret, m_name.c_str());

        if (find_id_from_mongo)
        {
            ret = mongo->find(result, mongofind, mongofield, 1, 0);
            ERRLOG_IF1BRK(ret, ERR_MONGO_FIND, "FINDOBJID| msg=find objid fail| ret=%d| err=%s| i=%s",
                ret, mongo->geterr().c_str(), m_name.c_str());

            if (0 == ret && !result.empty())
            {
                ret = StrParse::PickOneJson(objetid, result, "_id");
                WARNLOG_IF1BRK(ret, ERR_SYSTEM, "FINDOBJID| msg=pickjson fail| ret=%d| result=%s",
                    ret, result.c_str());

                if (string::npos != result.find("downtime"))
                {
                    LOGERROR("FINDOBJID| msg=no login and logoff| devmac=%s| mac=%s| time=%s",
                        m_devmac.c_str(), m_udata.mac.c_str(), m_udata.time.c_str());
                    objetid.clear();
                    ret = ERR_MONGO_NOLOGIN;
                    break;
                }
            }
        }

        ret = objetid.empty()? ERR_MONGO_FIND: 0;
    }
    while (0);

    if (mongo)
    {
        MongoConnPoolAdmin::Instance()->ReleaseConnect(mongo);
    }
    
    return ret;
}

void DealUserlog::appendJsonObject( string& objstr, const string& name,
                                   const string& val, bool comma, bool quot/*=true*/ ) const
{
    objstr += "\"" + name + "\":";
    if (quot)
    {
        objstr += "\"" + val + "\"";
    }
    else
    {
        objstr += val;
    }

    if (comma)
    {
        objstr += ", ";
    }
}



// event线程执行,响应消息
void DealUserlog::resume( int result )
{
    m_step = 10;
    respAndExit();
}

// Event线程调用
void DealUserlog::respAndExit( void )
{
    int ret = SendRespond(m_mainreq, m_respbody);

    LOGINFO("USERLOG| msg=req %s| snd=%d| resp=%s| i=%s",
        m_udata.req_type.c_str(), ret, m_respbody.c_str(), m_name.c_str());

    if (ret <= 0)
    {
        LOGERROR("USERLOG_RESP| msg=SendRespond fail| ret=%d| i=%s", ret, m_name.c_str());

        evhttp_connection_free(evhttp_request_get_connection(m_mainreq)); // 不能发送,提前关闭
        m_mainreq = NULL;
    }

    m_finish = true;
    postDeleteMe();
}


