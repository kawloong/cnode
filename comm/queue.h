/*-------------------------------------------------------------------------
summery  : �̰߳�ȫ��������
Date  09/26/2008
Author zhanglei
Description  : �������ж��߳�ͬʱ������������, �ڲ��Դ���������֪ͨ����
Modification Log:
--------------------------------------------------------------------------
<���ڣ���ʽmm/dd/yyyy>   <��д����>     zhanglei   <�޸ļ�¼˵��>
<2016-09-27>  hejl  ������ʱ������
-------------------------------------------------------------------------*/
#ifndef __GENERAL_QUEUE_H__
#define __GENERAL_QUEUE_H__

//#include <assert.h>
#include <map>
#include <deque>
#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <sys/time.h>

using namespace std; 


template<bool isPtr=false>
class Bool2Type
{
	enum{eType = isPtr};
};

static bool operator<(struct timeval l, struct timeval r)
{
    bool ret = (l.tv_sec == r.tv_sec) ? (l.tv_usec < r.tv_usec): (l.tv_sec < r.tv_sec);
    return ret;
}

template< typename T, bool isPtr=false, typename storage=deque<T> >
class Queue
{
    //struct timeval m_basetime; // �����ʱ���
    map< struct timeval, vector<T> > m_dely_task;
    struct timeval m_near_time;
    typedef typename map< struct timeval,vector<T> >::iterator DTASK_ITER;
    typedef typename vector<T>::iterator DLIST_ITER;

public:
    bool append_delay(const T &pNode, int delay_ms) // �����Ӻ�ⷢ������
    {
        struct timeval destime;
        bool bnotify = true;

        if ( !pNode ) return false;

        if (delay_ms <= 0)
        {
            return append(pNode); // ��ʱ����
        }

        now_dtime(&destime, delay_ms);

        pthread_mutex_lock(&m_mutex);
        if (!m_dely_task.empty())
        {
            DTASK_ITER it = m_dely_task.begin();
            bnotify = (destime < it->first);
        }
        
        m_dely_task[destime].push_back(pNode);

        if (bnotify)
        {
            pthread_cond_signal(&m_cond);
        }
        
        pthread_mutex_unlock(&m_mutex);

        return true;
    }

    bool pop_delay(T &t)
    {
        bool got = false;
        struct timeval nexttime;
        
        pthread_mutex_lock(&m_mutex);
        if(m_list.empty())
        {
            got = _pop_delay_top(t, &nexttime);

            if (!got) // δ��ȡ, �ȴ�
            {
                struct timespec abstime;

                m_near_time.tv_sec = nexttime.tv_sec;
                m_near_time.tv_usec = nexttime.tv_usec;

                TIMEVAL_TO_TIMESPEC(&nexttime, &abstime);
                {
                    struct timeval now;
                    gettimeofday(&now, NULL);
                    printf("Wait cond %ld sec %ld ns | now=%ld.%ld\n", // debug
                        abstime.tv_sec, abstime.tv_nsec, now.tv_sec, now.tv_usec);
                }
                pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);

                if (m_list.empty())
                {
                    got = _pop_delay_top(t, &nexttime);
                }
                else
                {
                    t = m_list.front();
                    m_list.pop_front();
                    got = true;
                }
            }
        }
        else
        {
            t = m_list.front();
            m_list.pop_front();
            got = true;
        }

        pthread_mutex_unlock(&m_mutex);

        return got;
    }

private:
    // ��õ�ǰʱ��ƫ��delta_ms����, ��t����
    void now_dtime(struct timeval* t, int delta_ms)
    {
        const static int s_1sec_in_us = 1000000;
        gettimeofday(t, NULL);
        t->tv_sec += delta_ms/1000;
        t->tv_usec += (delta_ms%1000) * 1000;
        if (t->tv_usec > s_1sec_in_us) // ��λ��ȷ����
        {
            t->tv_sec++;
            t->tv_usec %= s_1sec_in_us;
        }
    }

    // ����ʱ�Ѽ���
    bool _pop_delay_top(T &t, struct timeval* ntime)
    {
        bool got = false;
        // ������ȡ��ʱ����
        if ( !m_dely_task.empty() )
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            DTASK_ITER it = m_dely_task.begin();
            if (it->first < now) // ��ʱʱ���ѵ�
            {
                DLIST_ITER lsit = it->second.begin();
                for (; lsit != it->second.end(); ++lsit)
                {
                    if (got)
                    {
                        m_list.push_back(*lsit);
                    }
                    else
                    {
                        t = *lsit;
                        got = true;
                    }
                }

                m_dely_task.erase(it);
            }
            else
            {
                ntime->tv_sec = it->first.tv_sec;
                ntime->tv_usec = it->first.tv_usec;
            }
        }
        else
        {
            now_dtime(ntime, g_defwait_time_ms);
        }

        return got;
    }


