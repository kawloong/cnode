/******************************************************************* 
 *  summery:        ������Ȩҵ����
 *  author:         hejl
 *  date:           2016-09-23
 *  description:    ����·��http://host:port/cloud/auth?kk=v
 ******************************************************************/
#ifndef _DEAL_AUTH_H_
#define _DEAL_AUTH_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"

class DealAuth: public DealBase, public ITaskRun
{
public:
    DealAuth(struct evhttp_request* req, ThreadData* tdata);

    // ���󵽴����
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // ��̬���ó�ʼ��
    static int Init(void);

protected:
    virtual int run_task(int flag); // -�����߳���ִ��

    virtual void resume(int result);// -��event�߳���ִ��

private:
    static string MakeSession(ThreadData* tdata);

private:
    string device_id;
    string device_mac;
    string device_model;
    string firmware_version;
    string respbody;
    static string s_icomet_jsonstr;

};

#endif