#ifndef _BASE_64_H_
#define _BASE_64_H_
#include <string>

class Base64
{
public:
    /* @summey: bas64���뺯��
    ** @param: inbuf [in] ����������ʼ��ַ
    ** @param: size [in] ��������size,�ֽڵ�λ
    ** @param: str [out] ���ձ���������ݷ��ض�ָ��,�������free()�ͷ�
    ** @return: �ɹ����������������ݵ��ֽ���, ʧ�ܷ���С��0
    **/
    static int Encode(const void* inbuf, int size, char** str);

    /* @summey: bas64���뺯��
    ** @param: str [in] ����������ʼ��ַ
    ** @param: size [in] ��������size,�ֽڵ�λ
    ** @param: outbuf [out] ���ս���������ݷ��ض�ָ��,�������free()�ͷ�
    ** @return: �ɹ����������������ݵ��ֽ���, ʧ�ܷ���С��0
    **/
    static int Decode(const char* str, int size, void** outbuf);


    /* @summey: ����string��bas64���뺯��
    ** @param: iobuff [in/out] �������������,���base64������ַ�������
    ** @return: �ɹ����������������ݵ��ֽ���, ʧ�ܷ���С��0
    **/
    static int Encode(std::string& iobuff);

    /* @summey: ����string��bas64���뺯��
    ** @param: iobuff [in/out] ������ܵ��ַ�������,���base64�����Ķ���������
    ** @return: �ɹ����������������ݵ��ֽ���, ʧ�ܷ���С��0
    **/
    static int Decode(std::string& iobuff);
};

#endif