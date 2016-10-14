#include <stdio.h>
#include <stdlib.h>
#include "bmsh_config.h"

namespace BmshConf
{
    Config s_config;


int Init( const char* conffile )
{
    int ret = 0;

    if (NULL == conffile || 0 == conffile[0])
    {
        ret = s_config.load(DEF_CONFILENAME);
    }
    else
    {
        ret = s_config.load(conffile);
    }

    if ( 0 != ret )
    {
        fprintf(stderr, "CONFINIT| msg=conf(%s) init fail(%d)\n", conffile, ret);
    }

    return ret;
}

void UnInit( void )
{
    s_config.unload();
}

#define FUNC_STR_CONF_IMP(funname, szSec, szKey, defval)       \
std::string funname(void)                                      \
{                                                              \
    std::string outval;                                        \
    int result = s_config.read(szSec, szKey, outval);          \
    if (result) return defval;                                 \
    return outval;                                             \
}

#define FUNC_INT_CONF_IMP(funname, szSec, szKey, defval)      \
int funname(void)                                             \
{                                                             \
    int outi;                                                 \
    std::string outval;                                       \
    int result = s_config.read(szSec, szKey, outval);         \
    if (result) return defval;                                \
    outi = atoi(outval.c_str());                              \
    return outi;                                              \
}

#define FUNC_ARRSTR_CONF_IMP(funname, szSec, szKey, defval)   \
static std::string funname(int idx, bool warnlog=false)       \
{                                                             \
    int result;                                               \
    char outval;                                              \
    char keystr[128];                                         \
    snprintf(keystr, sizeof(keystr), "%s%d", szKey, idx);     \
    result = s_config.read(szSec, keystr, outval);            \
    if (result) return defval;                                \
    return outval;                                            \
}

// 快捷配置读取定义
// <todo>: append reader function implement here
FUNC_INT_CONF_IMP(ListenPort, "common", "port", 6001)
FUNC_STR_CONF_IMP(GETLOGPATH, "common", "logpath", ".")
FUNC_STR_CONF_IMP(RedisPoolConfFile, "common", "redisconffile", "/etc/bmsh_redis_connpoll_conf.xml")
FUNC_STR_CONF_IMP(RedisPoolName, "common", "redispool", "qlyredis")
FUNC_INT_CONF_IMP(ThreadNum, "common", "event_thread", 1)
FUNC_INT_CONF_IMP(ProcessNum, "common", "process", 1)
FUNC_INT_CONF_IMP(Respawn, "common", "respawn", 0)
FUNC_INT_CONF_IMP(TaskThread, "common", "task_thread", 2)
FUNC_STR_CONF_IMP(LogServer, "common", "log_server", "unknow")

FUNC_STR_CONF_IMP(IcometEHost, "icomet", "host_e", "unknow")
FUNC_STR_CONF_IMP(IcometIHost, "icomet", "host", "127.0.0.1")
FUNC_INT_CONF_IMP(IcometAdminPort, "icomet", "admin_port", 8100)
FUNC_INT_CONF_IMP(IcometFrontPort, "icomet", "front_port", 8000)
FUNC_INT_CONF_IMP(IcometInterval, "icomet", "interval", 30)
FUNC_INT_CONF_IMP(TESTINI, "comm3", "ss", 234)

}


