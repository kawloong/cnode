/******************************************************************* 
 *  summery:        ��������������
 *  author:         hejl
 *  date:           2016-09-28
 *  description:    ����·��http://host:port/cloud/command/response?kk=v
 ******************************************************************/
#ifndef _DEAL_CMDRESP_H_
#define _DEAL_CMDRESP_H_
#include <string>
#include "dealbase.h"
#include "comm/taskpool.h"


class DealCmdResp: public DealBase, public ITaskRun
{
public:
    DealCmdResp(struct evhttp_request* req, ThreadData* tdata);

    // ���󵽴����
    static int Run(struct evhttp_request* req, ThreadData* tdata);

    // ��̬���ó�ʼ��
    static int Init(void);

protected:
    virtual int run_task(void); // -�����߳���ִ��

    virtual void resume(int result);// -��event�߳���ִ��

private:

private:


};

#endif