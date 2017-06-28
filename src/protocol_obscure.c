#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>
typedef uint16_t packet_size_type;

static char* g_useragent[] = {
    "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:40.0) Gecko/20100101 Firefox/40.0",
    "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:40.0) Gecko/20100101 Firefox/44.0",
    "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Ubuntu/11.10 Chromium/27.0.1453.93 Chrome/27.0.1453.93 Safari/537.36",
    "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:35.0) Gecko/20100101 Firefox/35.0",
    "Mozilla/5.0 (compatible; WOW64; MSIE 10.0; Windows NT 6.2)",
    "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/533.20.25 (KHTML, like Gecko) Version/5.0.4 Safari/533.20.27",
    "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.3; Trident/7.0; .NET4.0E; .NET4.0C)",
    "Mozilla/5.0 (Windows NT 6.3; Trident/7.0; rv:11.0) like Gecko",
    "Mozilla/5.0 (Linux; Android 4.4; Nexus 5 Build/BuildID) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/30.0.0.0 Mobile Safari/537.36",
    "Mozilla/5.0 (iPad; CPU OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3",
};
static int g_useragent_index = -1;

typedef struct http_simple_local_data {
    int has_sent_header;
    int has_recv_header;
    char *encode_buffer;
}http_simple_local_data;


/*需要 filename，host，user-agent*/
static char request_header[][1024]={
"GET /%s/%s HTTP/1.1\r\nRange: bytes=0-\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nHost : %s\r\nUser-Agent: %s\r\nAccept-Encoding: gzip, deflate\r\nUpgrade-Insecure-Requests: 1\r\nConnection: Keep-Alive\r\n\r\n"
};

//ETag: \"594cce5b-1232f0\"\r\n
//需要 Date,content-length (204472320~262144000)190MB~250MB,last date,etag
static char response_header[][1024]={
"HTTP/1.1 200 OK\r\nServer: nginx/1.13.0\r\nDate: %s\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\nLast-Modified: %s\r\nConnection: keep-alive\r\nETag: \"%s\"\r\nAccept-Ranges: bytes\r\n\r\n"
};

//host的最大长度为25个字节，作为随机host，现有host有12种
#define HOSTNUM 12
static char host[HOSTNUM * 2][63]={
"www.baidu.com",
"220.181.57.217",
"www.sina.com.cn",
"218.30.108.232",
"www.nuomi.com",
"180.97.34.232",
"www.dd373.com",
"120.55.179.10",
"r3---sn-upoxu-25gs.qooqlevideo.com/videoplayback",
"1.1.1.1",
"download.winzip.com/gl/gad/",
"1.1.1.1",
"www.exavault.com/ftp",
"1.1.1.1",
"s3.onlinevideoconverter.com/download",
"1.1.1.1",
"dl.pconline.com.cn",
"1.1.1.1",
"www.xiazaiba.com",
"1.1.1.1",
"www.dytt8.net",
"1.1.1.1",
"mydown.yesky.com",
"1.1.1.1"
};

int randomNumber(int min, int max) //产生的随机数在min和max之间
{
    int num = rand()%(max + 1 - min) + min;
    return num;
}

//flag:1 首字母为大写，否则为小写(默认)， num-1:为字符串的长度
static char * randomString(char *buf, short flag, int num) 
{
    int i = 0;
    memset(buf, 0x00, num);
    if(flag == 1) {
    buf[0] = (char)randomNumber(65, 90);
    i++;
    }
    for(; i < num; i++)
        buf[i] = (char)randomNumber(97,122);
    buf[i]='\0';
    return buf;
}

static char * randomFileName(char *filename, int max) //随机生成一个文件, max 必须要 >= 8
{
    char type[][6]={".mp3", ".mp4",".avi"};
    int filechar = max - 6;
    if(filechar < 2)
    {
        printf("filename more little\n");
        return NULL;
    }
    int index = randomNumber(0,2);
    randomString(filename, 0, filechar);
    strncpy(&filename[filechar], type[index], strlen(type[index]));
    return filename;
}

//构建http协议头，返回值为http头的字符个数，filenamelen为文件名的长度
int buildHttpRequest(char *buf,int buflen)
{
    char tmp_filename[50]={0};
    char dir1[50]={0};
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    int index_host = randomNumber(0, HOSTNUM-1) *2;  //随机获取一个host
    int index_agent = randomNumber(0, 5); //随机获取user-agent
    snprintf(buf,buflen,request_header[0],
            randomString(dir1, 1, 16),
            randomFileName(tmp_filename, 12),
            &host[index_host][4], 
            g_useragent[index_agent]);

    return strlen(buf);
}

static int getGMTtime(char *buf, int buflen)
{
    time_t timep;
    struct tm * tmp;
    if(buflen < 30)
    {
        printf("getGMTtime buflen more little\n");
        return -1;
    }
    time(&timep);
    tmp = localtime(&timep);
    strftime(buf, buflen, "%a, %d %b %Y %T GMT", tmp);
    return 0;
}

int buildHttpResponse(char *buf, int buflen)
{
    char data1[32]={0};
    char data2[32]={0};
    getGMTtime(data1, 32);
    getGMTtime(data2, 32);

    //Date,content-length (204472320~262144000)190MB~250MB,last date,etag
    snprintf(buf, buflen, response_header[0],data1,randomNumber(204472320,262144000),data2,"23n348b-nw323");
    return  strlen(buf);
}

/*
int main()
{
    char filename[10]={0};
    randomFileName(filename, 10);
    printf("filename: %s\n", filename);
    char buf[1024]={0};
    int len = buildHttpRequest(buf, 1024, 10);
    printf("send httprequest=%d\nbuf=%s\n",len, buf);

    len = buildHttpResponse(buf, 1024);
    printf("send httpresponse=%d\nbuf=%s\n",len, buf);

    len = buildDnsRequest(buf, 1024,1);
    int i = 0;
    for(i = 0; i < len; i++)
        printf("%02x ",buf[i]);
    printf("\n");


    len = buildDnsResponse(buf, 1024,1);
    for(i = 0; i < len; i++)
        printf("%02X ",(unsigned char)buf[i]);
    printf("\n");
}
*/
