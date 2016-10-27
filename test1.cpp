#include <stdio.h>
#include "comm/public.h"
#include "comm/strparse.h"
#include "mongo/mongopooladmin.h"

const char g_test_name[] = "cnode_pool";
const string g_tjson("{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}");

int main(int argc, char* argv[])
{
    printf("mongo test begin:\n");

    //configItemInit();

    Mongo* mgo1 = NULL;
    Mongo* mgo2 = NULL;
    Mongo* mgo3 = NULL;

    MongoConnPoolAdmin::Instance()->LoadPoolFromFile("/etc/bmsh_mongo_connpoll_conf.json");
    
    int ret = MongoConnPoolAdmin::Instance()->GetConnect(mgo1, g_test_name);
    if (0 == ret)
    {
        string tmp, tmp2;
        string quer("{ \"$query\":{ \"kk11\": \"1234\"}, \"$orderby\":{ \"kk22\":-1 } }");
        //string quer("{  \"kk1\": \"vv1\" }");
        string field("{ \"_id\": 1, \"kk22\":1, \"kk11\":1 }");

        ret = MongoConnPoolAdmin::Instance()->GetConnect(mgo3, g_test_name); // test pool
        ret = MongoConnPoolAdmin::Instance()->GetConnect(mgo2, g_test_name); // test pool

//         {
//             if (0 == ret)
//             {
//                 ret = mgo1->insert("{ \"kk11\": \"1234\", \"kk22\": \"5678\" }", false);
//                 LOGDEBUG("insert result=%d, err=%s", ret, mgo1->geterr().c_str());
//             }
//         }

        MongoConnPoolAdmin::Instance()->showPoolStatus(g_test_name);

		ret = mgo1->find(tmp, quer, field, 1, 0);
        LOGDEBUG("result set:%s\n", tmp.c_str());
        if (!tmp.empty())
        {
            StrParse::PickOneJson(tmp2, tmp, "kk11");
            printf("PickOneJson -> %s\n", tmp2.c_str());
        }
        LOGDEBUG("mgo1->find result %d, err=%s", ret, mgo1->geterr().c_str());

        tmp = "{ \"$set\": { \"xxml\": \"989898989\" } }";

        // "{\"kk11\":\"1234\", }"
        ret = mgo1->update("{ \"xxml\":{ \"$exist\": true} }", tmp, false, false);
        LOGDEBUG("update ret=%d| err=%s", ret, mgo1->geterr().c_str());


        ret = mgo1->remove("{ \"name\": \"aaaaaaa\" } ", true);
        LOGDEBUG("remove ret=%d| err=%s", ret, mgo1->geterr().c_str());

        // ´òÓ¡SQL³ØÔËÐÐ×´Ì¬
        MongoConnPoolAdmin::Instance()->ReleaseConnect(mgo2);
        MongoConnPoolAdmin::Instance()->showPoolStatus(g_test_name);

        MongoConnPoolAdmin::Instance()->ReleaseConnect(mgo1);
        ret = MongoConnPoolAdmin::Instance()->ReleaseConnect(mgo3);
    }
    else
    {
        printf("GetConnect Err=%d\n", ret);
    }

	MongoConnPoolAdmin::Instance()->DestroyPool(NULL);

    
	return 0;
}