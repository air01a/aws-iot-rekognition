#ifndef _UPDATER_H
#define _UPDATER_H

#include <Update.h>
#include "WiFiClientSecure.h"

void execOTA(String host, String bin, int port,int retry);
#endif