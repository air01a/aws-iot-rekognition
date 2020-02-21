#include "HTTPJson.h"
#define HEADER "POST %s  HTTP/1.1\r\ncache-control: no-cache\r\nContent-Type: application/json\r\nUser-Agent: PostmanRuntime/6.4.1\r\nAccept: */*\r\nHost: %s\r\nx-api-key: %s\r\nConnection: close\r\ncontent-length: %d\r\n\r\n"

#if defined(SECURE) && SECURE > 0    
#define HTTPCLIENT WiFiClientSecure
#else
#define HTTPCLIENT WiFiClient
#endif

char resHeader[1024]="";

// *********************************************************
// * Format HTTP Header
// *********************************************************
char *getHeader(size_t length)
{
    char * data;
    int size;

    int packetSize = strlen(HEADER)+strlen(URL)+strlen(SERVER)+strlen(TOKEN)+15;
    data = (char *)malloc(packetSize);
    sprintf(data,HEADER,URL,SERVER,TOKEN,length);
    
  return (data);
}

// *********************************************************
// * Convert char buffer to base64 string 
// *********************************************************

char *imageToBase64(uint8_t *data_pic, size_t size_pic)
{
  char *base64Image;
  int sSize = (floor(size_pic / 3) + 1) * 4 + 1;
  base64Image = (char *)malloc(sSize);
  memset(base64Image,0,sSize);
  base64_encode(base64Image,(char *)data_pic,size_pic);
  return base64Image;
}

// *****************************************************************
//* Used to read one header line from http response
// ***************************************************************** 

void httpReadLine( HTTPCLIENT *client,char *buff, int buffsize, long tOut)
{
  int ptr = 0;
  while (client->connected()&& tOut > millis() && ptr<buffsize)
  {
    buff[ptr] = char(client->read());

    if (buff[ptr]!=255){
        if (buff[ptr]=='\n')
          break;
        ptr++;
    }
  }
  buff[ptr+1]=0;
  ESP_LOGI(TAG,"%s",buff);
}

// *****************************************************************
// * search string in http header and get int value associated
// ***************************************************************** 

int getHttpValue(char *line, char *varName){
  char tmp[30];
  int ptr = 0;
  char *str=strstr(line,varName);
  if (str==NULL)
    return -1;
  
  str+=strlen(varName);
  while (*str==' ' or *str==':')
    str++;

  while (*str<='9' and *str>='0')
  {
    tmp[ptr++]=*str;
    str++;
  }
    tmp[ptr]='\0';

  ESP_LOGI(TAG,"%s",tmp);
  return atoi(tmp);
  
}

// *****************************************************************
// * Send image using HTTP (or HTTPS) protocol
// ***************************************************************** 
char *sendImage(char *body)
{

  ESP_LOGI(TAG,"+++ Memory start sendimage destr: %d\n",ESP.getFreeHeap());

  char *header;
  int contentLength = 0;
  int response = 0;
  HTTPCLIENT client;

  // ------------------------------------------------
  // Initialize header
  // ------------------------------------------------

  size_t allLen =  strlen(body);
  header = getHeader(allLen);

  char *data;
  // ------------------------------------------------
  // Init HTTP connection
  // ------------------------------------------------

  int err = client.connect(SERVER, PORT);
  if (!err)
  {
    ESP_LOGE(TAG,"Connection to HTTP server Failed\n");
    return ("connection failed");
  }

  // ------------------------------------------------
  // client.write does not work for big packet (>4k)
  // So split the data
  // ------------------------------------------------
 
  ESP_LOGD(TAG,"%s",header);
  err = client.print(header);
  ESP_LOGI(TAG,"send : %d/%d",err,strlen(header));
  free(header);  

  int ptr=0;
  err=1;
  while (ptr<strlen(body)&&(err!=0)) {
      err = client.print(body+ptr);
      ESP_LOGI(TAG,"send : %d/%d",err,strlen(body));
      ptr+=err;
      if(err==0)
          break;
  }
  err = client.print("\r\n");
  
  // Get response Header
  long tOut = millis() + TIMEOUT;
  httpReadLine(&client,resHeader,1024,tOut);
  ESP_LOGI(TAG,"RESP HEADER: %s",resHeader);

  response = getHttpValue(resHeader,"HTTP/1.1");

  while (strcmp(resHeader,"\r\n")!=0 &&client.connected()&& tOut > millis()) {
    httpReadLine(&client,resHeader,1024,tOut);
    ESP_LOGI(TAG,"RESP HEADER: %s",resHeader);
    if (contentLength<=0)
      contentLength=getHttpValue(resHeader,"Content-Length");
  }
  data=NULL;
  ESP_LOGI(TAG,"HTTP Code : %d, Content Length : %d",response,contentLength);

  // Get response body
  if (contentLength>0) 
  {
    data = (char *) malloc(contentLength+1);
    ptr=0;
    while (client.connected()&& tOut > millis() && ptr<=contentLength) {
      data[ptr++]=char(client.read());
    }
    data[ptr-1]='\0';
    ESP_LOGI(TAG,"HTTP data : %s",data);
  }

  client.stop();
  return data;
}