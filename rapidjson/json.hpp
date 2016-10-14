#ifndef _JSON_HPP_
#define _JSON_HPP_
#include <string>
#include "document.h"
#include "rapidjson.h"
#include "prettywriter.h"

using namespace rapidjson;
using std::string;

	
class Rjson
{
public:
static int GetValue(const Value** value, const char* name, const Value* parent)
{
	if (parent && name && parent->IsObject())
	{
		Value::ConstMemberIterator itr = parent->FindMember(name);
		if (itr != parent->MemberEnd())
		{
			*value = &(itr->value);
			return 0;
		}
	}
	
	return -1;
}

static int GetValue(const Value** value, int idx, const Value* parent)
{
	if (parent && idx >= 0 && parent->IsArray() && idx < (int)parent->Size())
	{
		*value = &( (*parent)[idx]);
		return 0;
	}

	return -1;
}

template<typename T>
static int GetObject(const Value** value, T t, const Value* parent)
{
	if (0 == GetValue(value, t, parent) && (*value)->IsObject())
	{
		return 0;
	}
	
	*value = NULL;
	return -1;
}

template<typename T>
static int GetArray(const Value** value, T t, const Value* parent)
{
	if (0 == GetValue(value, t, parent) && (*value)->IsArray())
	{
		return 0;
	}
	
	*value = NULL;
	return -1;
}

template<typename T>
static int GetStr(string& str, T t, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t, parent) && value->IsString())
	{
		str = value->GetString();
		return 0;
	}
	
	return -1;
}

template<typename T>
static int GetInt(int& n, T t, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t, parent) && value->IsInt())
	{
		n = value->GetInt();
		return 0;
	}
	
	return -1;
}

///////////////////////////////////////////

template<typename T1, typename T2>
static int GetValue(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	const Value* tmpv = NULL;
	int ret = GetValue(&tmpv, t1, parent);
	if (0 == ret)
	{
		return GetValue(value, t2, tmpv);
	}
	
	return -1;
}

template<typename T1, typename T2>
static int GetObject(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	if (0 == GetValue(value, t1, t2, parent) && (*value)->IsObject())
	{
		return 0;
	}
	*value = NULL;
	return -1;
}

template<typename T1, typename T2>
static int GetArray(const Value** value, T1 t1, T2 t2, const Value* parent)
{
	if (0 == GetValue(value, t1, t2, parent) && (*value)->IsArray())
	{
		return 0;
	}
	*value = NULL;
	return -1;
}

template<typename T1, typename T2>
static int GetStr(string& str, T1 t1, T2 t2, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t1, t2, parent) && value->IsString())
	{
		str = value->GetString();
		return 0;
	}
	
	return -1;
}

template<typename T1, typename T2>
static int GetInt(int& n, T1 t1, T2 t2, const Value* parent)
{
	const Value* value = NULL;
	if (0 == GetValue(&value, t1, t2, parent) && value->IsInt())
	{
		n = value->GetInt();
		return 0;
	}
	
	return -1;
}

///////////////////////////////////////////////

static int ToString(string& str, const Value* parent)
{
	if (parent)
	{
		StringBuffer sb;
		PrettyWriter<StringBuffer> writer(sb);
		parent->Accept(writer);
		str = sb.GetString();
		return 0;
	}

	return -1;
}

};

#endif
