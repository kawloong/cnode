/******************************************************************* 
 *  summery:        基于libevent的http客户端类
 *  author:         hejl
 *  date:           2016-09-07
 *  description:    暂时未使用evdns_base
 ******************************************************************/
#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_
#include <event2/http.h>
#include <event2/buffer.h>
#include <string>
#include "interface.h"

using std::string;
enum RspErr
{
    EURL_INVALID = 1,
    EMEM_ALLOC, 
    EMAK_REQUEST,
    ECONNECT_FAIL,
};

class HttpClient
{
public:
    HttpClient(struct event_base *base, const char *address, unsigned short port);
    ~HttpClient(void);

    void setTimeOut(int sec);

    int request(const string& url, IHttpClientBack* owner);

    void close(void);
private:
    HttpClient(void);

    static void ResponseCB(struct evhttp_request* req, void* arg);

private:
    string m_name; // = "m_address:m_port"
    string m_address;
    unsigned short m_port;
    int m_timeoutsec;
    struct evhttp_connection* m_conn;
    int m_reqcount;
};

#endif
