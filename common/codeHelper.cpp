#include "codeHelper.h"
#include <assert.h>

#include <curl/curl.h>
#include <memory.h>

#include "speech/token.h"
#include "speech/ttscurl.h"
#include "speech/ttscurl.c"
#include "../database/config/inirw.h"






// 设置APPID/AK/SK
const std::string app_id = ""; //"你的 App ID";
const std::string api_key = "7ZM7qRbFdrBF701okoSXyY5L";
const std::string secret_key = "uEAf5uIe0H3NDeDlA2HKe6BKalh65G4s";

const char TTS_SCOPE[] = "audio_tts_post";
const char API_TTS_URL[] = "http://tsn.baidu.com/text2audio"; // �?改为https
const int ENABLE_CURL_VERBOSE = 0;

// 获取access_token所需要的url
const std::string access_token_url = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials";

codeHelper *codeHelper::m_pInstance = NULL;

/**
 * curl发送http请求调用的回调函数，回调函数�?对返回的json格式的body进�?�了解析，解析结果储存在result�?
 * @param 参数定义见libcurl库文�?
 * @return 返回值定义�?�libcurl库文�?
 */
static size_t token_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    // 获取到的body存放在ptr�?，先将其�?�?为string格式
    std::string s((char *)ptr, size * nmemb);

    printf("token_callback=%s\n", s.c_str());

    char token[MAX_TOKEN_SIZE];

    obtain_json_str(s.c_str(), "access_token", token, MAX_TOKEN_SIZE);
    printf("token=%s\n", token);

    return size * nmemb;
}

// libcurl 返回回调
size_t cbwritefunc(void *ptr, size_t size, size_t nmemb, char **result)
{
    size_t result_len = size * nmemb;
    if (*result == NULL)
    {
        *result = (char *)malloc(result_len + 1);
    }
    else
    {
        *result = (char *)realloc(*result, result_len + 1);
    }
    if (*result == NULL)
    {
        printf("realloc failure!\n");
        return 1;
    }
    memcpy(*result, ptr, result_len);
    (*result)[result_len] = '\0';
    // printf("buffer: %s\n", *result);
    return result_len;
}

unsigned char codeHelper::ToHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

unsigned char codeHelper::FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        y = x - '0';
    else
        assert(0);
    return y;
}
std::string codeHelper::UrlDecode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
            strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = this->FromHex((unsigned char)str[++i]);
            unsigned char low = this->FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else
            strTemp += str[i];
    }
    return strTemp;
}
string codeHelper::notYinghao(const char *str)
{
    std::string nnstr(str);
    string temp;
    if (nnstr.length() > 1)
    {
        temp = nnstr.substr(1);
    }

    return temp.substr(0, temp.length() - 1);
}

string codeHelper::createRequestEntity(const string &phone, const string &status, const string &type, const string &content, const string &id, const string &recordId)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *item = cJSON_CreateObject();
    cJSON *next = cJSON_CreateObject();
    cJSON *arrItem = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();

    cJSON_AddItemToObject(item, "SYS_CODE", cJSON_CreateString("5001"));                             //根节点下添加
    cJSON_AddItemToObject(item, "USERNAME", cJSON_CreateString("GZPOST"));                           //根节点下添加
    cJSON_AddItemToObject(item, "PASSWORD", cJSON_CreateString("4BD065C44151F8F6F65C18A38FFE5ECA")); //根节点下添加
    cJSON_AddItemToObject(item, "FUNC_CODE", cJSON_CreateString("MOBP026"));                         //根节点下添加
    cJSON_AddItemToObject(root, "HEAD", item);                                                       //根节点下添加

    cJSON_AddItemToObject(arrItem, "phoneNumber", cJSON_CreateString(phone.c_str()));
    cJSON_AddItemToObject(arrItem, "status", cJSON_CreateString(status.c_str()));
    cJSON_AddItemToObject(arrItem, "type", cJSON_CreateString(type.c_str()));
    cJSON_AddItemToObject(arrItem, "content", cJSON_CreateString(content.c_str()));
    cJSON_AddItemToObject(arrItem, "recordId", cJSON_CreateString(recordId.c_str()));
    cJSON_AddItemToObject(arrItem, "id", cJSON_CreateString(id.c_str()));

    cJSON_AddItemToObject(arr, "data", arrItem); //semantic节点下添加item节点
    cJSON_AddItemToObject(next, "data", arr);    //semantic节点下添加item节点
    cJSON_AddItemToObject(root, "BODY", next);   //semantic节点下添加item节点
    printf("%s\n", cJSON_Print(root));

    char *ret = cJSON_Print(root);
    if (ret == NULL)
    {
        return "";
    }
    else
    {
        return ret;
    }
}

