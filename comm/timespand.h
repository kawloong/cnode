/******************************************************************* 
 *  summery:        ͳ�Ƴ�������ʱ����
 *  author:         hejl
 *  date:           2016-06-30
 *  description:    
 ******************************************************************/
#ifndef _TIMESPAND_H__
#define _TIMESPAND_H__
#include <sys/time.h>

/* remark:
*       psec���ܵĺ�ʱ������;
*       pms���ܵĺ�ʱ����
*       psec��pms���������,֮�����ۼӹ�ϵ; 
*/

class TimeSpand
{
public:
    TimeSpand(void);
    TimeSpand(long* psec, long* pms);
    ~TimeSpand(void);
    void reset(void);

    // ������;�ڼ��ʱ��: ���ù�����󵽵������淽��ʱ����ʱ
    long spandMs(bool breset = false);
    long spandSecond(bool breset = false);

protected:
    long* m_psec;
    long* m_pms;
    struct timeval m_begin_t;
};

#endif
