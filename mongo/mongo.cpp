#include <bson.h>
#include <bcon.h>

#include "mongo.h"

#define  DESTROY_BSON_PTR(pbson) { if(pbson) {bson_destroy(pbson); pbson=NULL;} }

int Mongo::ping( string* err /*= NULL*/ )
{
    return 0;
}

const char* Mongo::name( void )
{
    return 0;
}

string Mongo::geterr( void )
{
#if 1
    return m_operror;
#else 
    string stre;
    const bson_t* bson = mongoc_collection_get_last_error(m_coll);
    char* perr = bson_as_json(bson, NULL);
    if (perr)
    {
        stre = perr;
        bson_free(perr);
    }

    return stre;
#endif
}

int Mongo::insert( bson_t* bson, bool save )
{
    int ret = 0;
    bson_error_t berr;
    bool result;

    if (save)
    {
        result = mongoc_collection_save(m_coll, bson, NULL, &berr);
    }
    else
    {
        result = mongoc_collection_insert(m_coll, MONGOC_INSERT_NONE, bson, NULL, &berr);
    }

    if (!result)
    {
        m_operror = berr.message;
        ret = EMOG_OP_FAIL;
    }

    return ret;
}

int Mongo::insert( const string& json, bool save )
{
    int ret;
    bson_t* bson = NULL;

    ret = json2bson(&bson, json);
    if (0 == ret)
    {
        ret = insert(bson, save);
    }

    DESTROY_BSON_PTR(bson);
    return ret;
}

int Mongo::find( string& rs, const string& queryjson, const string& fieldjson, int limit, int skip )
{
    int ret;
    list<string> lstresult;

    ret = find(lstresult, queryjson, fieldjson, limit, skip);
    if (0 == ret && !lstresult.empty())
    {
        bool bfirst = true;
        rs = "[ ";
        list<string>::const_iterator itr = lstresult.begin();
        for (; itr != lstresult.end(); ++itr)
        {
            bfirst = bfirst? false: (rs += ", " , false);
            rs += (*itr);
        }

        rs += " ]";
    }

    return ret;
}

int Mongo::find( list<string>& vrs, const string& queryjson, const string& fieldjson, int limit, int skip )
{
    int ret;
    bson_t* qbson = NULL;
    bson_t* fbson = NULL;
    
    do 
    {
        ret = json2bson(&qbson, queryjson);
        if(ret) break;
        if (NULL == qbson)
        {
            m_operror = "query null";
            ret = EMOG_PARAM_INPUT;
            break;
        }

        ret = json2bson(&fbson, fieldjson);
        if(ret) break;

        ret = find(vrs, qbson, fbson, limit, skip);
    }
    while(0);

    DESTROY_BSON_PTR(qbson);
    DESTROY_BSON_PTR(fbson);

    return ret;
}

int Mongo::find( list<string>& vrs, const bson_t* query, const bson_t* field, int limit, int skip )
{
    int ret = 0;
    mongoc_cursor_t* cursor;
    bson_error_t error;
    const bson_t* doc = NULL;

    if (NULL == query)
    {
        return EMOG_PARAM_INPUT;
    }

    cursor = mongoc_collection_find(m_coll, MONGOC_QUERY_NONE, skip, limit, 0, query, field, NULL);

    while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc))
    {
        if (!doc) continue;
        char* str = bson_as_json(doc, NULL);
        if (str)
        {
            vrs.push_back(str);
            bson_free(str);
        }
    }

    if (mongoc_cursor_error(cursor, &error))
    {
        m_operror = error.message;
        ret = EMOG_OP_FAIL;
    }

    mongoc_cursor_destroy(cursor);
    return ret;
}

int Mongo::update( const string& queryjson, const string& setjson, bool upsert, bool mutiset )
{
    int ret;
    bson_t* bquery = NULL;
    bson_t* bset = NULL;

    do
    {
        if (queryjson.empty() || setjson.empty())
        {
            ret = EMOG_PARAM_INPUT;
            break;
        }

        ret = json2bson(&bquery, queryjson);
        if(ret) break;
        if (NULL == bquery)
        {
            m_operror = "query param null";
            ret = EMOG_PARAM_INPUT;
            break;
        }

        ret = json2bson(&bset, setjson);
        if(ret) break;
        if (NULL == bset)
        {
            m_operror = "set param null";
            ret = EMOG_PARAM_INPUT;
            break;
        }

        ret = update(bquery, bset, upsert, mutiset);
    }
    while (0);

    DESTROY_BSON_PTR(bquery);
    DESTROY_BSON_PTR(bset);
    return ret;
}

