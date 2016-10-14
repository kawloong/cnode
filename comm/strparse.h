/******************************************************************* 
 *  summery:        �ַ�������������
 *  author:         hejl
 *  date:           2016-05-17
 *  description:    
 ******************************************************************/

#ifndef _STRPARSE_H_
#define _STRPARSE_H_
#include <vector>
#include <map>
#include <string>

using namespace std;


class StrParse
{
public:
    // ��dv��Ϊ�ָ������str�����õ��Ľ����ŵ�data����
    static int SpliteInt(vector<int>& data, const string& str, char dv, int nildef = 0);
    static int SpliteStr(vector<string>& data, const string& str, char dv);

    // ����http��url����
    static int SpliteQueryString(map<string, string>& outPar, const string& qstr);
    // ��json����, srcȫ��ת����Сд����;
    static int PickOneJson(string& ostr, const string& src, const string& name);

    // ����·��β���ַ�
	static void AdjustPath(string& path, bool useend, char dv = '/');

    // �ж��ַ����Ƿ񲻺������ַ�
    static bool IsCharacter(const string& str, bool inc_digit = true);
    static bool IsNumberic(const string& str);

    // �ַ�����ʽ��,����sprintf����
    static int AppendFormat(string& ostr, const char* fmt, ...);
    static string Format(const char* fmt, ...);
    static string Itoa(int n);
};


#endif