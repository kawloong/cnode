/******************************************************************* 
 *  summery: ����mongodb���ӳؼ���
 *  author:  hejl
 *  date:    2016-10-10
 *  description: ��poolnameΪ��ʶ������MongoConnPool����
 *  remark: libmongoc api help at http://mongoc.org/libmongoc/1.4.0/
 ******************************************************************/ 

#ifndef _MONGOPOOLADMIN_H_
#define _MONGOPOOLADMIN_H_
#include <map>
#include "comm/lock.h"
#include "mongopool.h"

using namespace std;
class MongoConnPool;


/******************************************************************
�ļ�����  : comm/lock.h comm/public.h  rapidjson/ *.h
so����    : libmongoc-1.0.so (��valgrind���Դ����ڴ�й¶)

��������ϵ: MongoConnPoolAdmin  --> MongoConnPool   -->   Mongo     -->  mongoContext
            �������ӳع���(����)    �������ӳ�            ������         Mongo api�ӿ�
            ��������ض���          �����ж��������      ��װMongo����  himongoʵ��
ʹ��ʾ��:
        Mongo* mgo;
        const char mongofileconf[] = "/etc/bmsh_mongo_connpoll_conf.json"; // ����json
        // program begin
        assert( MongoConnPoolAdmin::Instance()->LoadPoolFromFile(mongofileconf) );

        // ... ҵ������ ...
        if (0 == MongoConnPoolAdmin::Instance()->GetConnect(mgo, "mongo1"))
        {
            // ����Mongo��ط���
            mgo->ping();
            // ...

            MongoConnPoolAdmin::Instance()->ReleaseConnect(mgo);
        }
        // ... ҵ������ ...

        // program end
        MongoConnPoolAdmin::Instance()->DestroyPool(NULL);
*******************************************************************/

class MongoConnPoolAdmin
{
public:
	static MongoConnPoolAdmin* Instance(void); // sigleton
	
public:
	/***************************************
	@summery: �������ļ���������Ϣ, ����m_all_conf��
    @param1:  file ����json�ж��������, Ĭ����"bmsh_mongo_connpoll_conf.json"
	@return: 0 �ɹ�ִ��;
	**************************************/
	int LoadPoolFromFile(const char* conffile);

	/***************************************
	@summery: ע��һ�����ӳ�,ÿ�����̳�ͨ��conf->poolname��ʶ
    @param1:  conf ����xml�ж��������
    @param2:  delay �Ƿ���ʱ������ // �ݲ�ʵ��
	@return: 0 �ɹ�ִ��; 1 ��������; 2 poolname����; 3 ��ע���; 4 ������ʧ��
	**************************************/
    int RegisterPool(mongo_pool_conf_t* conf);
    int RegisterPool(void);
	
	/***************************************
	@summery: ���ݳ���������һ�����ӳ�
	@param1:  poolname Ҫ���ٵ����ӳ���
	@return: 0 �ɹ�ִ��; ��������; 
	**************************************/	
	int DestroyPool(const char* poolname);
	
	/***************************************
	@summery: ���ݳ�����,��ó���һ��Mongo����
	@param1:  pmongo [out] ���շ��ص�Mongo����ָ��
	@param2:  poolname [in] Ҫ���ٵ����ӳ���
	@return:  0 �ɹ�ִ��; 
	@remark:  GetConnect��ReleaseConnectҪ��ɶ�ʹ��
	**************************************/	
	int GetConnect(Mongo*& pmongo, const char* poolname);
	
	/***************************************
	@summery: �ͷ�һ����GetConnect���յ������Ӷ���
	@param1:  pmongo ���Ӷ���, ��GetConnect���
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  GetConnect��ReleaseConnectҪ��ɶ�ʹ��
	**************************************/	
	int ReleaseConnect(Mongo* pmongo);
	
public:
	// ��ӡ���ӳصĵ�ǰ״̬
	void showPoolStatus(const char* poolname);
    void showPoolStatus(string& strbak, const char* poolname);
	
protected: // ��ʵ������
	MongoConnPoolAdmin(void);
	~MongoConnPoolAdmin(void);

private:
    RWLock m_lock;
	map<string, MongoConnPool*> m_conn_pools;
    map<string, mongo_pool_conf_t*> m_all_conf; // ������ʱ��ʼ���ĳ�
	
};

#endif //
