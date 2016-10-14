#include <cstdio>
#include "httpclient.h"
#include "comm/public.h"

HttpClient::HttpClient( struct event_base *base, const char *address, unsigned short port )
{
    HttpClient(); // initial member
    m_conn = evhttp_connection_base_new(base, NULL, address, port);
    if (m_conn)
    {
        m_port = port;
        m_address = address;
    }
}

HttpClient::HttpClient( void )
{
    m_port = 0;
    m_timeoutsec = 0;
    m_conn = NULL;
    m_reqcount = 0;
}

HttpClient::~HttpClient( void )
{
    close();
}

void HttpClient::setTimeOut( int sec )
{
    m_timeoutsec = sec;
    if (m_conn)
    {
        evhttp_connection_set_timeout(m_conn, sec);
    }
}

/*
** @summery: 发起http请求GET or POST
** @param: url "/do?key=va" or "http://abc.com/do?key=va";
** @return: 0 sucess
** @remark: 经尝试evhttp_make_request()在连接失败时亦返回0, 并在return前发生CallBack
**          evhttp_make_request()内部首次进行了同步dns解析, 返回后再connect();
*/
int HttpClient::request( const string& url, IHttpClientBack* owner )
{
    int ret;
    size_t pos0 = 0;
    size_t pos1 = 0;
    string hsvr; // host+port
    string obj;
    struct evhttp_uri* uri = NULL;

    do 
    {
        pos0 = url.find("://");
        pos0 = (string::npos==pos0)? 0 : pos0+3;
        pos1 = url.find("/", pos0);
        
        if (string::npos == pos1)
        {
            hsvr.assign(url.c_str() + pos0);
            obj = "/";
        }
        else
        {
            hsvr.assign(url, pos0, pos1-pos0);
            obj.assign(url.c_str() + pos1);
        }


        uri = evhttp_uri_parse(url.c_str());
        IFBREAK_N(NULL == uri, EURL_INVALID);
        // to do: check uri's host&port match this->name

        struct evhttp_request* htpreq = evhttp_request_new(ResponseCB, owner);
        IFBREAK_N(NULL == htpreq, EMEM_ALLOC);

        ret = evhttp_make_request(m_conn, htpreq, EVHTTP_REQ_GET, obj.c_str()); 
        IFBREAK_N(ret, EMAK_REQUEST);

        evhttp_add_header(evhttp_request_get_output_headers(htpreq), "Host", m_address.c_str()); 
    }
    while (0);

    if (uri)
    {
        evhttp_uri_free(uri);
    }

    return ret;
}

// #static
void HttpClient::ResponseCB( struct evhttp_request* req, void* arg )
{
    IHttpClientBack* owner = (IHttpClientBack*)arg;
    static string strnull;

    if (NULL == req)
    {
        static string strconnfail("connect fail");
        LOGERROR("RESPONSECB| msg=req null maybe connect fail| arg=%p", arg);
        if (owner)
        {
            owner->clireq_callback(ECONNECT_FAIL, strnull, strconnfail);
        }
    }
    else
    {
        int rspcode = evhttp_request_get_response_code(req);

        LOGDEBUG("CLIENTCB| msg=%s response", evhttp_request_get_uri(req));

        if (HTTP_OK == rspcode)
        {
            struct evbuffer* buf = evhttp_request_get_input_buffer(req);
            string bodydata((char*)evbuffer_pullup(buf, -1), evbuffer_get_length(buf));

            owner->clireq_callback(0, bodydata, strnull);
        }
        else
        {
            char err[64];
            snprintf(err, sizeof(err), "respcode:%d", rspcode);
            string errmsg(err);

            owner->clireq_callback(0, strnull, errmsg);
        }
    }
}

void HttpClient::close( void )
{
    if (m_conn)
    {
        evhttp_connection_free(m_conn);
        m_conn = NULL;
    }
}

