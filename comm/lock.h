// �����߳���

#ifndef _LOCK_H_
#define _LOCK_H_

#include <semaphore.h>
#include <pthread.h>
#include <string>

class LockBase
{
public:
    virtual ~LockBase(){}

    virtual int Lock() = 0;
    virtual int Unlock() = 0;
};

class PVBase
{
public:
    virtual ~PVBase(){}

    virtual int WaitP() = 0;
    virtual int PostV() = 0;
};

class LockGuard
{
private:
    LockBase& m_lock;

public:
    LockGuard(LockBase& lock) :m_lock(lock)
    {
        m_lock.Lock();
    }

    ~LockGuard()
    {
        m_lock.Unlock();
    }

private:
    LockGuard(const LockGuard&);
    LockGuard& operator=(const LockGuard&);
};

// ͨ���ֲ����������Զ��ͷ���
template<class T>
class _AutoRelease
{
	typedef int (T::*TFun)(void);
public:
	_AutoRelease(T& obj, TFun in_func, TFun out_func):
	  m_obj(obj), m_outf(out_func) { if (in_func)(obj.*in_func)();}   

	  ~_AutoRelease() { (m_obj.*m_outf)(); }

private:
	T& m_obj;
	TFun m_outf;
};

// �ⲿ�ӿ�: ����ͬ������,����, ��������, �ͷŷ���
#define LockGuardEx(obj, Cls, in_method, out_method) \
	_AutoRelease<Cls> _lock_(obj, &Cls::in_method, &Cls::out_method)


// �߳���
class ThreadLock : public LockBase
{
public:
    ThreadLock();
    virtual ~ThreadLock();
    
    int Lock();
    int Unlock();

private:
    ThreadLock( const ThreadLock& );
    ThreadLock& operator=( const ThreadLock& );

private:
    pthread_mutex_t m_mutex;
};

// �ɵݹ���߳���
class RecursiveLock : public LockBase
{
public:
    RecursiveLock();
    virtual ~RecursiveLock();

    int Lock();
    int Unlock();

private:
    RecursiveLock( const RecursiveLock&);
    RecursiveLock& operator=( const RecursiveLock& );

private:
    pthread_mutex_t m_mutex;
};

class ConditionMutex : public LockBase
{    
public: 
    ConditionMutex();
    virtual ~ConditionMutex();

    int Lock();
    int Unlock();
    int Wait();
    int Wait(int nWaitTime);
    int Signal();

private: 
    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;

private:
    ConditionMutex( const ConditionMutex& );
    ConditionMutex& operator=( const ConditionMutex& );
};

// ��д��
class RWLock
{
public:
    RWLock();
    RWLock(bool muti_thread);
	~RWLock();

	int RLock(void);
	int WLock(void);
	int UnLock(void);

private:
	RWLock( const RWLock& );
	RWLock& operator=( const RWLock& );

private:
	pthread_rwlock_t m_rwlock;
    bool m_mutithread; // �Ƿ���߳�; (Ŀ�����ڵ��̻߳����к��Լ�������)
};
// ��д���Զ�ջ�ڼӽ���
#define RWLOCK_READ(rwlock) LockGuardEx(rwlock, RWLock, RLock, UnLock)
#define RWLOCK_WRITE(rwlock) LockGuardEx(rwlock, RWLock, WLock, UnLock)


// ���ź���ʵ�ֵ������������̻߳���̼�Ļ���
class SemLock : public LockBase
{
    union semun {
        int              val;
        struct semid_ds* buf;
        unsigned short*  array;
        struct seminfo*  __buf;
    };
    
public:
    SemLock();
    virtual ~SemLock();

    int Lock();
    int Unlock();

    int Init(const std::string& strPath);
    int Init(key_t key);

private:
    SemLock( const SemLock& );
    SemLock& operator=( const SemLock& );

protected:
    void Finish();

private:
    int m_iSemid;
};

// �ļ�����ֻ�����ڽ��̼��������Ҫ�����̼߳����
class FileLock : public LockBase
{
public:
    FileLock();
    virtual ~FileLock();

    int Lock();
    int Unlock();

    int Init(const std::string& filePath);
    int Init(int fd);

private:
    int m_fd;
    bool m_bOpen;

private:
    FileLock( const FileLock& );
    FileLock& operator=( const FileLock& );
};

// ֧��PV�������ź���
class SemPV : public PVBase
{
public:
    SemPV();
    virtual ~SemPV();

    int WaitP();
    int PostV();

private:
    sem_t m_sem;

private:
    SemPV( const SemPV& );
    SemPV& operator=( const SemPV& );
};

#endif

