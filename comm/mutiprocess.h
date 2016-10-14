/******************************************************************* 
 *  summery:        linux������Թ���ʵ��
 *  author:         hejl
 *  date:           2016-09-09
 *  description:    master��������linux�źŻ���,����SIGCHLD
 ******************************************************************/

#ifndef _MUTIPROCESS_H_
#define _MUTIPROCESS_H_
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

/* ----------------  usage  ---------------
 *      ret = MutiProcess::Setup(3, true);
 *      if (ret > 0) // master process exit
 *        exit(0);
 *      else if (ret < 0) // error happen
 *        exit(-1);
 *      // sub-process run here 
 *      // ... 
-------------------------------------------*/


class MutiProcess
{
	static sig_atomic_t s_process_max;
	static sig_atomic_t s_process_cur;
	static sig_atomic_t s_run_state; // �ο�enum RunState

	static const int process_max_limit = 512;

    enum RunState
    {
        FORCEEND, // �����˳�״̬
        WAITSUB,  // �ȴ��ӽ��̽���
        ONETIME,  // �ӽ��̲�������
        RESPAWN,  // �ӽ��̿�����״̬
    };

public:
	/* @summery: ��������̳��򣬵�������Ϊ������
	 * @param: count �ӽ�������
	 * @param: respawn �Ƿ����ӽ��̽������Զ����� 
	 * @return: 0 �ӽ��̳ɹ����أ�>0 ������Ҫ����ʱ����; <0 ����
	 */
	static int Setup(int count, bool respawn)
	{
		int ret = 0;
		s_process_max = (count>process_max_limit)? process_max_limit: count;
        s_run_state = respawn? RESPAWN: ONETIME;
		
        ret = master_run();
		return ret;
	}

    static int GetPIndex(void) { return s_process_cur; }

private:

    // �����ӽ���
    // @return: �ӽ��̷���0,�����̷�������,�����ظ���
	static int mkprocess(void)
	{
		int ret = 1;
		for (; s_process_cur < s_process_max; )
        {
			pid_t pid = fork();
            if (pid > 0) // parent
            {
				s_process_cur++;
				ret = 1;
            }
            else if (0 == pid) // child
            {
				ret = 0;
                break;
            }
            else
            {
				perror("fork fail");
				ret = -1;
                break;
            }
        }
		
		return ret;
	}

    /* @summery: ���ؽ��̻�ͣ���ں����ڲ�,
        ֱ��ȫ������Ҫ�˳��ŷ��ش���0ֵ; �ӽ��̻���������0
    */
	static int master_run(void)
	{
		int ret;
        bool first = true;
	 
        sigset_t sigset;
        sigset_t sigold;
        struct sigaction sa;
        struct sigaction sa_int;
        struct sigaction sa_term;

		
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = sighandle;
		sa.sa_flags = 0;
		sigaction(SIGCHLD, &sa, NULL);
        sigaction(SIGINT, &sa, &sa_int);
        sigaction(SIGTERM, &sa, &sa_term);

		sigemptyset(&sigset);
		sigaddset(&sigset, SIGCHLD);
        sigaddset(&sigset, SIGINT);
        sigaddset(&sigset, SIGTERM);

		if (sigprocmask(SIG_BLOCK, &sigset, &sigold) == -1)
		{
		   perror("master set sigprocmask");
		}

		sigemptyset(&sigset);
		for ( ;; )
        {
            if (FORCEEND == s_run_state) // ������Ҫ�˳�
            {
                ret = 3;
                break;
            }

			if (RESPAWN == s_run_state || first)
			{
				ret = mkprocess();
                first = false;
				if (ret <= 0)
                {
                    // �ָ��ź�MASK���ӽ���
                    sigaction(SIGINT, &sa_int, NULL);
                    sigaction(SIGTERM, &sa_term, NULL);
                    sigprocmask(SIG_SETMASK, &sigold, NULL);
                    break;
                }
			}
            else if (s_process_cur <= 0)
            {
                ret = 2; // �ӽ���ȫ���˳��ˣ�������Ҳ�˳�
                break;
            }
			
			sigsuspend(&sigset);
		}
		
		return ret;
	}

    // �źŴ���ص�
	static void sighandle(int signo)
	{
		switch (signo)
		{
			case SIGTERM:
			case SIGINT:
			//case SIGQUIT:
                // ��1��TERMʱ�ȹر�������ʶ�͵ȴ��ӽ���ȫ���˳���
                // ��2��TERMʱ�����������˳���ʶ; 
                // (����ӽ��̿����˳�,������2��TERM�����̻�˳���˳�)
                if (RESPAWN == s_run_state || ONETIME == s_run_state)
                {
                    s_run_state = WAITSUB;
                }
                else if (WAITSUB == s_run_state)
                {
                    s_run_state = FORCEEND;
                }
                printf("s_run_state-- %d\n", s_run_state);

			break;

			case SIGCHLD:
            {
                printf("SIGCHLD-- %d\n", s_run_state);
				chld_handle();
			}
			break;
			
			default:
				break;
		}

	}
	
    // �ź�(�ӽ��̽���)����ص�
	static void chld_handle(void)
	{
		int status;
		pid_t pid;
		
		for ( ;; )
		{
			pid = waitpid(-1, &status, WNOHANG);

			if (pid == 0)return;

			if (pid == -1)
			{
				if (errno == EINTR)continue;
				return;
			}

			//WEXITSTATUS(status); // exit code
			s_process_cur--;
        }

    }
	
};


sig_atomic_t MutiProcess::s_process_max = 0;
sig_atomic_t MutiProcess::s_process_cur = 0;
sig_atomic_t MutiProcess::s_run_state = 1;
 
 #endif
  