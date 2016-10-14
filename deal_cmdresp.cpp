#include <strings.h>
#include <event2/buffer.h>
#include "errdef.h"
#include "rapidjson/json.hpp"
#include "deal_cmdresp.h"


DealCmdResp::DealCmdResp( struct evhttp_request* req, ThreadData* tdata ): DealBase(req, tdata)
{

}

int DealCmdResp::Run( struct evhttp_request* req, ThreadData* tdata )
{
    int ret;
    const char* err_reason = "";
    char* bodydata = NULL;
    string encoding;

    do 
    {
        evbuffer* bodybuff = evhttp_request_get_input_buffer(req);
        size_t bodylen = bodybuff? evbuffer_get_length(bodybuff): 0;
        err_reason = "body no data";
        IFBREAK_N(bodylen<=0, ERR_PARAMETER);

        ret = GetHeadStr(encoding, req, "Content-Encoding");
        IFBREAK_N(ret, ERR_HEADER);
        if (0 == strcasecmp(encoding.c_str(), "gzip"))
        {
            LOGWARN("CMDRSPRUN| msg=not implement gzip"); // 暂未实现gzip解压缩
            // to do gunzip process
        }
        else if (encoding.empty())
        {
        }
        else
        {
            err_reason = "unknow content-encoding";
            LOGERROR("CMDRSPRUN| msg=%s (%s)", err_reason, encoding.c_str());
            ret = ERR_ENCODING;
            break;
        }

        {
            Document doc;
            bodydata = (char*)malloc(bodylen + 1);
            err_reason = "malloc fail";
            IFBREAK_N(NULL == bodydata, ERR_SYSTEM);

            ret = doc.ParseInsitu(bodydata).HasParseError();
            err_reason = "parse body fail";
            IFBREAK_N(ret, ERR_JSON_FORMAT);
        }
    }
    while (0);

    if (ret) // 参数不合法的响应
    {
        ret = SendRespondFail(req, err_reason, ret);
        //IFDELETE(deal);
    }

    IFFREE(bodydata);

    return 0;
}

int DealCmdResp::run_task( void )
{
    return 0;
}

void DealCmdResp::resume( int result )
{

}
