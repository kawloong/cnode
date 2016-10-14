#include <event2/buffer.h>
#include "deal_cmdreq.h"
#include "errdef.h"
#include "threaddata.h"
#include "rapidjson/json.hpp"
#include "redis/redispooladmin.h"

/*************** body format *************
{
"ID": 123,
"Type": 1,
"CommandList": 
    [
        { "CommandTarget": [mac1, mac2] },
        { "CommandTarget": [mac3, mac4] },
    ]
}
*****************************************/

using namespace rapidjson;

DealCmdReq::DealCmdReq( struct evhttp_request* req, ThreadData* tdata ): DealBase(req, tdata)
{
    m_online_count = 0;
    m_step = 0;
}

DealCmdReq::~DealCmdReq( void )
{
    std::list<RouteMsg*>::iterator it;
    for (it = m_routemsg_lst.begin(); it != m_routemsg_lst.end(); ++it)
    {
        RouteMsg* rmg = *it;
        delete rmg;
    }

    m_routemsg_lst.clear();
}

// static
int DealCmdReq::Run( struct evhttp_request* req, ThreadData* tdata )
{
    int ret = 0;
    const char* err_reason = "";
    DealCmdReq* deal = NULL;
    char* bodydata = NULL;

    do
    {
        size_t sztmp;
        evbuffer* bodybuff = evhttp_request_get_input_buffer(req);
        size_t bodylen = bodybuff? evbuffer_get_length(bodybuff): 0;
        err_reason = "body no data";
        IFBREAK_N(bodylen<=0, ERR_PARAMETER);

        // 创建请求处理实例
        deal = new DealCmdReq(req, tdata);
        bodydata = (char*)malloc(bodylen + 1);
        sztmp = evbuffer_remove(bodybuff, bodydata, bodylen);
        err_reason = "evbuffer error";
        IFBREAK_N(sztmp != bodylen, ERR_SYSTEM);

        // 让实例解析处理json内容, 得到m_routemsg_lst
        ret = deal->parseReqJson(bodydata);
        err_reason = "parse json fail";
        ERRLOG_IF1BRK(ret, ERR_PARAMETER, "CMDREQRUN| msg=parse req json fail| ret=%d", ret);

        // 放到任务队列,交由run_task()异步处理
        ret = TaskPool::Instance()->addTask(deal);
    }
    while(0);

    if (ret) // 参数不合法的响应
    {
        ret = SendRespondFail(req, err_reason, ret);
        IFDELETE(deal);
    }

    IFFREE(bodydata);

    return ret;
}