string codeHelper::sentiment_classifyRequesst(const string &text)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "text", cJSON_CreateString(text.c_str())); //根节点下添加
    char *ret = cJSON_Print(root);

    CURL *pCurl = NULL;
    CURLcode res;
    char *response = NULL;

    pCurl = curl_easy_init();

    if (NULL != pCurl)
    {
        // 设置超时时间�?1�?
        curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 5);

        // First set the URL that is about to receive our POST.
        // This URL can just as well be a
        // https:// URL if that is what should receive the data.
        curl_easy_setopt(pCurl, CURLOPT_URL, "http://www.forevermaybe.xyz/sentiment_classify");

        // 设置http发送的内�?�类型为JSON
        curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json; charset=utf-8");
        // curl_slist_append(plist, "Accept:application/json");
        curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, cbwritefunc);

        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &response);

        // 设置要POST的JSON数据
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, ret);

        // Perform the request, res will get the return code
        res = curl_easy_perform(pCurl);
        // Check for errors
        if (res != CURLE_OK)
        {
            printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
        }
        char val[300];
        printf("response==%s\n", response);

        parse_positive_prob(response, "positive_prob", val, 300);
        printf("back==%s\n", val);
        return val;
    }
    curl_easy_cleanup(pCurl);
    return "OK";
}

// 组发消息接口
// "{\n" +
//                 "    \"batchName\": \"玄�?�无线�?�技组发测试\",\n" +
//                 "    \"items\": [\n" +
//                 "        {\n" +
//                 "            \"to\": \"15811112222\",\n" +
//                 "            \"content\": \"玄�?��?�技测试�?�?01\"\n" +
//                 "        },\n" +
//                 "        {\n" +
//                 "            \"to\": \"13511112222\",\n" +
//                 "            \"content\": \"玄�?��?�技测试�?�?02\"\n" +
//                 "        }\n" +
//                 "    ],\n" +
//                 "    \"msgType\": \"sms\",\n" +
//                 "    \"bizType\": \"100\"\n" +
//                 "}";
string codeHelper::createMosRequestEntity(const string &phone,
                                          const string &batchName,
                                          const string &bizType,
                                          const string &type,
                                          const string &content)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *arrItem = cJSON_CreateArray();
    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToObject(item, "to", cJSON_CreateString(phone.c_str()));
    cJSON_AddItemToObject(item, "content", cJSON_CreateString(content.c_str()));
    cJSON_AddItemToObject(arrItem, "items", item);
    cJSON_AddItemToObject(root, "items", arrItem);

    cJSON_AddItemToObject(root, "batchName", cJSON_CreateString(batchName.c_str())); //根节点下添加
    cJSON_AddItemToObject(root, "msgType", cJSON_CreateString(type.c_str()));        //根节点下添加
    cJSON_AddItemToObject(root, "bizType", cJSON_CreateString(bizType.c_str()));     //根节点下添加
    char *ret = cJSON_Print(root);
    printf("%s\n", ret);
    if (ret == NULL)
    {
        return "";
    }
    else
    {
        return ret;
    }
}

string codeHelper::createSimnetBody(const string &text1, const string &text2)
{
    cJSON *root = cJSON_CreateObject();
    std::wstring temp1, temp2;
    const char *t1 = text1.c_str();
    const char *t2 = text2.c_str();
    this->UTF2Uni(t1, temp1);
    this->UTF2Uni(t2, temp2);
    string c1, c2;
    c1 = ws2s(temp1);
    c2 = ws2s(temp2);
    cJSON_AddItemToObject(root, "text_1", cJSON_CreateString(text1.c_str())); //根节点下添加
    cJSON_AddItemToObject(root, "text_2", cJSON_CreateString(text2.c_str())); //根节点下添加
    char *ret = cJSON_Print(root);
    printf("createSimnetBody=%s\n", ret);
    if (ret == NULL)
    {
        return "";
    }
    else
    {
        return ret;
    }
}

