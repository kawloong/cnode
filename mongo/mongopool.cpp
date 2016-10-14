#include <mongoc.h>
#include "comm/public.h"
#include "mongopool.h"

#define BREAK_CTRL_BEGIN do{
#define BREAK_CTRL_END  }while(0);
#define BREAKIF1(exp) if(exp){break;}
#define BREAKNIF1(exp, n) if(exp){ret=n; break;}

#define UNINIT_RETURN_N(n) if (!m_inited) return n;
#define UNINIT_RETURN if (!m_inited) return ;

MongoConnPool::MongoConnPool(void): m_conf(NULL), m_inited(false), m_pool(NULL), m_uri(NULL)
{
}

int MongoConnPool::init(mongo_pool_conf_t* conf)
{
	int ret = 0;

	do
	{
		if (NULL == conf || conf->mongo_uri.empty() ||
            conf->initConnNum > conf->processMaxConnNum || conf->initConnNum < 0)
		{
			ret = ERR_PARAM_INPUT;
			break;
		}
		
		m_conf = conf;
        m_conn_max_process = conf->processMaxConnNum;
		
		m_conn_count_process = 0;
		m_totalconn_count = 0;
        m_peek_conn_count = 0;
		m_freeconn_list.clear();

        mongoc_init();
        m_uri = mongoc_uri_new(conf->mongo_uri.c_str());
        m_pool = mongoc_client_pool_new(m_uri);

        mongoc_client_pool_min_size(m_pool, conf->initConnNum);
        mongoc_client_pool_max_size(m_pool, conf->processMaxConnNum);
        m_inited = true;
		
        if (conf->initConnNum > 0)
        {
            Mongo* mog = NULL;
            int ret_get = getConnect(mog, false);

            ERRLOG_IF1BRK(ret_get, ERR_INIT_FAIL,
                "MONGOPOOLINIT| msg=init connect fail| poolname=%s| ret=%d",
                conf->poolname.c_str(), ret_get);

            {
                string errmsg;
                int ret_ping = mog->ping(&errmsg);
                ERRLOG_IF1BRK(ret_ping, ERR_INIT_FAIL, "MONGOPOOLINIT| msg=ping fail| err=%s| poolname=%s",
                    errmsg.c_str(), conf->poolname.c_str());
                relConnect(mog);
            }
        }
		
	}
	while(0);
	
	return ret;
}

void MongoConnPool::uninit(void)
{
	if (m_inited)
	{
		m_conf = NULL;

        RWLOCK_WRITE(m_lock); // 写操作锁
		list<Mongo*>::iterator it = m_freeconn_list.begin();
		while (it != m_freeconn_list.end())
		{
			Mongo* p = *it;
			destroy_connect(p);
			++it;
		}
		
		m_freeconn_list.clear();
        mongoc_client_pool_destroy(m_pool);
        mongoc_uri_destroy(m_uri);
        mongoc_cleanup();

        m_pool = NULL;
        m_uri = NULL;
		m_inited = false;
	}
}

MongoConnPool::~MongoConnPool(void)
{
	if (m_inited)
	{
		uninit();
	}
}


