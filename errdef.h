

enum 
{
    ERR_BASE = 100,
    ERR_SERVER_LISTEN,
    ERR_SERVER_EVNEW,
    ERR_SERVER_ACCEPT,
    ERR_SERVER_PIPE,

    ERR_DEAL_BASE = 500,
    ERR_PARAMETER, // ������������ȱʧ
    ERR_HEADER,
    ERR_ENCODING,
    ERR_JSON_FORMAT,
    ERR_SYSTEM,
    ERR_MD5INVALID, // MD5���Ϸ�
    ERR_REDIS,

    ERR_SYSBUG, // ���ڳ����߼�����
};