string codeHelper::emsCallbackRequest(const string &phone,
                                      const string &state, const string &type, const string &content, const string &recordId, const string &order_id)
{
    char http_return[4096] = {0};
    string json = createRequestEntity(phone.c_str(),
                                      state.c_str(), type.c_str(), content.c_str(), order_id.c_str(), recordId.c_str());

    if (json.empty())
    {
        return "";
    }
    inirw *configRead = inirw::GetInstance("./database.conf");
    char emsUrl[200] = {0};
    configRead->iniGetString("EMS", "callbackUrl", emsUrl, sizeof emsUrl, "0");

    CURL *pCurl = NULL;
    CURLcode res;
    char szJsonData[1024];
    memset(szJsonData, 0, sizeof(szJsonData));
    strcpy(szJsonData, json.c_str());

    //     // get a curl handle
    pCurl = curl_easy_init();

    if (NULL != pCurl)
    {
        // 设置超时时间�?1�?
        curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 1);

        // First set the URL that is about to receive our POST.
        // This URL can just as well be a
        // https:// URL if that is what should receive the data.
        curl_easy_setopt(pCurl, CURLOPT_URL, emsUrl);

        // 设置http发送的内�?�类型为JSON
        curl_slist *plist = curl_slist_append(NULL, "Content-Type: text/plain; charset=utf-8");
        curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);

        // 设置要POST的JSON数据
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, szJsonData);

        // Perform the request, res will get the return code
        res = curl_easy_perform(pCurl);
        // Check for errors
        if (res != CURLE_OK)
        {
            printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
        }
    }
    curl_easy_cleanup(pCurl);
    return "OK";
}
string codeHelper::getAliAsrTxt(const string &json)
{
    char value[1024]={0};
    char cjson[1024];
    string tmp=json;
    strcpy(cjson,tmp.c_str());

    parse_ali_asr(cjson, "text", value, sizeof value);
    return value;
}

string codeHelper::mosCallbackRequest(const string &phone,
                                      const string &batchName,
                                      const string &bizType,
                                      const string &type,
                                      const string &content)
{
    string json = createMosRequestEntity(phone.c_str(), batchName.c_str(), bizType.c_str(), type.c_str(), content.c_str());
    if (json.empty())
    {
        return "";
    }
    // esl_log(ESL_LOG_INFO, "MOS RET=%s\n", json.c_str());
    // json = "{\"batchName\": \"test\", \"items\": [{\"content\": \"ccccccuuuu\", \"to\": \"13928779610\"}], \"bizType\": \"100\", \"msgType\": \"sms\"}";
    CURL *pCurl = NULL;
    CURLcode res;
    char szJsonData[1024];
    memset(szJsonData, 0, sizeof(szJsonData));
    strcpy(szJsonData, json.c_str());

    //     // get a curl handle
    pCurl = curl_easy_init();

    if (NULL != pCurl)
    {
        // 设置超时时间�?1�?
        curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 1);

        // First set the URL that is about to receive our POST.
        // This URL can just as well be a
        // https:// URL if that is what should receive the data.
        // curl_easy_setopt(pCurl, CURLOPT_URL, mosUrl.c_str());

        // 设置http发送的内�?�类型为JSON
        curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
        curl_slist_append(plist, "Authorization:Z2RlbXM6MzcwMWJmNjM4NjA5ZjM0ZDc4OGMxZDM1ZGNkOTE4ZmQ=");
        curl_slist_append(plist, "Accept:application/json");
        curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);

        // 设置要POST的JSON数据
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, szJsonData);

        // Perform the request, res will get the return code
        res = curl_easy_perform(pCurl);
        // Check for errors
        if (res != CURLE_OK)
        {
            printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
        }
    }
    curl_easy_cleanup(pCurl);
    return "OK";
}

