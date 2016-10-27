/*-------------------------------------------------------------------------
FileName     : bmsh_config.h
Description  : ����ݵض�ȡconf/ini����
dependency   : config.h
Modification :
--------------------------------------------------------------------------
   1��Date  2016-09-06       create     hejl 
-------------------------------------------------------------------------*/

/*
usage:

    ������Ҫ��Ӷ�[comm]:key1=val1��Ķ�ȡ�� ʹ���߽������3�д��뼴ʵ��

    1 ��bmsh_config.h��<todo>����ӻ�ȡ�����������������Զ��壬֮������Ҫ
      ��ȡ��ʹ�øú�������ȡ: FUNC_STR_CONF(Getkey1)

    2 ��bmsh_config.cpp�����<todo>�·����庯��ʵ�֣�����ʱʹ���ļ�ǰ�沿��
      ʵ�ֵĺ꣺FUNC_STR_CONF_IMP(Getkey1, "comm", "key1", "default_str")

    3 ���ݳ�����Ҫ��Ҫ��ȡ���õĵط����õ�1���ĺ����������ɻ������ֵ
      std::string val1=Getkey1(); // �������ü��ɻ��"val1"ֵ

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

