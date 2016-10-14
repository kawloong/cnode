#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "strparse.h"

// ��dv��Ϊ�ָ������str�����õ��Ľ����ŵ�data����
int StrParse::SpliteInt(vector<int>& data, const string& str, char dv, int nildef)
{
    vector<string> vstr;
    vector<string>::iterator it;

    SpliteStr(vstr, str, dv);
    it = vstr.begin();
    for (; it != vstr.end(); ++it)
    {
        if (it->empty())
        {
            data.push_back(nildef);
        }
        else
        {
            data.push_back(atoi(it->c_str()));
        }
    }

    return 0;
}

// ��dv��Ϊ�ָ������str�����õ��Ľ����ŵ�data����
int StrParse::SpliteStr(vector<string>& data, const string& str, char dv)
{
    if (!str.empty())
    {
        const char* pstr = str.data();
        size_t len = str.size();
        for (unsigned int i = 0, begi = 0; i < len; ++i)
        {
            if (i+1 == len)
            {
                string item(pstr+begi, len-begi);
                data.push_back(item);
                break;
            }
            else if (pstr[i] == dv)
            {
                string item(pstr+begi, i-begi);
                data.push_back(item);

                begi = i + 1;
            }
        }
    }

    return 0;
}

// @summery: query_string��������, ����qstr��"key1=va11&key2=vv2&key3=123"��ʽ;
// @param: outPar [out] ���ս�Ҫ���outPar[key1]="val1"..
int StrParse::SpliteQueryString( map<string, string>& outPar, const string& querystr )
{
    int ret = 0;

    bool findkey = true;
    int begidx = 0;
    string key;
    string value;
    const char* qstr = querystr.data();
    size_t qslen = querystr.length();

    for (int i = 0; i < (int)qslen; )
    {
        if (findkey)
        {
            if ('=' == qstr[i])
            {
                key.assign(qstr+begidx, i-begidx);
                begidx = i + 1;
                findkey = false;
            }
        }
        else
        {
            if ('&' == qstr[i])
            {
                value.assign(qstr+begidx, i-begidx);
                begidx = i + 1;
                findkey = true;
                outPar[key] = value;
                //printf("[%s] -> %s\n", key.c_str(), value.c_str());
            }
        }

        // last part
        if (0 == qstr[++i])
        {
            if (findkey)
            {
                // skip
            }
            else
            {
                value.assign(qstr+begidx, i-begidx);
                outPar[key] = value;
                //printf("[%s] -> %s\n", key.c_str(), value.c_str());
            }

            break;
        }
    }

    return ret;
}

int StrParse::PickOneJson(string& ostr, const string& src, const string& name)
{
    // תСд
    string tempstr;
    int ret = 1;
    bool found = false;
    size_t namelen = name.length();
    const char* pchstr = src.data();
    size_t len = src.length();

    for (int i = 0; i < (int)len; ++i)
    {
        char rch = pchstr[i];
        if (rch >= 'A' && rch <= 'Z')
        {
            rch = 'a' + rch - 'A';
        }

        tempstr.append(1, rch);
    }

    do 
    {
        char chkch;
        size_t namepos = tempstr.find(name);

        // �ҵ�һ����ȫƥ���key
        while (namepos != string::npos)
        {
            pchstr = tempstr.data();
            found = true;
            if (namepos > 0)
            {
                chkch = pchstr[namepos-1];
                if ((chkch >= 'a' && chkch <= 'z') ||
                    (chkch >= '0' && chkch <= '9') ||
                    ('_' == chkch || '-' == chkch) )
                {
                    found = false;
                }
            }

            if (found && namepos+namelen < len)
            {
                chkch = pchstr[namepos+namelen];
                if ((chkch >= 'a' && chkch <= 'z') ||
                    (chkch >= '0' && chkch <= '9') ||
                    ('_' == chkch || '-' == chkch) )
                {
                    found = false;
                }
            }

            if (found) break;

            namepos = tempstr.find(name, namepos+namelen);
        }

        if (!found) break; // �Ҳ���key

        size_t valpos = tempstr.find_first_of(":=", namepos+namelen);
        if (string::npos == valpos) break;

        for (++valpos ;valpos < len && ' ' == pchstr[valpos]; ++valpos) // skip space
        {
        }

        if (valpos >= len) break;

        // 2016-06-14 ���Ӵ���˫���ŵĹ���
        size_t valend;
        if ('\"' == pchstr[valpos])
        {
            bool found = false;
            ++valpos;
            valend = valpos;
            for (; valend < len; ++valend)
            {
                if ('\"' == pchstr[valend])
                {
                    found = true;
                    break;
                }
                
                if ('\\' == pchstr[valend])
                {
                    ++valend;
                }
            }

            if (!found)
            {
                --valpos;
                valend = string::npos;
            }
        }
        else
        {
            valend = tempstr.find_first_of(" ,}\r\n", valpos);
        }

        // ��ȡ���ֵ
        if (string::npos == valend)
        {
            ostr = tempstr.substr(valpos);
        }
        else
        {
            ostr = tempstr.substr(valpos, valend - valpos);
        }

        ret = 0;
    }
    while (0);

    return ret;
}

void StrParse::AdjustPath( string& path, bool useend, char dv /*= '/'*/ )
{
	if (!path.empty())
	{
		size_t len = path.length();
		if (useend)
		{
			if (dv != path.at(len-1))
			{
				path.append(1, dv);
			}
		}
		else
		{
			while (dv == path.at(len-1))
			{
				path.erase(len-1);
				len -= 1;
			}
		}
	}
}

bool StrParse::IsCharacter(const string& str, bool inc_digit)
{
    bool ret = true;
    size_t len = str.length();
    const char* pstr = str.c_str();

    for (size_t i = 0; i < len; ++i)
    {
        char ch = pstr[i];
        if (!( (ch >= 'a' && ch <= 'z') ||
             (ch >= 'A' && ch <= 'Z') ||
             (inc_digit && ch >= '0' && ch <= '9') ))
        {
            ret = false;
            break;
        }
    }

    return ret;
}

bool StrParse::IsNumberic(const string& str)
{
    bool ret = true;
    size_t len = str.length();
    const char* pstr = str.c_str();

    for (size_t i = 0; i < len; ++i)
    {
        char ch = pstr[i];
        if ( !( ch >= '0' && ch <= '9') )
        {
            ret = false;
            break;
        }
    }

    return ret;
}

int StrParse::AppendFormat(string& ostr, const char* fmt, ...)
{
    va_list ap;
    int ret;
    unsigned int n;
    char buff[512] = {0};

    va_start(ap, fmt);
    n = vsnprintf(buff, sizeof(buff)-1, fmt, ap);
    va_end(ap);

    ostr.append(buff);
    ret = (n>=sizeof(buff));

    return ret;
}

string StrParse::Format( const char* fmt, ... )
{
    va_list ap;
    char buff[512] = {0};

    va_start(ap, fmt);
    vsnprintf(buff, sizeof(buff)-1, fmt, ap);
    va_end(ap);

    return buff;
}

string StrParse::Itoa( int n )
{
    return Format("%d", n);
}