// 解析body里json进dom对象
int DealCmdReq::parseReqJson(const char* bodydata)
{
    int ret;
    const char* err_reason = "";
    IFRETURN_N(!bodydata, -1);

    Document doc;
    IFRETURN_N(doc.ParseInsitu((char*)bodydata).HasParseError(),
        doc.GetParseError());

    IFRETURN_N(!doc.IsObject(), -2);

#define ASSERT_BREAK(exp, n, errstr) if(!exp){ ret = n; err_reason = errstr; break; }
#define VALUE2STRING(str, val) { StringBuffer sb; PrettyWriter<StringBuffer> writer(sb); val.Accept(writer); str=sb.GetString(); }
    do 
    {
        ret = (doc.HasMember("Type") && doc["Type"].IsInt());
        ASSERT_BREAK(ret, -3, "Type member invalid");
        ret = (doc.HasMember("ID") && doc["ID"].IsString());
        ASSERT_BREAK(ret, -4, "ID member invalid");
        ret = (doc.HasMember("CommandList") && doc["CommandList"].IsArray());
        ASSERT_BREAK(ret, -5, "CommandList member invalid");
        m_cmdid = doc["ID"].GetString();
        m_cmdtype = doc["Type"].GetInt();

        const Value& cmdlist = doc["CommandList"];
        ASSERT_BREAK(cmdlist.Size()>0, -6, "cmdlist empty");

        Value cpObj;
        Document::AllocatorType& allocator = doc.GetAllocator();
        // 解析"CommandList"数组
        for (Value::ConstValueIterator itr = cmdlist.Begin(); itr != cmdlist.End(); ++itr)
        {
            ASSERT_BREAK(itr->IsObject(), -7, "cmdlist member isnot object");
            ASSERT_BREAK(itr->HasMember("CommandTarget"), -8, "invalid cmdtarget");
            const Value& cmdtarget = (*itr)["CommandTarget"];
            string strdev;
            RouteMsg* routemsg = new RouteMsg;

            routemsg->status = RMS_INIT;
            routemsg->cmd = m_cmdid;
            routemsg->parent = this;

            cpObj.CopyFrom(*itr, allocator); 
            cpObj.HasMember("CommandID") && cpObj.RemoveMember("CommandID");
            cpObj.AddMember("CommandID", doc["ID"], allocator);

            if (cmdtarget.IsString())
            {
                VALUE2STRING(routemsg->req, cpObj); //cpObj.Clear();
                routemsg->mac = cmdtarget.GetString();
                m_routemsg_lst.push_back(routemsg);
            }
            else if (cmdtarget.IsArray())
            {
                for (SizeType i = 0; i < cmdtarget.Size(); i++)
                {
                    cpObj["CommandTarget"].CopyFrom(cmdtarget[i], allocator);
                    VALUE2STRING(routemsg->req, cpObj); //cpObj.Clear();
                    routemsg->mac = cmdtarget[i].GetString();
                    m_routemsg_lst.push_back(routemsg);
                }
            }
            else // error format
            {
                //ASSERT_BREAK(false, -9, "invalid cmdtarget type");
            }
        }
        IFBREAK(ret);

        IFBREAK_N(m_routemsg_lst.empty(), -10);
    }
    while(0);

    return ret;
}

// virtual
int DealCmdReq::run_task( int flag )
{
    int ret;

    if (0 == m_step) // check router online or not
    {
        ret = checkOnline();
    }
    else if (2 == m_step) // check cmdresp back in redis
    {
        ret = waitRedisResult();
    }

    return 0;
}

// 检查路由器是否在线,不在线不下发命令
int DealCmdReq::checkOnline( void )
{
    int ret;
    int onlinen = 0;
    std::list<RouteMsg*>::iterator it;
    std::map<string, int> tmpmp; // 避免同一设备查多次
    Redis* rds_conn = NULL;

    do
    {
        ret = RedisConnPoolAdmin::Instance()->GetConnect(rds_conn, s_redis_poolname.c_str());
        ERRLOG_IF1BRK(ret, ERR_REDIS, "CHKONLINE| msg=get redis fail| ret=%d| redis=%s", ret, s_redis_poolname.c_str());

        for (it = m_routemsg_lst.begin(); it != m_routemsg_lst.end(); ++it)
        {
            RouteMsg* rmg = *it;

            if (tmpmp.find(rmg->mac) != tmpmp.end())
            {
                rmg->status = tmpmp[rmg->mac];
                continue;
            }

            string key = "device:" + rmg->mac;
            string val;
            ret = rds_conn->hget(val, key.c_str(), "status");

            if (0 == ret && 0 == val.compare("1"))
            {
                rmg->status = RMS_SEND;
                ++onlinen;
            }
            else
            {
                LOGWARN("CMDREQTASK| msg=route maybe off| ret=%d| mac=%s", ret, rmg->mac.c_str());
                rmg->status = RMS_OFFLINE;
            }

            tmpmp[rmg->mac] = rmg->status;
        }

        ret = 0;
    }while(0);

    if (rds_conn)
    {
        RedisConnPoolAdmin::Instance()->ReleaseConnect(rds_conn);
        rds_conn = NULL;
    }

    if (ret) // redis 不可用
    {
        m_step = -1;
        ret = SendRespondFail(m_mainreq, "redis connect fail", ret);
    }
    else
    {
        m_step = 1;
        ret = postResume();
    }

    return ret;
}

// virtual
void DealCmdReq::resume( int result )
{
    int ret;
    if (1 == m_step)
    {
        std::list<RouteMsg*>::iterator it;
        for (it = m_routemsg_lst.begin(); it != m_routemsg_lst.end(); ++it)
        {
            RouteMsg* rmg = *it;
            if (RMS_SEND == rmg->status)
            {
                ret = pushICometReq(rmg->mac, rmg->req, rmg);
                if (ret)
                {
                    rmg->status = RMS_PUSHFAIL;
                }
            }
        }

        waitPushResp();
    }
}


