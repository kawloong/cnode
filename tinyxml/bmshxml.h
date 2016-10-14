/******************************************************************* 
 *  summery:     �ṩ��ݵķ�������tinyxml
 *  author:      hejl
 *  date:        2016-04-14
 *  description: ��Ч����string(NULL), element=NULL, strdup(NULL)�ж�
 ******************************************************************/ 

#ifndef _BMSH_XML_H_
#define _BMSH_XML_H_
#include <string>
#include "tinyxml.h"

using std::string;
class TiXmlElement;

/*****************************
�ؼ���˵��:
text      :   xml��ǩ�����ݲ��� <tag>���ݲ���</tag>
attr      :   xml��ǩ�����Բ��� <tag attr=���Բ���></tag>
child_text:   �ӱ�ǩ�����ݲ��� <tag> <item>����</item> </tag>
string    :   ����std::string����
strdup    :   ������strdup�ѷ���õ���ָ��, �������free�ͷ�
strptr    :   ����xmldocument�����ַ���λ��,�����߲���free
*****************************/

class BmshXml
{
public: 
    // ��ȡ<element>�������ַ���
    static string text_string(TiXmlElement* element);
    static bool text_string(string& outstr, TiXmlElement* element);
    static char*  text_strdup(TiXmlElement* element, bool notNull = false);
    static const char* text_strptr(TiXmlElement* element, bool notNull = false);

    // ��ȡ<element>�������ַ���
    static string attr_string(TiXmlElement* element, const char* attr);
    static bool attr_string(string& outstr, TiXmlElement* element, const char* attr);
    static char*  attr_strdup(TiXmlElement* element, const char* attr, bool notNull = false);
    static const char* attr_strptr(TiXmlElement* element, const char* attr, bool notNull = false);

    // ��ȡ�ӱ�ǩ��������ı��ַ���
    static string child_text_string(TiXmlElement* element, const char* item);
    static bool child_text_string(string& outstr, TiXmlElement* element, const char* item);
    static char*  child_text_strdup(TiXmlElement* element, const char* item, bool notNull = false);
    static const char* child_text_strptr(TiXmlElement* element, const char* item, bool notNull = false);


    // ��ȡ<element>����������ֵ
    static int text_int(TiXmlElement* element, int def = 0);
    // ��ȡ<element>����������ֵ
    static int attr_int(TiXmlElement* element, const char* attr, int def = 0);
    // ��ȡ�ӱ�ǩ��������ı�����ֵ
    static int child_text_int(TiXmlElement* element, const char* item, int def = 0);

};

#define ISEMPTY_STR(pstr) (NULL==pstr||string(pstr).empty())

#endif
