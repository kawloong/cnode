
#ifndef _INTERFACE_H_
#define _INTERFACE_H_
#include <string>

#define interface struct
using std::string;

/* @: httpԶ�����󷵻� */
interface IHttpClientBack
{
    virtual int clireq_callback(int result, const string& body, const string& errmsg) = 0;
};

#endif