RETURN_CODE codeHelper::fill_config(struct tts_config *config, const char *txt)
{
    // �?写网页上申�?�的appkey �? g_api_key="g8eBUMSokVB1BHGmgxxxxxx"
    char api_key[] = "4E1BG9lTnlSeIf1NQFlrSq6h";
    // �?写网页上申�?�的APP SECRET �? $secretKey="94dc99566550d87f8fa8ece112xxxxx"
    char secret_key[] = "544ca4657ba8002e3dea3ac2f5fdd241";

    // text 的内容为"欢迎使用百度�?音合�?"的urlencode,utf-8 编码
    // �?以百度搜�?"urlencode"
    // char text[2000] = "欢迎使用百度�?�?";
    char text[2000];
    strcpy(text, txt);
    // 发音人选择, 0为普通女声，1为普通男生，3为情感合�?-度逍遥�?4为情感合�?-度丫�?，默认为�?通女�?
    int per = 0;
    // �?速，取�?0-9，默认为5�?�?�?
    int spd = 5;
    // #音调，取�?0-9，默认为5�?�?�?
    int pit = 5;
    // #音量，取�?0-9，默认为5�?音量
    int vol = 5;
    // 下载的文件格�?, 3：mp3(default) 4�? pcm-16k 5�? pcm-8k 6. wav
    int aue = 6;

    // 将上述参数填�?config�?
    snprintf(config->api_key, sizeof(config->api_key), "%s", api_key);
    snprintf(config->secret_key, sizeof(config->secret_key), "%s", secret_key);
    snprintf(config->text, sizeof(text), "%s", text);
    config->text_len = sizeof(text) - 1;
    snprintf(config->cuid, sizeof(config->cuid), "1234567C");
    config->per = per;
    config->spd = spd;
    config->pit = pit;
    config->vol = vol;
    config->aue = aue;

    // aue对应的格式，format
    const char formats[4][4] = {"mp3", "pcm", "pcm", "wav"};
    snprintf(config->format, sizeof(config->format), formats[aue - 3]);

    return RETURN_OK;
}

RETURN_CODE codeHelper::run_tts(struct tts_config *config, const char *token, const char *fileName)
{
    char params[200 + config->text_len * 9];
    CURL *curl = curl_easy_init();                                           // 需要释�?
    char *cuid = curl_easy_escape(curl, config->cuid, strlen(config->cuid)); // 需要释�?
    char *textemp = curl_easy_escape(curl, config->text, config->text_len);  // 需要释�?
    char *tex = curl_easy_escape(curl, textemp, strlen(textemp));            // 需要释�?
    curl_free(textemp);
    char params_pattern[] = "ctp=1&lan=zh&cuid=%s&tok=%s&tex=%s&per=%d&spd=%d&pit=%d&vol=%d&aue=%d";
    snprintf(params, sizeof(params), params_pattern, cuid, token, tex,
             config->per, config->spd, config->pit, config->vol, config->aue);

    char url[sizeof(params) + 200];
    snprintf(url, sizeof(url), "%s?%s", API_TTS_URL, params);
    printf("test in browser: %s\n", url);
    curl_free(cuid);
    curl_free(tex);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params);
    curl_easy_setopt(curl, CURLOPT_URL, API_TTS_URL);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20); // 连接60s超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);        // 整体请求60s超时
    char file[360];
    strcpy(file, fileName);
    struct http_result result = {1, config->format, NULL, file};
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback); // 检查头�?
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result); // 需要释�?
    curl_easy_setopt(curl, CURLOPT_VERBOSE, ENABLE_CURL_VERBOSE);
    CURLcode res_curl = curl_easy_perform(curl);

    RETURN_CODE res = RETURN_OK;
    if (res_curl != CURLE_OK)
    {
        // curl 失败
        snprintf(g_demo_error_msg, BUFFER_ERROR_SIZE, "perform curl error:%d, %s.\n", res,
                 curl_easy_strerror(res_curl));
        res = ERROR_TTS_CURL;
        printf("curl nnnn=%s\n", g_demo_error_msg);
    }
    if (result.fp != NULL)
    {
        fclose(result.fp);
    }
    curl_easy_cleanup(curl);
    return res;
}

RETURN_CODE codeHelper::run(const char *fileName, const char *txt)
{
    curl_global_init(CURL_GLOBAL_ALL);

    struct tts_config config;

    char token[MAX_TOKEN_SIZE];

    RETURN_CODE res = fill_config(&config, txt);
    printf("%s===%d\n", config.text, config.text_len);
    if (res == RETURN_OK)
    {
        // 获取token
        res = speech_get_token(config.api_key, config.secret_key, TTS_SCOPE, token);
        string tokenStr = this->notYinghao(token);
        printf("new tokenStr==%s\n", tokenStr.c_str());

        if (res == RETURN_OK)
        // if (true)
        {
            // 调用识别接口

            // string tokenStr = "24.f3cd72576451e266049c52cd7f908e1d.2592000.1546080345.282335-10854623";
            RETURN_CODE code = run_tts(&config, tokenStr.c_str(), fileName);
            printf("tttttttt==%d\n", code);
        }
    }

    return RETURN_OK;
}

