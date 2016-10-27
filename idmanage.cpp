#include <errno.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "idmanage.h"


IdMgr::IdMgr( void ): m_shmid(-1), m_ptr(NULL)
{

}

IdMgr::~IdMgr( void )
{
    uninit();
}

// 初始化objectid的共享内存, 用于产生唯一ID
int IdMgr::init( int key, short hostid )
{
    int ret;
    m_key = key;

    do 
    {
        void* tmpptr = NULL;
        bool first = true;

        ret = m_lock.Init(key);
        ERRLOG_IF1BRK(ret, -1, "IDMGRINIT| msg=sem init fail| key=%d", key);

        LockGuard lk(m_lock);
        m_shmid = shmget(key, sizeof(ShmData), IPC_CREAT|IPC_EXCL|0660);
        if (m_shmid < 0)
        {
            if (EEXIST == errno)
            {
                first = false;
                m_shmid = shmget(key, sizeof(ShmData), 0660);
                ERRLOG_IF1BRK(m_shmid<0, -2, "IDMGRINIT| msg=shmget fail(%s)", strerror(errno));
            }
        }

        tmpptr = shmat(m_shmid, 0, 0);
        ERRLOG_IF1BRK((void*)-1==tmpptr || 0==tmpptr, -3, "IDMGRINIT| msg=shmat fail(%s)| shmid=0x%x",
            strerror(errno), m_shmid);
        m_ptr = (ShmData*)tmpptr;

        if (first)
        {
            m_ptr->ver = SHMDATA_VERSION;
            m_ptr->hostid = hostid;
            m_ptr->num = 0;
            LOGINFO("IDMGRINIT| msg=create shm sucess| shmid=0x%x| ver=%d", m_shmid, (int)m_ptr->ver);

            // to do: 当共享内存被删时恢复
        }
        else
        {
            // 检查版本
            m_ptr->hostid = hostid;
        }

        ret = 0;
    }
    while (0);
    return ret;
}

void IdMgr::uninit( void )
{
    if (m_ptr)
    {
        shmdt(m_ptr);
        m_ptr = NULL;
    }
}

int IdMgr::jumpObjectCount(int jmpn)
{
    IFRETURN_N(NULL == m_ptr, -1);

    LockGuard lk(m_lock);
    m_ptr->num += jmpn;

    return 0;
}

int IdMgr::getObjectId(string& objectid)
{
    char buff[32];
    long long nn;
    int addsum;
    int hostid;

    IFRETURN_N(NULL == m_ptr, -1);
    {
        LockGuard lk(m_lock);
        nn = ++m_ptr->num;
        addsum = ++m_ptr->addsum;
        hostid = m_ptr->hostid;
    }

    snprintf(buff, sizeof(buff), "%llx%02x", nn, hostid);
    objectid = buff;
    if (addsum > NUMTOFLUSH) // 满一定量后执行一下持久化
    {
        {
            LockGuard lk(m_lock);
            m_ptr->addsum = 0; // reset count++
        }

        flush(nn);
    }

    return 0;
}

template<class T>
int IdMgr::flush(const T& data)
{
    int ret = -1;
    FILE* file = fopen(".objectid", "w");

    if (file)
    {
        size_t nw = fwrite(&data, sizeof(T), 1, file);
        ret = (nw >= sizeof(T)? 0 : -2);
        fclose(file);
    }

    ERRLOG_IF1(ret, "FLUSHOBJID| msg=fail(%s)", strerror(errno));

    return ret;
}

template<class T>
int IdMgr::read(T& data)
{
    int ret = -1;
    FILE* file = fopen(".objectid", "r");

    if (file)
    {
        size_t nr = fread(&data, sizeof(T), 1, file);
        ret = (nr >= sizeof(T)? 0 : -2);
        fclose(file);
    }

    ERRLOG_IF1(ret, "READOBJID| msg=fail(%s)", strerror(errno));

    return ret;
}