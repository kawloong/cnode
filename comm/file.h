/******************************************************************* 
 *  summery:        linux�ļ�������
 *  author:         hejl
 *  date:           2016-09-07
 *  description:    
 ******************************************************************/
#ifndef _FILE_H_
#define _FILE_H_
#include <string>

using std::string;

class File
{
public:
    // �ļ���Ŀ¼�Ƿ����
    static bool Exists(const char* name);
    // �Ƿ���Ŀ¼
    static bool Isdir(const char* dirname);
    // �Ƿ�����ͨ�ļ�
    static bool Isfile(const char* filename);

    // �ݹ鴴��Ŀ¼
    static bool CreatDir_r(const char* path);
    // ���ݾ����ļ�·������ȡ����Ŀ¼·��
    static bool GetPath(const char* fullfile, string& path, bool nosep);
    // ���ݾ����ļ�·������ȡ�ļ���
    static bool GetFilename(const char* fullfile, string& name);
};


#endif
