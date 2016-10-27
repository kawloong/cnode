/******************************************************************* 
 *  summery:        ���ݲɼ�֮�û���־
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    ����·��http://host:port/cloud/userlog?kk=v
 ******************************************************************/
#ifndef _DEAL_USERLOG_H_
#define _DEAL_USERLOG_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"

class DealUserlog: public DealBase, public ITaskRun
{
public:
    DealUserlog(struct evhttp_request* req, ThreadData* tdata);

    // ���󵽴����
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // ��̬���ó�ʼ��
    static int Init(void);

private:
    // �����������
    int parseRequest(string& errback);
    int _parseRequestBody(const char* ptr, unsigned int len, string& errback);

    // apppend json string
    void appendJsonObject(string& objstr, const string& name,
        const string& val, bool comma, bool quot=true) const;
    int findObjectId( string& objetid ) const;

    // �û����ߴ���
    int userOnline(void);
    // �û����ߴ���
    int userOffline(void);
    // �û���֤�Ǽ�
    int userAuth(void);

    // ��Ӧ�������˳�
    void respAndExit(void);

protected:
    virtual int run_task(int flag); // -�����߳���ִ��

    virtual void resume(int result);// -��event�߳���ִ��

    struct UserData // ����body�û�����
    {
        string time;
        string mac;
        string signal;
        string auth;
        string ip;
        string req_type;
        int status;
    };

private:
    string m_respbody;
    string m_routeid;
    string m_devid;
    string m_devmac;
    string m_session;
    string m_client_addr;
    string m_module;

    int m_step;
    bool m_isauth; // ��֤����ϱ�

    UserData m_udata;

    static string s_mongopool_name;
    static string s_redispool_name;
};

#endif