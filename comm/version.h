/*-------------------------------------------------------------------------
fileName     : version.h
Description  : �汾��ͷ�ļ�
Functions    : 
Modification :
-------------------------------------------------------------------------*/
#ifndef _VERSION_H__
#define _VERSION_H__
#include <iostream>

using std::cout;
using std::endl;

//����汾��Ϣ
void version_output(const char* name, const char* ver)
{
    //����汾��Ϣ
    cout << "************************************************" << endl;
    cout << "    program : " << name << endl;
    cout << "    version : " << ver << endl;
	cout << "    Build   : "<< __DATE__ " " __TIME__ << endl;		
	cout << "************************************************" << endl;	
}

#endif //_COMMON_VERSION_H__

