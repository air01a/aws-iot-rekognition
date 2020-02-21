#ifndef _HTTPJSON_H
#define _HTTPJSON_H
#include "ssl_client.h"
#include "secrets.h"
#include "WiFiClientSecure.h"
#include "Base64.h"
#define TIMEOUT 8000
#define BUFFERSIZE 500

char *imageToBase64(uint8_t *data_pic, size_t size_pic);
char  *sendImage(char *body);


#endif