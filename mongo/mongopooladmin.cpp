/******************************************************************* 
 *  summery:     管理Mongo连接池集合实现
 *  author:      hejl
 *  date:        2016-04-11
 *  description: 以poolname为标识管理多个MongoConnPool对象
 ******************************************************************/ 
#include <fstream>
#include <sstream>
#include "mongopooladmin.h"
#include "mongopool.h"
#include "comm/public.h"
#include "rapidjson/json.hpp"


#define BREAK_CTRL_BEGIN do{
#define BREAK_CTRL_END  }while(0);
#define BREAKIF1(exp) if(exp){break;}
#define BREAKNIF1(exp, n) if(exp){ret=n; break;}

MongoConnPoolAdmin* MongoConnPoolAdmin::Instance(void)
{
	static MongoConnPoolAdmin sig_obj;
	return &sig_obj;
}

MongoConnPoolAdmin::MongoConnPoolAdmin(void)
{
}

MongoConnPoolAdmin::~MongoConnPoolAdmin(void)
{
	DestroyPool(NULL);
}

/*********************************
 配置文件格式:
{
"cnode_pool":
    {   
    "mongo_uri": "mongodb://192.168.11.150:27017/?connectTimeoutMS=3000" ,
    "db_name": "cloud",
    "coll_name": "test"
    "poolconnmax": 3,
    "initconn": 1
    },
"test_pool":
    {
    ...
    }
}
********************************/
int MongoConnPoolAdmin::LoadPoolFromFile( const char* conffile )
{
    int ret = -1;
    int mgocount = 0;

    Document doc;

    do
    {
        IFBREAK_N(NULL == conffile || 0 == conffile[0], ERR_PARAM_INPUT);
        ifstream ifs(conffile);
        stringstream ss;
        ss << ifs.rdbuf();
        string strjson(ss.str());

        ret = doc.ParseInsitu((char*)strjson.c_str()).GetParseError();
        ERRLOG_IF1BRK(ret, ERR_LOADCONF, "MONGOPOOLCONF| msg=load confiure file fail| ret=%d| file=%s",
            ret, conffile);
        IFBREAK_N(!doc.IsObject(), ERR_PARAM_INPUT);

        Value::MemberIterator cit = doc.MemberBegin();
        for (; cit != doc.MemberEnd(); ++cit)
        {
            const char* poolname = cit->name.GetString();
            const Value& val = cit->value;
            string strtemp;

            if (!val.IsObject()) continue;
            mongo_pool_conf_t* conf = new mongo_pool_conf_t;
            conf->poolname = poolname;
            if (Rjson::GetStr(conf->mongo_uri, "mongo_uri", &val))
            {
                delete conf;
                continue;
            }

            Rjson::GetStr(conf->db_name, "db_name", &val);
            Rjson::GetStr(conf->coll_name, "coll_name", &val);

            Rjson::GetInt(conf->processMaxConnNum, "poolconnmax", &val);
            Rjson::GetInt(conf->initConnNum, "initconn", &val);
            m_all_conf[conf->poolname] = conf;
            ++mgocount;
        }

        ret = 0;
    }
    while (false);

    if (ret)
    {
        fprintf(stderr, "parse %s ret=%d\n", conffile?conffile:"", ret);
    }

    fprintf(stdout, "mongo_conf_init| result=%d| count=%d\n", ret, mgocount);
    //ret = mgocount>0? 0: -1;
    return ret;
}

int MongoConnPoolAdmin::RegisterPool(void)
{
    string tracelog;
    int okcnt = 0;
    // 此处可控制默认是延迟加载还是启动全加载, 目前是后者
    for (map<string, mongo_pool_conf_t*>::iterator it = m_all_conf.begin();
        it != m_all_conf.end(); ++it)
    {
        mongo_pool_conf_t* conf = it->second;
        tracelog.append(conf->poolname);

        if (0 == conf->processMaxConnNum)
        {
            tracelog.append("=skip ");
            continue;
        }

        if (0 == conf->initConnNum)
        {
            tracelog.append("=delay ");
            continue;
        }

        int reg_ret = RegisterPool(conf);

        if (0 == reg_ret)
        {
            tracelog.append("=ok ");
            ++okcnt;
        }
        else
        {
            char buff[32];
            snprintf(buff, sizeof(buff), "=fail%d ", reg_ret);
            tracelog.append(buff);
            LOGWARN("REGISTER_MONGOPOOL| msg=register Mongo pool fail| "
                "poolname=%s| ret=%d", conf->poolname.c_str(), reg_ret);
        }
    }

    LOGINFO("REGISTER_MONGOPOOL| msg=%d(%s)", okcnt, tracelog.c_str());
    fprintf(stdout, "load_mongopool result: %s\n", tracelog.c_str());
    return 0;
}

