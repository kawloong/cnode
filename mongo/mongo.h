#ifndef _MONGO_H_
#define _MONGO_H_
#include <map>
#include <list>
#include <string>
#include <mongoc.h>

using std::map;
using std::list;
using std::string;

/* mongoc API接口帮助参考
issue help for libmongoc.so http://mongoc.org/libmongoc/1.4.0/
*/

// MongoPool所有调用的错误返回值定义
enum _mongo_err_
{
    ERR_INNER_SYS = 300, // 内部错误
    ERR_PARAM_INPUT,
    ERR_LOADCONF,
    ERR_INIT_FAIL,
    ERR_DUP_INIT,
    ERR_INVALID_NAME,
    ERR_OP_FAIL,
    ERR_JSON_FORMAT,
};

// 封装mongodb的增删改查的相关操作
class Mongo
{
public:
    int insert(const string& json, bool save = false);
    int insert(bson_t* bson, bool save = false);

    // 查询, limit=0时不限数量, skip是开头跳过的记录数
    int find(string& rs, const string& queryjson, const string& fieldjson, int limit, int skip);
    int find(list<string>& vrs, const string& queryjson, const string& fieldjson, int limit, int skip);
    int find(list<string>& vrs, const bson_t* query, const bson_t* field, int limit, int skip);

    //查询并更新
    int findAndModify(string& rs, const string& queryjson, const string& sort,
        const string& update, const string& field, bool rm, bool upsert, bool bnew);
    int findAndModify(string& rs, const bson_t* bquery, const bson_t* bsort, 
        const bson_t* bupdate, const bson_t* bfield, bool rm, bool upsert, bool bnew);

    int update(const string& queryjson, const string& setjson, bool upsert, bool mutiset);
    int update(const bson_t* bquery, const bson_t* bset, bool upsert, bool mutiset);

    int remove(const string& queryjson, bool onlyone);
    int remove(const bson_t* bquery, bool onlyone);

private:
    int json2bson(bson_t** bson, const string& strjson);

public:
    int ping(string* err = NULL);
    const char* name(void);
    string geterr(void);


    friend class MongoConnPool;
    friend class MongoConnPoolAdmin;

private:
    void* parent;
    bool inpool; // 池中长连接
    int begtime;
    string m_operror;

    mongoc_client_t* m_client;
    mongoc_collection_t* m_coll;
};

#endif //_MONGO_H_
