#ifndef _MONGOPOOL_H_
#define _MONGOPOOL_H_
#include <list>
#include "mongo.h"
#include "comm/lock.h"

using std::list;

struct mongo_pool_conf_t
{
    string poolname;
    string mongo_uri; // ��ʱ�����ɰ�����uri��
    string db_name;
    string coll_name; //collection

    int processMaxConnNum; // ���������
    int initConnNum; // ���̳�ʼ����ʱ�����ĳ�������, ȡֵ[0-processMaxConnNum]

    mongo_pool_conf_t(): processMaxConnNum(0), initConnNum(0) {}
};

class MongoConnPool
{
public:
	/***************************************
	@summery: ��ȡ����һ������, ͨ��Mongo�����������
	@param1:  Mongo [out] �����ߴ�����յ�ָ��
	@param2:  only_pool [in] �Ƿ���޷��س��г�����; ��trueʱ,�޳�����ʱ���ض�����
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  getConnect��relConnect����ƥ��ʹ��,�ⲿ��Ҫ����delete
	**************************************/	
	int getConnect(Mongo*& mog, bool only_pool = false);
	
	/***************************************
	@summery: �ͷŻ�һ������
	@param1:  mog [in] ��getConnect��õ�ָ��
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  getConnect��relConnect����ƥ��ʹ��,�ⲿ��Ҫ����delete
	**************************************/	
	void relConnect(Mongo* mog) ; // �ͷ�����

public:
	MongoConnPool(void);
	~MongoConnPool(void);
	
	// ��ʼ���ض���
	int init(mongo_pool_conf_t* conf);
	// ����ʼ���ض���
	void uninit(void);

    // �鿴��������״̬��Ϣ
    void trace_stat(string& msg, bool includeFreeConn);

private:
	inline bool can_create_poolconn(void); // �ж��Ƿ����������
	int init_pool(unsigned int count);
	
private: //const method
	Mongo* create_connect(bool onlypool);         // ����һ������
	void destroy_connect(Mongo* p) const;      // ����һ������
	
private:
	mongo_pool_conf_t* m_conf;
	int m_conn_count_process;    // �����ڵ��ѿ��ĳ�������
	int m_conn_max_process;      // �����ڵĳ����������������
	//int m_freeconn_count;      // ���������ӳ��еĿ���������(����m_freeconn_list.size()�õ�)
	int m_totalconn_count;       // �����ڵ�ǰ�ܻ������(���������Ӻͳ�����)
    int m_peek_conn_count;       // ��������������ֵ(ĳʱ��)

	bool m_inited;               // �Ƿ��ѳ�ʼ��
	RWLock m_lock;               //�߳���
	list<Mongo*> m_freeconn_list; //��������
    mongoc_client_pool_t* m_pool;
    mongoc_uri_t* m_uri;
};

#endif //