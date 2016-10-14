#ifndef _MONGOPOOL_H_
#define _MONGOPOOL_H_
#include <list>
#include "mongo.h"
#include "comm/lock.h"

using std::list;

struct mongo_pool_conf_t
{
    string poolname;
    string mongo_uri; // 超时参数可包含在uri中
    string db_name;
    string coll_name; //collection

    int processMaxConnNum; // 最大连接数
    int initConnNum; // 进程初始启动时创建的长连接数, 取值[0-processMaxConnNum]

    mongo_pool_conf_t(): processMaxConnNum(0), initConnNum(0) {}
};

class MongoConnPool
{
public:
	/***************************************
	@summery: 获取池内一个连接, 通过Mongo输出参数返回
	@param1:  Mongo [out] 调用者传入接收的指针
	@param2:  only_pool [in] 是否仅限返回池中长连接; 当true时,无长连接时返回短连接
	@return:  0 成功执行; 其他出错; 
	@remark:  getConnect和relConnect必须匹配使用,外部不要对其delete
	**************************************/	
	int getConnect(Mongo*& mog, bool only_pool = false);
	
	/***************************************
	@summery: 释放回一个连接
	@param1:  mog [in] 由getConnect获得的指针
	@return:  0 成功执行; 其他出错; 
	@remark:  getConnect和relConnect必须匹配使用,外部不要对其delete
	**************************************/	
	void relConnect(Mongo* mog) ; // 释放连接

public:
	MongoConnPool(void);
	~MongoConnPool(void);
	
	// 初始化池对象
	int init(mongo_pool_conf_t* conf);
	// 反初始化池对象
	void uninit(void);

    // 查看对象运行状态信息
    void trace_stat(string& msg, bool includeFreeConn);

private:
	inline bool can_create_poolconn(void); // 判断是否有连接余额
	int init_pool(unsigned int count);
	
private: //const method
	Mongo* create_connect(bool onlypool);         // 创建一个连接
	void destroy_connect(Mongo* p) const;      // 销毁一个连接
	
private:
	mongo_pool_conf_t* m_conf;
	int m_conn_count_process;    // 进程内的已开的常连接数
	int m_conn_max_process;      // 进程内的池中允许最大连接数
	//int m_freeconn_count;      // 进程内连接池中的可用连接数(可由m_freeconn_list.size()得到)
	int m_totalconn_count;       // 进程内当前总活动连接数(包括短连接和长连接)
    int m_peek_conn_count;       // 进程内连接数峰值(某时间)

	bool m_inited;               // 是否已初始化
	RWLock m_lock;               //线程锁
	list<Mongo*> m_freeconn_list; //空闲连接
    mongoc_client_pool_t* m_pool;
    mongoc_uri_t* m_uri;
};

#endif //