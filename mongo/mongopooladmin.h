/******************************************************************* 
 *  summery: 管理mongodb连接池集合
 *  author:  hejl
 *  date:    2016-10-10
 *  description: 以poolname为标识管理多个MongoConnPool对象
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
文件依赖  : comm/lock.h comm/public.h  rapidjson/ *.h
so依赖    : libmongoc-1.0.so (经valgrind测试存在内存泄露)

类依赖关系: MongoConnPoolAdmin  --> MongoConnPool   -->   Mongo     -->  mongoContext
            缓存连接池管理(单例)    缓存连接池            缓存类         Mongo api接口
            包括多个池对象          池中有多个缓存类      封装Mongo操作  himongo实现
使用示例:
        Mongo* mgo;
        const char mongofileconf[] = "/etc/bmsh_mongo_connpoll_conf.json"; // 来自json
        // program begin
        assert( MongoConnPoolAdmin::Instance()->LoadPoolFromFile(mongofileconf) );

        // ... 业务流程 ...
        if (0 == MongoConnPoolAdmin::Instance()->GetConnect(mgo, "mongo1"))
        {
            // 调用Mongo相关方法
            mgo->ping();
            // ...

            MongoConnPoolAdmin::Instance()->ReleaseConnect(mgo);
        }
        // ... 业务流程 ...

        // program end
        MongoConnPoolAdmin::Instance()->DestroyPool(NULL);
*******************************************************************/

class MongoConnPoolAdmin
{
public:
	static MongoConnPoolAdmin* Instance(void); // sigleton
	
public:
	/***************************************
	@summery: 从配置文件解析池信息, 加载m_all_conf中
    @param1:  file 来自json中读入的配置, 默认是"bmsh_mongo_connpoll_conf.json"
	@return: 0 成功执行;
	**************************************/
	int LoadPoolFromFile(const char* conffile);

	/***************************************
	@summery: 注册一个连接池,每个进程池通过conf->poolname标识
    @param1:  conf 来自xml中读入的配置
    @param2:  delay 是否延时创建池 // 暂不实现
	@return: 0 成功执行; 1 参数出错; 2 poolname出错; 3 已注册过; 4 创建池失败
	**************************************/
    int RegisterPool(mongo_pool_conf_t* conf);
    int RegisterPool(void);
	
	/***************************************
	@summery: 根据池名字销毁一个连接池
	@param1:  poolname 要销毁的连接池名
	@return: 0 成功执行; 其他出错; 
	**************************************/	
	int DestroyPool(const char* poolname);
	
	/***************************************
	@summery: 根据池名字,获得池内一个Mongo连接
	@param1:  pmongo [out] 接收返回的Mongo对象指针
	@param2:  poolname [in] 要销毁的连接池名
	@return:  0 成功执行; 
	@remark:  GetConnect与ReleaseConnect要求成对使用
	**************************************/	
	int GetConnect(Mongo*& pmongo, const char* poolname);
	
	/***************************************
	@summery: 释放一个由GetConnect接收到的连接对象
	@param1:  pmongo 连接对象, 由GetConnect获得
	@return:  0 成功执行; 其他出错; 
	@remark:  GetConnect与ReleaseConnect要求成对使用
	**************************************/	
	int ReleaseConnect(Mongo* pmongo);
	
public:
	// 打印连接池的当前状态
	void showPoolStatus(const char* poolname);
    void showPoolStatus(string& strbak, const char* poolname);
	
protected: // 单实例对象
	MongoConnPoolAdmin(void);
	~MongoConnPoolAdmin(void);

private:
    RWLock m_lock;
	map<string, MongoConnPool*> m_conn_pools;
    map<string, mongo_pool_conf_t*> m_all_conf; // 非启动时初始化的池
	
};

#endif //