std::wstring String2WString(const std::string &s)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const char *chSrc = s.c_str();
    size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t *wchDest = new wchar_t[nDestSize];
    wmemset(wchDest, 0, nDestSize);
    mbstowcs(wchDest, chSrc, nDestSize);
    std::wstring wstrResult = wchDest;
    delete[] wchDest;
    setlocale(LC_ALL, strLocale.c_str());
    return wstrResult;
}

int codeHelper::UnicodeToUTF_8(unsigned long *InPutStr, int InPutStrLen, char *OutPutStr)
{
    int i = 0, offset = 0;
    InPutStrLen = InPutStrLen / sizeof(unsigned long);
    for (i = 0; i < InPutStrLen; i++)
    {
        if (InPutStr[i] <= 0x0000007f)
        {
            OutPutStr[offset++] = (char)(InPutStr[i] & 0x0000007f);
        }
        else if (InPutStr[i] >= 0x00000080 && InPutStr[i] <= 0x000007ff)
        {
            OutPutStr[offset++] = (char)(((InPutStr[i] & 0x000007c0) >> 6) | 0x000000e0);
            OutPutStr[offset++] = (char)((InPutStr[i] & 0x0000003f) | 0x00000080);
        }
        else if (InPutStr[i] >= 0x00000800 && InPutStr[i] <= 0x0000ffff)
        {
            OutPutStr[offset++] = (char)(((InPutStr[i] & 0x0000f000) >> 12) | 0x000000e0);
            OutPutStr[offset++] = (char)(((InPutStr[i] & 0x00000fc0) >> 6) | 0x00000080);
            OutPutStr[offset++] = (char)((InPutStr[i] & 0x0000003f) | 0x00000080);
        }
        else if (InPutStr[i] >= 0x00010000 && InPutStr[i] <= 0x0010ffff)
        {
            OutPutStr[offset++] = (char)((((InPutStr[i] & 0x001c0000) >> 16) | 0x000000f0));
            OutPutStr[offset++] = (char)(((InPutStr[i] & 0x0003f000) >> 12) | 0x00000080);
            OutPutStr[offset++] = (char)(((InPutStr[i] & 0x00000fc0) >> 6) | 0x00000080);
            OutPutStr[offset++] = (char)((InPutStr[i] & 0x0000003f) | 0x00000080);
        }
    }
    return offset;
}




/****************************************************************************** 
    * function: gbk2utf8 
    * description: 实现由gbk编码到utf8编码的转�?  
    *  
    * input: utfstr,�?换后的字符串;  srcstr,待转换的字�?�串; maxutfstrlen, utfstr的最大长度�?
    * output: utfstr 
    * returns: -1,fail;>0,success 
    *  
******************************************************************************/

int gbk2utf8(char *utfstr, const char *srcstr, int maxutfstrlen)
{
    if (NULL == srcstr)
    {
        printf(" bad parameter\n");
        return -1;
    }
    //首先先将gbk编码�?�?为unicode编码
    if (NULL == setlocale(LC_ALL, "zh_CN.gbk")) //设置�?�?为unicode前的�?,当前为gbk编码
    {
        printf("setlocale bad parameter\n");
        return -1;
    }
    wchar_t wc[600];

    wstring ws = String2WString(srcstr);

    int unicodelen = ws.length();

    // int unicodelen = mbstowcs(wc, srcstr, 0); //计算�?换后的长�?  
    // if (unicodelen <= 0)
    // {
    //     printf("can not transfer!!!\n");
    //     return -1;
    // }
    wchar_t *unicodestr = (wchar_t *)calloc(sizeof(wchar_t), unicodelen + 1);
    mbstowcs(unicodestr, srcstr, strlen(srcstr)); //将gbk�?�?为unicode  

    //将unicode编码�?�?为utf8编码

    if (NULL == setlocale(LC_ALL, "zh_CN.utf8"))
    {
        printf("bad parameter\n");
        return -1;
    }
    int utflen = wcstombs(NULL, unicodestr, 0); //计算�?换后的长�?  
    if (utflen <= 0)
    {
        printf("can not transfer!!!\n");
        return -1;
    }
    else if (utflen >= maxutfstrlen) //判断空间�?否足�?  
    {
        printf("dst str memory not enough\n");
        return -1;
    }
    wcstombs(utfstr, unicodestr, utflen);
    utfstr[utflen] = 0; //添加结束�?  

    free(unicodestr);
    return utflen;
}