public:
    static const int def_timeout_us = 60*1000000;
	typedef void (*FUNC)(T &t);
    Queue() 
	{
        m_nListMaxSize = 10000;
		pthread_mutex_init(&m_mutex, NULL);
		pthread_cond_init(&m_cond, NULL);
        m_near_time.tv_sec = 10000;

        //gettimeofday(&m_basetime, NULL);
	}

   
	
    virtual ~Queue()
	{
		desDel(Bool2Type<isPtr>() );
		
		pthread_cond_broadcast(&m_cond);
		pthread_mutex_unlock(&m_mutex);
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutex);
		
	}

    void wakeup(void)
    {
        pthread_cond_broadcast(&m_cond);
    }

    void SetMaxSize(unsigned int nMaxSize)
    {
        if (nMaxSize < 100 || nMaxSize > 1000000)
        {
            return;
        }
        m_nListMaxSize = nMaxSize;
    }

	void desDel(Bool2Type<false>)
	{
		
	}
	void desDel(Bool2Type<true>)
	{
		//cout << "����ɾ������\n";
		
		pthread_mutex_lock(&m_mutex);
		typename storage::iterator iter;
		for ( iter = m_list.begin(); iter !=m_list.end(); iter++ )
		{
			if(NULL != *iter)
			delete ( *iter );
		}
		pthread_mutex_unlock(&m_mutex);
		
	}
    void insert(int index,  const T &pNode)
	{
        pthread_mutex_lock(&m_mutex);
        typename storage::iterator iter = m_list.begin();
        advance( iter, index);
        m_list.insert( iter, pNode);
		pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
	}
	
    bool append(const T &pNode, int nMaxWaitSeconds = 0)
    {
        bool bRet = false;

        int nSeconds = 0;
        while (!bRet)
        {
            nSeconds++;
            pthread_mutex_lock(&m_mutex);
            if (m_list.size() < m_nListMaxSize)
            {
                m_list.push_back(pNode);
                bRet = true;
                pthread_cond_signal(&m_cond);
            }
            pthread_mutex_unlock(&m_mutex);  

            if (!bRet)
            {
                if (nSeconds > nMaxWaitSeconds) break;
                sleep(1);
            }
        }

        return bRet;         
    }


	//��λ΢��
	int waitToNotEmpty(int timeout=def_timeout_us)
	{
		int nRet = 0;
		pthread_mutex_lock(&m_mutex);
		if(m_list.empty())
		{
			struct timespec abstime;
			this->maketime(&abstime, timeout);
			
			pthread_cond_timedwait(&m_cond,&m_mutex,&abstime);
		}

		if(m_list.empty())
		{
			nRet = -1;
		}
		
		pthread_mutex_unlock(&m_mutex);
		return nRet;
	}
	
	//�Ӷ�����ɾ��Ԫ�ص�λ΢��1us=1/1000000s
	bool pop(T &t, int timeout_us=def_timeout_us) //��λ΢��
	{
		pthread_mutex_lock(&m_mutex);
		
		if(m_list.empty())
		{
            if (-1 == timeout_us) // ���õȴ�
            {
                pthread_cond_wait(&m_cond, &m_mutex);
            }
            else
            { // �����-lpthread��������
			    struct timespec abstime;
			    this->maketime(&abstime, timeout_us);
			    pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);
            }
		}
		
		if(m_list.empty())
		{
			pthread_mutex_unlock(&m_mutex);
			return false;
		}

		t = m_list.front();
		m_list.pop_front();
		
		pthread_mutex_unlock(&m_mutex);
		return true;
	}

	
    int size()
	{
		int nSize = 0;
		pthread_mutex_lock(&m_mutex);
		nSize = m_list.size();
        nSize += m_dely_task.empty()? 0 : 1;
		pthread_mutex_unlock(&m_mutex);
		return nSize;
	}

	void each(FUNC func, bool rmall)
    {
        DTASK_ITER it;
		pthread_mutex_lock(&m_mutex);
		
		typename storage::iterator iter = m_list.begin();
		for(; iter != m_list.end(); iter++)
		{
			(*func)(*iter);
		}

        it = m_dely_task.begin();
        for (; it != m_dely_task.end(); ++it)
        {
            DLIST_ITER lsit = it->second.begin();
            for (; lsit != it->second.end(); ++lsit)
            {
                T t = *lsit;
                func(t);
            }
        }

        if (rmall)
        {
            m_list.clear();
            m_dely_task.clear();
        }
		
		pthread_mutex_unlock(&m_mutex);
		return ;
		
	}
    
protected:
	//΢��1s=1000ms=1000000us=1000000000ns
	void maketime(struct timespec *abstime, int timeout)
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		abstime->tv_sec = now.tv_sec + timeout/1000000;
		abstime->tv_nsec = now.tv_usec*1000 + (timeout%1000000)*1000;
		return;
	}

	//bool m_bIsPtr;
    //������� ���б�
    storage  m_list;
      //Ԫ��ָ��
    typedef typename storage::iterator NODE_ITEM;
    //�߳���
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;
	//bool m_bInit;
    unsigned int m_nListMaxSize;
    static const int g_defwait_time_ms = 100000000;
};



#endif



