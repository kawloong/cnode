/******************************************************************* 
 *  summery:        ObjectId生成管理类
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    使用共享内存保证机器的ID唯一性
 ******************************************************************/
#ifndef _IDMANAGE_H_
#define _IDMANAGE_H_
#include <string>
#include "comm/lock.h"
#include "comm/public.h"

using std::string;
const char SHMDATA_VERSION = 1;
const int NUMTOFLUSH = 1000; // 增加多少量flush一次

class IdMgr
{
    struct ShmData
    {
        char ver;
        short hostid;
        int addsum; // 统计flush后增加了的量
        long long num;
    };

    SINGLETON_CLASS2(IdMgr);
private:
    IdMgr(void);
    ~IdMgr(void);

public:
    int init(int key, short hostid);
    void uninit(void);

    int jumpObjectCount(int jmpn);
    int getObjectId(string& objectid);

private:
    template<class T>
    int flush(const T& data);
    template<class T>
    int read(T& data);

private:
    key_t m_key;
    int m_shmid; // 共享内存标识;
    ShmData* m_ptr;
    SemLock m_lock;
};

#endif