#include <cstdio>
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON

using namespace rapidjson;

const char json[] = 
//" { \"hello\" : \"world\", \"xx\":[\"a\", \"b\"], \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] }";
"{\"module\":\"wifidog_wifi\",\"data\":{\"time_sec\":\"1477294357\",\"time_usec\":\"408492\",\"mac\":\"d4:22:3f:63:ff:6b\",\"status\":0}}";


int main(int, char*[])
{
	printf("Original JSON:\n %s\n", json);
	Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.
	
	char buffer[sizeof(json)];
    memcpy(buffer, json, sizeof(json));
    if (document.ParseInsitu(buffer).HasParseError())return 1;

	printf("\nParsing to document succeeded. size=%u\n", sizeof(json));
	printf("t = %d\n", document["t"].GetBool());
	printf("gettype=%d\n", document["t"].GetType());
	
	Value& xx = document["xx"];
	const Value& aa = document["a"];
	
	Value cpa(document["a"], document.GetAllocator());
	cpa.PushBack("999", document.GetAllocator());
	StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    //cpa.Accept(writer);    // Accept() traverses the DOM and generates Handler events.
	document["xx"].Accept(writer);
    printf("cpa is:\n%s\n", sb.GetString());
	
	return 0;
}
