#include <signal.h>
#include <errno.h>

#include "comm/version.h"
#include "comm/daemon.h"
#include "comm/log.h"
#include "comm/bmsh_config.h"
#include "comm/strparse.h"
#include "comm/mutiprocess.h"
#include "comm/public.h"
#include "server.h"

bool g_bExit = false;

string g_str_pwdpath;
string g_str_exename;

#define DEF_CONFILE "connect.conf"
#define LOGFILE_NAME "cnodeconnect.log"
#define PROGRAM_NAME "CNodeConnect"
#define PROGRAM_VER "1.0.0"

//pthread_key_t g_thread_key; // for taskno log


enum _ret_main_
{
    RET_NORMAL   = 0,
    RET_SIGNAL   = 1,  // 信号注册
    RET_CONFIG   = 2,  // conf初始化失败
    RET_LOG      = 3,  // 日志服务初始化失败
    RET_COMMOND  = 4,  // 进程管理命令返回(start,stop,status)
    RET_MASTER   = 5,  // master进程返回
    RET_SERVICE  = 10,  // 业务初始化失败
};

struct ParamConf
{
    string inifile; // config file
    string logfile; // +
    int port;
    int threadcount;
    bool daemon;
}g_param;

static void sigdeal(int signo)
{
    printf("signal happen %d, thread is %x\n", signo, (int)pthread_self());
	g_bExit = true;
    Server::Instance()->Stop();
}

static int loginit(void);
static int sigregister(void);
static int parse_cmdline(int argc, char** argv);
#define PROGRAM_EXIT(retcode) result = retcode; goto mainend;

int main(int argc, char* argv[])
{
    int result = 0;

    // 解析命令行参数
    if ( (result = parse_cmdline(argc, argv)) )
    {
        PROGRAM_EXIT(result);
    }

    // 日志例程初始化
    if (BmshConf::Init(g_param.inifile.c_str()))
    {
        PROGRAM_EXIT(RET_CONFIG);
    }
    
    if (loginit())
    {
        PROGRAM_EXIT(RET_LOG);
    }

    // 信号处理(多线程下,全部线程共用相同处理函数,并且只有1个线程获得信号处理)
    if (sigregister())
    {
        PROGRAM_EXIT(RET_SIGNAL);
    }

    // 主服务启动
    {
        if (Server::Instance()->Init(BmshConf::ThreadNum(), g_param.port))
        {
            PROGRAM_EXIT(RET_SERVICE);
        }

        // 多进程管理
        int process_num = BmshConf::ProcessNum();
        LOGDEBUG("msg=master process run| subprocess=%d", process_num);
        if (process_num > 0 && MutiProcess::Setup(process_num, BmshConf::Respawn()))
        {
            PROGRAM_EXIT(RET_MASTER);
        }

        result = Server::Instance()->Run(MutiProcess::GetPIndex());
    }

mainend:


    LOGINFO("PROGRAM_END| msg=%s exit(%d)", PROGRAM_NAME, result);

	return 0;
}

// 获取参数
int parse_cmdline(int argc, char** argv)
{
    int ret = 0;
    int c;
    int port = BmshConf::ListenPort();
    int daemon = 0;
    string conffile(DEF_CONFILE);

    while ((c = getopt(argc, argv, "c:p:vd")) != -1)
    {
        switch (c)
        {
            case 'c' :
                conffile = optarg;
                break;
            case 'p' :
                port = atoi(optarg);
                break;
            case 'd' :
                daemon = 1;
                break;

            case 'v':
                version_output(PROGRAM_NAME, PROGRAM_VER);
                exit(0);
                break;

            case 'h':
            default :
                printf("command parameter fail! \n"
                    "\t-c <f.conf> configure file name\n"
                    "\t-p <port> listen port\n"
                    "\t-v show program version\n"
                    "\t-d  run as a deamon\n");
                ret = RET_COMMOND;
                break;
        }
    }

    g_param.inifile = conffile;
    g_param.port = port;
    g_param.daemon = daemon;

    return ret;
}

static int sigregister(void)
{
    int ret = 0;
    struct sigaction sa;

#define Sigaction(signo) if (sigaction(signo, &sa, NULL) == -1){ perror("signal req fail:"); ret=errno; }

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigdeal;
    Sigaction(SIGUSR1);
    Sigaction(SIGINT);
    Sigaction(SIGTERM);

    sa.sa_handler = SIG_IGN;
    Sigaction(SIGALRM);
    Sigaction(SIGPIPE);
    Sigaction(SIGHUP);
    Sigaction(SIGURG);
    Sigaction(SIGCONT);
    Sigaction(SIGTTIN);
    Sigaction(SIGTTOU); 
    Sigaction(SIGIO);
 
    return ret;
}

static int loginit( void )
{
    int result;
    g_param.logfile = BmshConf::GETLOGPATH();
    StrParse::AdjustPath(g_param.logfile, true);
    g_param.logfile += LOGFILE_NAME;
    result = log_open(g_param.logfile.c_str());
    LOGINFO("LOGINIT| result=%d| file=%s", result, g_param.logfile.c_str());
    return result;
}


