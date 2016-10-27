/******************************************************************* 
 *  summery:        ObjectId���ɹ�����
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    ʹ�ù����ڴ汣֤������IDΨһ��
 ******************************************************************/
#ifndef _IDMANAGE_H_
#define _IDMANAGE_H_
#include <string>
#include "comm/lock.h"
#include "comm/public.h"

using std::string;
const char SHMDATA_VERSION = 1;
const int NUMTOFLUSH = 1000; // ���Ӷ�����flushһ��

class IdMgr
{
    struct ShmData
    {
        char ver;
        short hostid;
        int addsum; // ͳ��flush�������˵���
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
    int m_shmid; // �����ڴ��ʶ;
    ShmData* m_ptr;
    SemLock m_lock;
};

#endif