// icomet push callback
int DealCmdReq::RouteMsg::clireq_callback( int result, const string& body, const string& errmsg )
{
    if (0 == result && string::npos != body.find("ok")) // sucess push
    {
        this->status = RMS_RESP;
    }
    else
    {
        this->status = RMS_PUSHFAIL;
        LOGERROR("SENDCMDCB| msg=push icomet req fail| mac=%s| cmdid=%s| errmsg=%s",
            mac.c_str(), cmd.c_str(), errmsg.c_str());
    }

    parent->waitPushResp();
    return 0;
}

// 等待所有http-push请求已响应
bool DealCmdReq::waitPushResp( void )
{
    if (1 == m_step)
    {
        int rspcount = 0;
        std::list<RouteMsg*>::iterator it;
        for (it = m_routemsg_lst.begin(); it != m_routemsg_lst.end(); ++it)
        {
            RouteMsg* rmg = *it;
            if (RMS_SEND == rmg->status)
            {
                return false; // continue wait
            }
            else if (RMS_RESP == rmg->status)
            {
                rspcount++;
            }
        }

        if (1 == m_cmdtype) // 无需等待回复类命令
        {
            SendRespond(m_mainreq, "{\"status\": \"SUCCESS\"");
            postDeleteMe();
        }
        else
        {
            m_step = 2;
            m_online_count = rspcount;
            m_waitcmd_sec = WAIT_CMDRESP_TIME_SEC; // 最大等待时间(秒)
            if (TaskPool::Instance()->addTask(this, 500)) // if err happen
            {
                m_step = 10;
                SendRespondFail(m_mainreq, "addtask fail", ERR_SYSTEM);
                postDeleteMe();
            }
        }

    }
    else
    {
        LOGERROR("WAITPUSHRSP| msg=flow error| step=%d| msg_lsize=%d", m_step, (int)m_routemsg_lst.size());
    }

    return true;
}

// 等待redis接收/cloud/command/response的结果
int DealCmdReq::waitRedisResult( void )
{
    int ret;
    unsigned result_len = 0;
    Redis* rds_conn = NULL;
    string key;
    list<string> rds_result;

    do
    {
        ret = RedisConnPoolAdmin::Instance()->GetConnect(rds_conn, s_redis_poolname.c_str());
        ERRLOG_IF1BRK(ret, ERR_REDIS, "CMDREQ| msg=get conn fail| ret=%d| task=%s", ret, m_cmdid.c_str());
        key = "task:" + m_cmdid;
        ret = rds_conn->llen(result_len, key.c_str());

        if (result_len >= m_online_count) // done
        {
        }
        else if (time(NULL) - m_begtime > m_waitcmd_sec) // 超时
        {
            LOGWARN("CMDREQ| msg=wait timeout| wait=%us| task=%s", m_waitcmd_sec, m_cmdid.c_str());
        }
        else // 继续等待
        {
            if (TaskPool::Instance()->addTask(this, 1000))
            { // if err happen
                ret = ERR_SYSTEM;
            }

            break;
        }

        ret = rds_conn->lrange(rds_result, key.c_str(), 0, -1);
        ERRLOG_IF1BRK(ret, ERR_REDIS, "CMDREQ| msg=lrange fail| ret=%d| task=%s", ret, m_cmdid.c_str());
        rds_conn->del(key.c_str());

        // 将redis结果放于json-array中
        list<string>::const_iterator itr = rds_result.begin();
        m_respbody = "[ ";
        for (bool first = true; itr != rds_result.end(); ++itr)
        {
            if (!first)
            {
                m_respbody += ", ";
            }

            m_respbody += (*itr);
            first = false;
        }
        m_respbody += " ]";
        SendRespond(m_mainreq, m_respbody);
        postDeleteMe();
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
        m_step = 10;
        SendRespondFail(m_mainreq, "addtask fail", ret);
        postDeleteMe();
    }

    return ret;
}
