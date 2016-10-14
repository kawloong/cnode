/******************************************************************* 
 *  summery: ����sql���ӳؼ���
 *  author:  hejl
 *  date:    2016-04-10
 *  description: ��redisnameΪ��ʶ������RedisConnPool����
 ******************************************************************/ 

#ifndef _REDISPOOLADMIN_H_
#define _REDISPOOLADMIN_H_
#include <map>
#include "comm/lock.h"
#include "redis.h"

using namespace std;
class RedisConnPool;


/******************************************************************
�ļ�����  : comm/lock.h comm/public.h tinyxml/xx.h  hiredis/hiredis.h
so����    : libhiredis.so

��������ϵ: RedisConnPoolAdmin  --> RedisConnPool   -->   Redis     -->  redisContext
            �������ӳع���(����)    �������ӳ�            ������         Redis api�ӿ�
            ��������ض���          �����ж��������      ��װRedis����  hiredisʵ��
ʹ��ʾ��:
        Redis* rds;
        const char redisfileconf[] = "/etc/bmsh_redis_connpoll_conf.xml"; // ����xml����
        // program begin
        assert( RedisConnPoolAdmin::Instance()->LoadPoolFromFile(redisfileconf) );

        // ... ҵ������ ...
        if (0 == RedisConnPoolAdmin::Instance()->GetConnect(rds, "redis1"))
        {
            // ����Redis��ط���
            rds->set("test_key1", "value1");
            // ...

            RedisConnPoolAdmin::Instance()->ReleaseConnect(rds);
        }
        // ... ҵ������ ...

        // program end
        RedisConnPoolAdmin::Instance()->DestroyPool(NULL);
*******************************************************************/

class RedisConnPoolAdmin
{
public:
	static RedisConnPoolAdmin* Instance(void); // sigleton
	
public:
	/***************************************
	@summery: �������ļ���������Ϣ, ����m_all_conf��
    @param1:  file ����xml�ж��������, Ĭ����"bmsh_redis_connpoll_conf.xml"
	@return: 0 �ɹ�ִ��;
	**************************************/
	int LoadPoolFromFile(const char* file);

	/***************************************
	@summery: ע��һ�����ӳ�,ÿ�����̳�ͨ��conf->redisname��ʶ
    @param1:  conf ����xml�ж��������
    @param2:  delay �Ƿ���ʱ������ // �ݲ�ʵ��
	@return: 0 �ɹ�ִ��; 1 ��������; 2 redisname����; 3 ��ע���; 4 ������ʧ��
	**************************************/
    int RegisterPool(redis_pool_conf_t* conf);
    int RegisterPool(void);
	
	/***************************************
	@summery: ���ݳ���������һ�����ӳ�
	@param1:  redisname Ҫ���ٵ����ӳ���
	@return: 0 �ɹ�ִ��; ��������; 
	**************************************/	
	int DestroyPool(const char* redisname);
	
	/***************************************
	@summery: ���ݳ�����,��ó���һ��Redis����
	@param1:  predis [out] ���շ��ص�Redis����ָ��
	@param2:  redisname [in] Ҫ���ٵ����ӳ���
	@return:  0 �ɹ�ִ��; 
	@remark:  GetConnect��ReleaseConnectҪ��ɶ�ʹ��
	**************************************/	
	int GetConnect(Redis*& predis, const char* redisname);
	
	/***************************************
	@summery: �ͷ�һ����GetConnect���յ������Ӷ���
	@param1:  predis ���Ӷ���, ��GetConnect���
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  GetConnect��ReleaseConnectҪ��ɶ�ʹ��
	**************************************/	
	int ReleaseConnect(Redis* predis);
	
public:
	// ��ӡ���ӳصĵ�ǰ״̬
	void showPoolStatus(const char* redisname);
    void showPoolStatus(string& strbak, const char* redisname);
	
protected: // ��ʵ������
	RedisConnPoolAdmin(void);
	~RedisConnPoolAdmin(void);

private:
    ThreadLock m_lock;
	map<string, RedisConnPool*> m_conn_pools;
    map<string, redis_pool_conf_t*> m_all_conf; // ������ʱ��ʼ���ĳ�
	
};

#endif //
