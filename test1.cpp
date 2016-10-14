#include <stdio.h>
#include "comm/public.h"
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
        string tmp;
        string quer("{ }");
        string field; //("{\"name\"}");

        ret = MongoConnPoolAdmin::Instance()->GetConnect(mgo3, g_test_name); // test pool
        ret = MongoConnPoolAdmin::Instance()->GetConnect(mgo2, g_test_name); // test pool

        {
            if (0 == ret)
            {
                ret = mgo1->insert("{ \"kk11\": \"vv11\", \"kk22\": \"vv22\" } ", false);
                LOGDEBUG("insert result=%d", ret);
            }
        }

        MongoConnPoolAdmin::Instance()->showPoolStatus(g_test_name);

		ret = mgo1->find(tmp, quer, field, 2, 0);
        LOGDEBUG("result set:%s\n", tmp.c_str());
        LOGDEBUG("mgo1->find result %d, err=%s", ret, mgo1->geterr().c_str());

        tmp = "{ \"$set\": { \"name\": \"xx\" } }";

        ret = mgo1->update(quer, tmp, false, false);
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