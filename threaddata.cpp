#include <unistd.h>
#include <event2/http.h>
#include <event2/event.h>
#include "comm/public.h"
#include "threaddata.h"
#include "httpclient.h"
#include "dealbase.h"

int ThreadData::s_total_threadcount = 1;
static const int INVALIDFD = -1;

ThreadData::ThreadData(void): base(NULL), http(NULL),
    asyn_timer(NULL), notify_event(NULL), //sigint_event(NULL), sigterm_event(NULL), 
    threadid(0), thidx(0), processidx(0), exitfg(false)
{
    icomet_cli = NULL;
    pipefd[0] = INVALIDFD;
    pipefd[1] = INVALIDFD;
    pthread_mutex_init(&m_mutex, NULL);
}

ThreadData::~ThreadData()
{
    delAllDealer();
    IFDELETE(icomet_cli);

    IFCLOSEFD(pipefd[0]);
    IFCLOSEFD(pipefd[1]);

    if (asyn_timer)
    {
        event_free(asyn_timer);
        asyn_timer = NULL;
    }

    if (notify_event)
    {
        event_free(notify_event);
        notify_event = NULL;
    }

    if (http)
    {
        evhttp_free(http);
        http = NULL;
    }

    if (base)
    {
        event_base_free(base);
        base = NULL;
    }

    pthread_mutex_destroy(&m_mutex);
}

void ThreadData::delDealer( void )
{
    if (!rmdeal.empty())
    {
        std::list<DealBase*>::iterator it = rmdeal.begin();
        while (it != rmdeal.end())
        {
            delete (*it);
            ++it;
        }

        rmdeal.clear();
    }
}

void ThreadData::delAllDealer( void )
{
    this->delDealer();
    if (!dealing.empty())
    {
        std::map<DealBase*, int>::iterator it = dealing.begin();
        while (it != dealing.end())
        {
            delete (it->first);
            ++it;
        }
        dealing.clear();
    }
}

bool ThreadData::removeDeal( DealBase* ptr )
{
    dealing.erase(ptr);
    rmdeal.push_back(ptr);
    return true;
}

void ThreadData::pushDeal( DealBase* ptr )
{
    if (ptr)
    {
        dealing[ptr] = (int)time(NULL);
    }
}

void ThreadData::pushResume( DealBase* ptr )
{
    pthread_mutex_lock(&m_mutex);
    resume.push_back(ptr);
    pthread_mutex_unlock(&m_mutex);
}

DealBase* ThreadData::popResume( void )
{
    DealBase* ptr = NULL;

    pthread_mutex_lock(&m_mutex);
    if (!resume.empty())
    {
        ptr = resume.front();
        resume.pop_front();
    }
    pthread_mutex_unlock(&m_mutex);

    return ptr;
}

bool ThreadData::mkpipe( void )
{
    if (INVALIDFD == pipefd[0])
    {
        return (0 == pipe(pipefd));
    }

    return false;
}
