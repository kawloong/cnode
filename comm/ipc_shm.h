/******************************************************************* 
 *  summery:        IPCͨ��֮�����ڴ���
 *  author:         hejl
 *  date:           2016-10-17
 *  description:    ��shmget shmat shmdt��(�Ȳ���װ)
 ******************************************************************/
#ifndef _IPC_SHM_H_
#define _IPC_SHM_H_


class IpcShm
{
public:
    IpcShm(key_t key);
    IpcShm(const char* pathfile, int proj_id); // call ftok()

    int init(unsigned int size);

private:
    int m_shmid; // �����ڴ��ʶ;
    void* m_ptr;
};

#endif
