#ifndef _REDISPOOL_H_
#define _REDISPOOL_H_
#include <list>
#include "redis.h"
#include "comm/lock.h"

using std::list;

class RedisConnPool
{
public:
	/***************************************
	@summery: ��ȡ����һ������, ͨ��Redis�����������
	@param1:  Redis [out] �����ߴ�����յ�ָ��
	@param2:  only_pool [in] �Ƿ���޷��س��г�����; ��trueʱ,�޳�����ʱ���ض�����
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  getConnect��relConnect����ƥ��ʹ��,�ⲿ��Ҫ����delete
	**************************************/	
	int getConnect(Redis*& rds, bool only_pool = false);
	
	/***************************************
	@summery: �ͷŻ�һ������
	@param1:  rds [in] ��getConnect��õ�ָ��
	@return:  0 �ɹ�ִ��; ��������; 
	@remark:  getConnect��relConnect����ƥ��ʹ��,�ⲿ��Ҫ����delete
	**************************************/	
	void relConnect(Redis* rds, bool check_connect) ; // �ͷ�����

public:
	RedisConnPool(void);
	~RedisConnPool(void);
	
	// ��ʼ���ض���
	int init(redis_pool_conf_t* conf);
	// ����ʼ���ض���
	void uninit(void);

    // �鿴��������״̬��Ϣ
    void trace_stat(string& msg, bool includeFreeConn);

private:
	inline bool can_create_poolconn(void); // �ж��Ƿ����������
	int init_pool(unsigned int count);
	
private: //const method
	Redis* create_connect(void) const;         // ����һ������
	void destroy_connect(Redis* p) const;      // ����һ������
	
private:
	redis_pool_conf_t* m_conf;
	int m_conn_count_process;    // �����ڵ��ѿ��ĳ�������
	int m_conn_max_process;      // �����ڵĳ����������������
	//int m_freeconn_count;      // ���������ӳ��еĿ���������(����m_freeconn_list.size()�õ�)
	int m_totalconn_count;       // �����ڵ�ǰ�ܻ������(���������Ӻͳ�����)
	int m_wait_timeout;
    int m_peek_conn_count;       // ��������������ֵ(ĳʱ��)
    static const int DEFAULT_TIMEOUT = 600;

	bool m_inited;               // �Ƿ��ѳ�ʼ��
	ThreadLock m_lock;           //�߳���
	list<Redis*> m_freeconn_list; //��������
};

#endif //