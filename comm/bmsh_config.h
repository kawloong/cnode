/*-------------------------------------------------------------------------
FileName     : bmsh_config.h
Description  : 灵活便捷地读取conf/ini配置
dependency   : config.h
Modification :
--------------------------------------------------------------------------
   1、Date  2016-09-06       create     hejl 
-------------------------------------------------------------------------*/

/*
usage:

    程序需要添加对[comm]:key1=val1项的读取， 使用者仅需加入3行代码即实现

    1 在bmsh_config.h的<todo>后添加获取函数宏声明，名字自定义，之后在需要
      读取处使用该函数名读取: FUNC_STR_CONF(Getkey1)

    2 在bmsh_config.cpp后面的<todo>下方定义函数实现，定义时使用文件前面部分
      实现的宏：FUNC_STR_CONF_IMP(Getkey1, "comm", "key1", "default_str")

    3 根据程序需要在要读取配置的地方调用第1步的函数名，即可获得配置值
      std::string val1=Getkey1(); // 这样调用即可获得"val1"值

*/

#ifndef __BMSH_CONFIG_H__
#define __BMSH_CONFIG_H__
#include <string>
//#include "public.h"
#include "config.h"

#define FUNC_STR_CONF(funname) std::string funname(void)
#define FUNC_INT_CONF(funname) int funname(void)
#define FUNC_ARRSTR_CONF(funname) static std::string funname(int idx)
#define DEF_CONFILENAME "connect.conf"

namespace BmshConf
{
    int Init( const char* confpath );
    void UnInit( void );

    // <todo>: append reader function declare here
    FUNC_INT_CONF(ListenPort);
    FUNC_STR_CONF(GETLOGPATH);
    FUNC_INT_CONF(ThreadNum);
    FUNC_INT_CONF(ProcessNum);
    FUNC_INT_CONF(Respawn);
    FUNC_INT_CONF(TaskThread);
    FUNC_STR_CONF(RedisPoolConfFile);
    FUNC_STR_CONF(RedisPoolName);
    FUNC_STR_CONF(LogServer);

    FUNC_STR_CONF(IcometEHost);
    FUNC_STR_CONF(IcometIHost);
    FUNC_INT_CONF(IcometAdminPort);
    FUNC_INT_CONF(IcometFrontPort);
    FUNC_INT_CONF(IcometInterval);

    FUNC_INT_CONF(NodeHostID);
    FUNC_INT_CONF(NodeIpcKEY);

    FUNC_STR_CONF(MongoPoolName);

    FUNC_INT_CONF(TESTINI);

};

#endif