char *codeHelper::simnet(const char *text1, const char *text2)
{
    // aip::Nlp client(app_id,api_key,secret_key);

    // std::string text_1 = "浙富股份";

    // std::string text_2 = "万事通自考网";

    // // 调用�?文本相似�?
    // result = client.simnet(text_1, text_2, aip::null);

    // // 如果有可选参�?
    // std::map<std::string, std::string> options;
    // options["model"] = "CNN";

    // // 带参数调用短文本相似�?
    // result = client.simnet(text_1, text_2, options);
    // printf("%s\n",result.)

    // string response;
    // int code = this->get_access_token(response, "7ZM7qRbFdrBF701okoSXyY5L", "uEAf5uIe0H3NDeDlA2HKe6BKalh65G4s");
    // // wstring t1=String2WString("编码�?换器�?国人");
    // // char buf1[4000];
    // // UnicodeToUTF_8((unsigned long*)((void*)t1.c_str()),t1.length(),buf1);
    // char utf[600] = {0};
    // char utf2[600] = {0};
    // char *str = "好的";
    // printf("506pppppppppppppp====%s\n\n", str);

    // // gbk2utf8(utf, str, sizeof utf);
    // // u2g(str,strlen(str),utf,sizeof utf);
    // g2u(str, strlen(str), utf2, sizeof utf2);
    //  printf("gb2312-->unicode out=%sn",utf2);

    // string json = createSimnetBody(str, "�?�?");

    // CURL *pCurl = NULL;
    // CURLcode res;
    // char szJsonData[1024];
    // memset(szJsonData, 0, sizeof(szJsonData));
    // strcpy(szJsonData, json.c_str());
    // char uJson[999];

    // // get a curl handle
    // pCurl = curl_easy_init();

    // if (NULL != pCurl)
    // {
    //     // 设置超时时间�?1�?
    //     curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 1);

    //     // First set the URL that is about to receive our POST.
    //     // This URL can just as well be a
    //     // https:// URL if that is what should receive the data.
    //     string url = "https://aip.baidubce.com/rpc/2.0/nlp/v2/simnet?access_token=24.8fa7658cfd43f55660f17223453704c9.2592000.1549091325.282335-15325512";
    //     curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str());

    //     // 设置http发送的内�?�类型为JSON
    //     curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");

    //     curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);

    //     // 设置要POST的JSON数据
    //     curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, szJsonData);
    //     // Perform the request, res will get the return code
    //     res = curl_easy_perform(pCurl);
    //     // Check for errors
    //     if (res != CURLE_OK)
    //     {
    //         printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
    //     }
    // }
    // curl_easy_cleanup(pCurl);
    return "OK";
}

int codeHelper::get_access_token(std::string &access_token, const std::string &AK, const std::string &SK)
{
    CURL *curl;
    CURLcode result_code;
    int error_code = 0;
    curl = curl_easy_init();
    if (curl)
    {
        std::string url = access_token_url + "&client_id=" + AK + "&client_secret=" + SK;
        curl_easy_setopt(curl, CURLOPT_URL, url.data());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        std::string access_token_result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &access_token_result);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, token_callback);
        result_code = curl_easy_perform(curl);
        if (result_code != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(result_code));
            return 1;
        }
        access_token = access_token_result;
        curl_easy_cleanup(curl);
        error_code = 0;
    }
    else
    {
        fprintf(stderr, "curl_easy_init() failed.");
        error_code = 1;
    }
    return error_code;
}

string codeHelper::getXmlInput(const string &xmlStr)
{
    string tmpStr = xmlStr;
    std::size_t start = tmpStr.find("h\">");
    std::size_t end = tmpStr.find("</input>");
    string xx;
    if (start != std::string::npos && end != std::string::npos)
    {
        string ret;

        ret = tmpStr.substr(start + 3, end - start - 3);
        wstring nn;
        this->UTF2Uni(ret.c_str(), nn);
        xx = this->ws2s(nn);
        return ret;
    }
    return xx;
}

