#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "file.h"


bool File::Exists(const char* name)
{
    struct stat st;
    return stat(name?name:"", &st) == 0;
}

bool File::Isdir(const char* dirname)
{
    struct stat st;
    if(stat(dirname, &st) == -1)
    {
        return false;
    }
    return (bool)S_ISDIR(st.st_mode);
}

bool File::Isfile(const char* filename)
{
    struct stat st;
    if(stat(filename, &st) == -1){
        return false;
    }
    return (bool)S_ISREG(st.st_mode);
}

bool File::CreatDir_r( const char* path )
{
    if (NULL == path) return false;

    int nHead, nTail;   
    std::string strDir(path);
    if ('/' != strDir[0] && '.' != strDir[0])
    {
        strDir = "./" + strDir;
    }
    if ('/' == strDir[strDir.size()-1])
    {
        strDir.erase(strDir.size()-1, 1);
    }

    DIR* pDir = opendir(path);

    if (NULL == pDir)
    {
        nHead = strDir.find("/");
        nTail = strDir.rfind("/");
        if (nHead != nTail)
        {
            if (false == CreatDir_r(strDir.substr(0, nTail).c_str()))
            {
                return false;
            }
        }

        if (-1 == mkdir(strDir.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH))
        {      
            return false;
        }
    }
    else
    {
        closedir(pDir);
    } 

    return true;
}

bool File::GetPath(const char* fullfile, string& path, bool nosep)
{
    struct stat buf;
    path = fullfile;

    unsigned length = path.length();
    int result = stat(fullfile, &buf);
    if (0 == result && S_ISDIR(buf.st_mode))
    {
        return true;
    }

    while (--length > 0)
    {
        if (path[length] == '/' || path[length] == '\\')
        {
            if (nosep)
            {
                path[length] = 0;
            }
            break;
        }
        else
        {
            path[length] = 0;
        }
    }

    return true;
}

bool File::GetFilename(const char* fullfile, string& name)
{
    int len = strlen(fullfile);
    while (--len >= 0)
    {
        if ('/' == fullfile[len] || '\\' == fullfile[len])
        {
            name = fullfile+len+1;
            return true;
        }
    }
    name = fullfile;
    return true;
}

