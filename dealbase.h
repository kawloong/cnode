/******************************************************************* 
 *  summery:        ҵ�������
 *  author:         hejl
 *  date:           2016-09-12
 *  description:    
 ******************************************************************/
#ifndef _DEALBASE_H_
#define _DEALBASE_H_
#include <string>
#include <map>
#include <event2/http.h>
//#include "interface.h" // only test

/* ����ʵ��˵��:
** 1.ÿ��ҵ����(����һ��path)������class DealBase;
** 2.ʵ��static Run()����, �ṩ��������ʱ��Server����;
** 3.Ҫʹ��http�ͻ��˵���,��ʵ��IHttpClientBack�ӿ�,���class HttpClientʹ��;
** 4.��new������DealBase������,��ҵ������ɺ����AsynDeleteMe()�첽�ͷ�����;
*/

using std::string;
using std::map;
struct ThreadData;
struct IHttpClientBack;

class DealBase
{
    const static int asyn_del_instance_time_sec = 4;

public:
    explicit DealBase(struct evhttp_request* req, ThreadData* tdata);
    virtual ~DealBase(void){}

    // ��̬���ó�ʼ��
    static int Init(void);
    // ����ʵ�����
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // ����postResume()����event�̵߳���resume()����,��������,Ӧ������ʵ��
    virtual void resume(int result);
    
protected:
    // Timer�ص�,�����첽ɾ���������
    static void AsynDelTimerCB(evutil_socket_t, short, void* arg);

    // ��ȡhttp-headerֵ
    static int GetHeadStr(string& val, struct evhttp_request* req, const char* key);
    // ��ȡhttp-uri�еĲ�ѯ��key/value
    static int FindQueryStrValue(string& val, const struct evhttp_request* req, const char* key);
    static int ParseQueryMap(map<string,string>& kvmap, const struct evhttp_request* req);

    // �ظ����ݸ��ͻ������
    static int SendRespond(struct evhttp_request* req, const string& body);
    static int SendRespondFail(struct evhttp_request* req, const string& reson, int ecode=0);

protected:
    // ����http����ص�,��������,Ӧ������ʵ��
    //virtual int clireq_callback(int result, const string& body, const string& errmsg);

    // �첽ɾ������this, ��ҪΪ�����׳���ĵݹ�ɾ��
    void postDeleteMe(void);

    // �����������߳��л�����event�߳���, ������������,֮���ⷢresume()����;
    int postResume( void );

    int pushICometReq(const string& cname, const string& content, IHttpClientBack* cb);
protected:
    struct evhttp_request* m_mainreq;
    ThreadData* m_tdata;
    string m_name;
    int m_begtime;
    bool m_finish;
    static string s_redis_poolname;
    static const string strnull;
};

#endif