void codeHelper::split(const string &s, vector<string> &sv, const char flag)
{
    sv.clear();
    istringstream iss(s);
    string temp;

    while (getline(iss, temp, flag))
    {

        sv.push_back(temp);
    }
}
void codeHelper::getKeyWord(multimap<int, string> &keyWord, const string &word)
{
    vector<string> v;
    this->split(word, v, '|');
    for (size_t i = 0; i < v.size(); i++)
    {
        string one = v.at(i);
        vector<string> vOne;
        this->split(one, vOne, ':');
        keyWord.insert(make_pair(atoi(vOne.at(0).c_str()), vOne.at(1)));
    }
}

/*******************************************************
    函数名称：UTF2Uni
    函数功能：utf-8转Unicode（跨平台�?
    输入参数�?
        src：utf-8编码格式的字符指�?
        t：Unicode编码格式的�?�字符串
    输出参数：无
    �? �? 值：0表示执�?�成�?
*******************************************************/
int codeHelper::UTF2Uni(const char *src, std::wstring &t)
{
    if (src == NULL)
    {
        return -1;
    }
    int size_s = strlen(src);
    int size_d = size_s * 2;
    wchar_t *des = new wchar_t[size_d];
    memset(des, 0, size_d * sizeof(wchar_t));

    int s = 0, d = 0;
    //设为true时，跳过错�??前缀
    bool toomuchbyte = true;

    while (s < size_s && d < size_d)
    {
        unsigned char c = src[s];
        if ((c & 0x80) == 0)
        {
            des[d++] += src[s++];
        }
        else if ((c & 0xE0) == 0xC0) ///< 110x-xxxx 10xx-xxxx
        {
            wchar_t &wideChar = des[d++];
            wideChar = (src[s + 0] & 0x3F) << 6;
            wideChar |= (src[s + 1] & 0x3F);

            s += 2;
        }
        else if ((c & 0xF0) == 0xE0) ///< 1110-xxxx 10xx-xxxx 10xx-xxxx
        {
            wchar_t &wideChar = des[d++];

            wideChar = (src[s + 0] & 0x1F) << 12;
            wideChar |= (src[s + 1] & 0x3F) << 6;
            wideChar |= (src[s + 2] & 0x3F);

            s += 3;
        }
        else if ((c & 0xF8) == 0xF0) ///< 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx
        {
            wchar_t &wideChar = des[d++];

            wideChar = (src[s + 0] & 0x0F) << 18;
            wideChar = (src[s + 1] & 0x3F) << 12;
            wideChar |= (src[s + 2] & 0x3F) << 6;
            wideChar |= (src[s + 3] & 0x3F);

            s += 4;
        }
        else
        {
            wchar_t &wideChar = des[d++]; ///< 1111-10xx 10xx-xxxx 10xx-xxxx 10xx-xxxx 10xx-xxxx

            wideChar = (src[s + 0] & 0x07) << 24;
            wideChar = (src[s + 1] & 0x3F) << 18;
            wideChar = (src[s + 2] & 0x3F) << 12;
            wideChar |= (src[s + 3] & 0x3F) << 6;
            wideChar |= (src[s + 4] & 0x3F);
            s += 5;
        }
    }

    t = des;
    delete[] des;
    des = NULL;

    return 0;
}

/*******************************************************
    函数名称：ws2s
    函数功能：wstring转string（跨平台），不涉及编码格式的�?�?
    输入参数�?
        ws：Unicode编码格式的wstring
    输出参数：无
    �? �? 值：Unicode编码格式的string
*******************************************************/
std::string codeHelper::ws2s(const std::wstring &ws)
{
    std::string curLocale = setlocale(LC_ALL, NULL);
#if (defined WIN32) || (defined _WIN32)
    setlocale(LC_ALL, "chs");
#else
    setlocale(LC_ALL, "zh_CN.gbk");
#endif
    const wchar_t *_Source = ws.c_str();
    size_t _Dsize = 2 * ws.size() + 1;
    char *_Dest = new char[_Dsize];
    memset(_Dest, 0, _Dsize);
    wcstombs(_Dest, _Source, _Dsize);
    std::string result = _Dest;
    delete[] _Dest;
    setlocale(LC_ALL, curLocale.c_str());
    return result;
}

/************************************************************************/
/* 得到单例                                                             */
/************************************************************************/
codeHelper *codeHelper::GetInstance()
{
    if (m_pInstance == NULL) //判断�?否�??一次调�?
        m_pInstance = new codeHelper();
    return m_pInstance;
}
