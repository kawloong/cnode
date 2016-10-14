/******************************************************************* 
 *  summery:        linux文件操作类
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
    // 文件或目录是否存在
    static bool Exists(const char* name);
    // 是否是目录
    static bool Isdir(const char* dirname);
    // 是否是普通文件
    static bool Isfile(const char* filename);

    // 递归创建目录
    static bool CreatDir_r(const char* path);
    // 根据绝对文件路径，获取所在目录路径
    static bool GetPath(const char* fullfile, string& path, bool nosep);
    // 根据绝对文件路径，获取文件名
    static bool GetFilename(const char* fullfile, string& name);
};


#endif
