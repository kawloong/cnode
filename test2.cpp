/*
        test idmanage class
*/

#include "comm/public.h"
#include "idmanage.h"


int main(int argc, char* argv[])
{
    int ret;
    string objid;

    ret = IdMgr::Instance()->init(0x100, 7);
    LOGDEBUG("IdMgr::init() return %d", ret);

    for (int i = 0; i < 10; ++i)
    {
        ret = IdMgr::Instance()->getObjectId(objid);
        printf("ret:%d| %s\n", ret, objid.c_str());
        objid.clear();
        IFBREAK(ret);
    }
    

    IdMgr::Instance()->uninit();
    return 0;
}