int MongoConnPoolAdmin::RegisterPool(mongo_pool_conf_t* conf)
{
	int ret = 0;
	
	BREAK_CTRL_BEGIN
	BREAKNIF1(NULL == conf, ERR_PARAM_INPUT);
    {
        RWLOCK_READ(m_lock); // 读操作锁
	    BREAKNIF1(m_conn_pools.find(conf->poolname) != m_conn_pools.end(), ERR_DUP_INIT);
    }

	
//     if (delay)
//     {
//         m_all_conf[conf->poolname] = conf;
//     }
//     else
    {
        MongoConnPool* pool = new MongoConnPool;
        ret = pool->init(conf);

        if (0 == ret)
        {
            RWLOCK_WRITE(m_lock); // 写操作锁
            m_conn_pools[conf->poolname] = pool;
        }
        else
        {
            delete pool;
        }
    }
	
	BREAK_CTRL_END
	return ret;
}

int MongoConnPoolAdmin::DestroyPool(const char* poolname)
{
    RWLOCK_WRITE(m_lock); // 写操作锁

	if (poolname)
	{
		map<string, MongoConnPool*>::iterator it = m_conn_pools.find(poolname);
		if (m_conn_pools.end() != it)
		{
			delete it->second;
			m_conn_pools.erase(it);
		}

        map<string, mongo_pool_conf_t*>::iterator it2 = m_all_conf.find(poolname);
        if (it2 != m_all_conf.end())
        {
            mongo_pool_conf_t* conf = it2->second;
            m_all_conf.erase(it2);
            delete conf;
        }
	}
	else
	{
		map<string, MongoConnPool*>::iterator it = m_conn_pools.begin();
		for (; it != m_conn_pools.end(); ++it)
		{
			delete it->second;
		}

		map<string, mongo_pool_conf_t*>::iterator it2 = m_all_conf.begin();
        for (; it2 != m_all_conf.end(); ++it2)
        {
            mongo_pool_conf_t* conf = it2->second;
            delete conf;
        }

        m_conn_pools.clear();
        m_all_conf.clear();
	}
	
	return 0;
}
	
int MongoConnPoolAdmin::GetConnect(Mongo*& pmongo, const char* poolname)
{
    int ret;
    MongoConnPool* pool = NULL;
    map<string, MongoConnPool*>::iterator it;

    {
        RWLOCK_READ(m_lock); // 读操作锁
        it = m_conn_pools.find(poolname);
        if (m_conn_pools.end() != it)
        {
            pool = it->second;
        }
    }

    if (pool) // 与DestroyPool()并发时有风险; 后考虑
    {
		ret = pool->getConnect(pmongo);
	}
	else
	{
		// log unexcept poolname or unregister poolname
		ret = ERR_INVALID_NAME;
        map<string, mongo_pool_conf_t*>::iterator it = m_all_conf.find(poolname);
        if (it != m_all_conf.end())
        {
            mongo_pool_conf_t* conf = it->second;
            ret = RegisterPool(conf);
            if (0 == ret)
            {
                ret = GetConnect(pmongo, poolname);
            }
        }
	}
	
	return ret;
}

int MongoConnPoolAdmin::ReleaseConnect(Mongo* pmongo)
{
	if (pmongo)
	{
		MongoConnPool* pool = (MongoConnPool*)pmongo->parent;
        if (pool)
        {
            pool->relConnect(pmongo);
        }
        else
        {
            LOGERROR("RELEASE_CONNECT| msg=Mongo object miss parent| ptr=%p", pmongo);
            delete pmongo;
        }
	}
	
	return 0;
}

void MongoConnPoolAdmin::showPoolStatus( const char* poolname )
{
    string logmsg;

    showPoolStatus(logmsg, poolname);
    if (!logmsg.empty())
    {
        LOGDEBUG("%s", logmsg.c_str());
    }
}

void MongoConnPoolAdmin::showPoolStatus( string& strbak, const char* poolname )
{
    MongoConnPool* pool = NULL;
    string keyname;
    map<string, MongoConnPool*>::iterator it;
    RWLOCK_READ(m_lock); // 读操作锁

    if (poolname)
    {
        keyname = poolname;
    }

    if (!keyname.empty())
    {
        it = m_conn_pools.find(keyname);
        if (m_conn_pools.end() != it)
        {
            pool = it->second;
            pool->trace_stat(strbak, true);
            strbak.append("<hr>");
        }
        else if (m_all_conf.find(keyname) != m_all_conf.end())
        {
            strbak.append("poolname=%s not init| ", poolname);
            strbak.append("<hr>");
        }
    }
    else
    {
        for (it = m_conn_pools.begin(); it != m_conn_pools.end(); ++it)
        {
            pool = it->second;
            pool->trace_stat(strbak, false);
        }
    }
}