// remark: 返回的值要求调用者释放
Mongo* MongoConnPool::create_connect(bool onlyPool)
{
    Mongo* pmg = NULL;
    mongoc_collection_t* collection = NULL;
    mongoc_client_t* mongocli = NULL;
    int conn_count_process;

    {
        RWLOCK_READ(m_lock); // 读操作锁
        conn_count_process = m_conn_count_process;
    }

    if (conn_count_process < m_conf->processMaxConnNum)
    {
        mongocli = mongoc_client_pool_pop(m_pool);
    }

    if (mongocli)
    {
        collection = mongoc_client_get_collection(mongocli, m_conf->db_name.c_str(), m_conf->coll_name.c_str());
        if (collection)
        {
            pmg = new Mongo;
            pmg->m_client = mongocli;
            pmg->m_coll = collection;
            pmg->inpool = true;
            pmg->parent = this;

            RWLOCK_WRITE(m_lock); // 写操作锁
            ++m_conn_count_process;
            ++m_totalconn_count;
            if (m_totalconn_count > m_peek_conn_count)
            {
                m_peek_conn_count = m_totalconn_count;
            }
        }
        else
        {
            mongoc_client_pool_push(m_pool, mongocli);
        }
    }

    if (NULL == pmg && !onlyPool) // 池中空,创建短连接
    {
        mongocli = mongoc_client_new(m_conf->mongo_uri.c_str());
        if (mongocli)
        {
            collection = mongoc_client_get_collection(mongocli, m_conf->db_name.c_str(), m_conf->coll_name.c_str());

            if (collection)
            {
                pmg = new Mongo;
                pmg->m_client = mongocli;
                pmg->m_coll = collection;
                pmg->inpool = false;
                pmg->parent = this;

                RWLOCK_WRITE(m_lock); // 写操作锁
                ++m_totalconn_count;
                if (m_totalconn_count > m_peek_conn_count)
                {
                    m_peek_conn_count = m_totalconn_count;
                }
            }
            else
            {
                mongoc_client_destroy(mongocli);
            }
        }
    }
    
    return pmg;
}

// return: 0 成功; 1 创建连接失败; 2 池中无空闲连接可用; -1未初始化
int MongoConnPool::getConnect(Mongo*& mog, bool only_pool)
{
	int result;
    bool need_create = true;
	Mongo* p = NULL;
	
	UNINIT_RETURN_N(-1);

    {
        RWLOCK_READ(m_lock); // 读操作锁
        if (!m_freeconn_list.empty())
        {
            p = m_freeconn_list.front();
            m_freeconn_list.pop_front();
            need_create = false;
        }
    }

    result = p? 0: 2;
    if (need_create)
    {
        p = create_connect(only_pool); // 池是否有空余配额
        result = p? 0: 1;
    }
	
    mog = p;
	return result;
}

// param: check_connect 如果此察觉连接可能断开时,传入true,回收检查
void MongoConnPool::relConnect(Mongo *mog)  // 释放连接
{
	UNINIT_RETURN;
	if (mog && mog->inpool) // 长连接
	{
        RWLOCK_WRITE(m_lock); // 写操作锁
        m_freeconn_list.push_back(mog);
	}
	else // 短连接
	{
		destroy_connect(mog);
        RWLOCK_WRITE(m_lock); // 写操作锁
        --m_totalconn_count;
	}
}

void MongoConnPool::destroy_connect(Mongo* p) const
{
	if (p)
	{
        mongoc_collection_destroy(p->m_coll);
        mongoc_client_destroy(p->m_client);
		delete p;
	}
}


// 是否还可以创建新连接加入池
inline bool MongoConnPool::can_create_poolconn(void)
{
	return (m_conn_count_process < m_conn_max_process);
}

// 查看连接池运行状态信息
void MongoConnPool::trace_stat( string& msg, bool includeFreeConn )
{
    char strbuf[512];
    int now = (int)time(NULL);

    // format: dbconn=当前进程总连接数/当前DB全局连接数/DB全局允许最大连接数;
    //         poolconn=池内空闲连接数/池内所有长连接数/池允许最大连接数
    //         dbtime_d=最后一次查全局连接数距今的秒数
    snprintf(strbuf, sizeof(strbuf),
        "poolname=%s| totalconn=%d| poolconn=%d/%d/%d (%d)| "
        "mogo_uri=%s",
        m_conf->poolname.c_str(), 
        m_totalconn_count, (int)m_freeconn_list.size(), 
        m_conn_count_process, m_conn_max_process, m_peek_conn_count,
        m_conf->mongo_uri.c_str()); 

    msg.append(strbuf);

    if (includeFreeConn)
    {
        list<Mongo*>::iterator it = m_freeconn_list.begin();
        while (it != m_freeconn_list.end())
        {
            Mongo* p = *it;
            snprintf(strbuf, sizeof(strbuf),
                "\nmongo_state=%s| ativetime=%ds",
                p->geterr().c_str(), now-p->begtime);
            msg.append(strbuf);
            ++it;
        }
    }
}