int Mongo::update( const bson_t* bquery, const bson_t* bset, bool upsert, bool mutiset )
{
    int ret = 0;
    bson_error_t error;

    if (NULL == bquery)
    {
        ret = EMOG_PARAM_INPUT;
        m_operror = "query param null";
    }
    else
    {
        int flag = MONGOC_UPDATE_NONE;
        if (upsert)
        {
            flag |= MONGOC_UPDATE_UPSERT;
        }

        if (mutiset)
        {
            flag |= MONGOC_UPDATE_MULTI_UPDATE;
        }

        if (!mongoc_collection_update(m_coll, mongoc_update_flags_t(flag), bquery, bset, NULL, &error))
        {
            m_operror = error.message;
            ret = EMOG_OP_FAIL;
        }
    }

    return ret;
}

int Mongo::remove( const string& queryjson, bool onlyone )
{
    int ret;
    bson_t* bquery = NULL;

    ret = json2bson(&bquery, queryjson);
    if (bquery)
    {
        ret = remove(bquery, onlyone);
    }

    DESTROY_BSON_PTR(bquery);
    return ret;
}

int Mongo::remove( const bson_t* bquery, bool onlyone )
{
    int ret = 0;

    if (NULL == bquery)
    {
        ret = EMOG_PARAM_INPUT;
        m_operror = "query param null";
    }
    else
    {
        int flag = MONGOC_REMOVE_NONE;
        bson_error_t error;

        if (onlyone)
        {
            flag |= MONGOC_REMOVE_SINGLE_REMOVE;
        }

        if (!mongoc_collection_remove(m_coll, mongoc_remove_flags_t(flag), bquery, NULL, &error))
        {
            m_operror = error.message;
            ret = EMOG_OP_FAIL;
        }
    }

    return ret;
}

int Mongo::findAndModify( string& rs, const string& qjson, const string& sort, const string& update,
                         const string& field, bool rm, bool upsert, bool bnew )
{
    int ret;
    bson_t* bquery = NULL;
    bson_t* bsort  = NULL;
    bson_t* bupdate = NULL;
    bson_t* bfield = NULL;

    do 
    {
        ret = json2bson(&bquery, qjson);
        if (ret) break;
        ret = json2bson(&bsort, sort);
        if (ret) break;
        ret = json2bson(&bupdate, update);
        if (ret) break;
        ret = json2bson(&bfield, field);
        if (ret) break;

        ret = findAndModify(rs, bquery, bsort, bupdate, bfield, rm, upsert, bnew);
    }
    while (0);

    DESTROY_BSON_PTR(bquery);
    DESTROY_BSON_PTR(bsort);
    DESTROY_BSON_PTR(bupdate);
    DESTROY_BSON_PTR(bfield);

    return ret;
}

// 查询并更新/删除
int Mongo::findAndModify( string& rs, const bson_t* bquery, const bson_t* sort, const bson_t* update,
                          const bson_t* field, bool rm, bool upsert, bool bnew )
{
    int ret = 0;
    bson_error_t berr;
    bson_t reply;

    if (NULL == bquery)
    {
        ret = EMOG_PARAM_INPUT;
        m_operror = "query param null";
    }
    else
    {
        if (!mongoc_collection_find_and_modify(m_coll, bquery, sort, update, field, rm, upsert, bnew, &reply, &berr))
        {
            m_operror = berr.message;
            ret = EMOG_OP_FAIL;
        }
        else
        {
            ret = 0;
            char* str = bson_as_json(&reply, NULL);
            if (str)
            {
                rs = str;
                bson_free(str);
            }
        }
    }

    bson_destroy(&reply);

    return ret;
}


// 做个bson_t -> string的转化;
// @remark: 返回的bson需调用者释放bson_destroy()
int Mongo::json2bson( bson_t** bson, const string& strjson )
{
    int ret = 0;
    bson_error_t berr;

    if (!strjson.empty())
    {
        *bson = bson_new_from_json((const uint8_t*)strjson.c_str(), strjson.length(), &berr);
        if (NULL == bson)
        {
            m_operror = berr.message;
            ret = EMOG_JSON_FORMAT;
        }
    }
    else
    {
        m_operror = "param null";
    }

    return ret;
}