#include "WiFiClientSecure.h"
#include "ssl_client.h"
#include <math.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "Arduino.h"
#include "secrets.h"
#include "aws_mqtt.h"
#include "HTTPJson.h"
#include "aws_iot_mqtt_client_interface.h"
#include <cJSON.h>
#include "updater.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#define DEBUG 1
int LOOPCTRL=0;
char CLIENT_ID[50];
char CLIENT_TOPIC[50];
// ------------------------------------------------
// define the number of bytes you want to access
// ------------------------------------------------


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  65        /* Time ESP32 will go to sleep (in seconds) */
#define MAX_WIFI_CONNECTION_LOOP 100
#define EEPROM_SIZE 1

//#define CLIENT_ID "esp32R1"

// ------------------------------------------------
// Pin definition for CAMERA_MODEL_AI_THINKER
// ------------------------------------------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define JSONIMAGEPACKET "{\"deviceId\":\"%s\",\"image\":\"%s\"}"




// *****************************************************************
// * Connect to WiFi
// ***************************************************************** 
void initWifi()
{
  // ------------------------------------------------
  // Wifi Connection
  // ------------------------------------------------
    ESP_LOGI(TAG,"MAC Address : %d : %d : %d : %d : %d : %d\n",WiFi.macAddress()[0],WiFi.macAddress()[1],WiFi.macAddress()[2],WiFi.macAddress()[3],WiFi.macAddress()[4],WiFi.macAddress()[5]);

  WiFi.begin(WIFISSID, WIFIPASSWORD);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ESP_LOGI(TAG,".");
    if (cnt++ > MAX_WIFI_CONNECTION_LOOP )
    {
      esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
      //DEBUG_PRINTLN("\nFAILED TO CONNECT WIFI : Setup ESP32 to sleep for every 60 Seconds");
      esp_deep_sleep_start();

    }
  }
  ESP_LOGI(TAG,"Connected\n"); 
  ESP_LOGI(TAG,"Ip Address : %d.%d.%d.%d\n",WiFi.localIP()[0],WiFi.localIP()[1],WiFi.localIP()[2],WiFi.localIP()[3]);
}

// *****************************************************************
// * Manage WiFi deconnection
// *****************************************************************
void WiFiEvent(WiFiEvent_t event)
{

    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            WiFi.reconnect();
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
  }
}
// *****************************************************************
// * Convert image to B64 and send it through http(s)
// ***************************************************************** 
char *sendPicture(uint8_t *data_pic, size_t size_pic)
{

  char *sBase64Image;
  char *sJson;

  sBase64Image = imageToBase64(data_pic,size_pic);
  int len = strlen(JSONIMAGEPACKET)+strlen(sBase64Image)+strlen(CLIENT_ID);
  sJson = (char *)malloc(len);
  sprintf(sJson,JSONIMAGEPACKET,CLIENT_ID,sBase64Image);
  free(sBase64Image);
  sBase64Image=sendImage(sJson);
  free(sJson);
  return sBase64Image;

}


// *****************************************************************
// * Get picture from camera
// ***************************************************************** 

char *getPicture(void) 
{
  char *res;

  camera_fb_t * fb = NULL;

  // ------------------------------------------------
  // Take Picture with Camera
  // ------------------------------------------------
ESP_LOGI(TAG,"+++ Memory before fb_get: %d\n",ESP.getFreeHeap());

  fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGI(TAG,"Camera capture failed\n");
    return NULL;
  } else {
    
    
    ESP_LOGI(TAG,"Sending image to server, size : %d\n",fb->len);
    delay(500);
    
    // ------------------------------------------------
    // Send image through HTTP protocol
    // ------------------------------------------------
    ESP_LOGI(TAG,"+++ Memory before sendpicture: %d\n",ESP.getFreeHeap());
    res = sendPicture( fb->buf, fb->len);
    ESP_LOGI(TAG,"+++ Memory after sendpicture: %d\n",ESP.getFreeHeap());
    //ESP_LOGI(TAG,"Return : %s\n",res);
    esp_camera_fb_return(fb);
    ESP_LOGI(TAG,"+++ Memory after fb_return: %d\n",ESP.getFreeHeap());
    return res;
  }
}

// *****************************************************************
// * Deep sleep function
// ***************************************************************** 
void doDeepSleep(int time) {
  esp_sleep_enable_timer_wakeup(time * uS_TO_S_FACTOR);
  ESP_LOGI(TAG,"Setup ESP32 to sleep for every %d Seconds\n",time);
  esp_deep_sleep_start();
}


// *****************************************************************
// * do action according to mqtts
// ***************************************************************** 

void iot_publish_message(AWS_IoT_Client *pClient,char *message)
{
  IoT_Publish_Message_Params paramsQOS0;

  paramsQOS0.qos = QOS0;
  paramsQOS0.payload = (void *) message;
  paramsQOS0.isRetained = 0;
  paramsQOS0.payloadLen = strlen(message);
  aws_iot_mqtt_publish(pClient, CLIENT_TOPIC, strlen(CLIENT_TOPIC), &paramsQOS0);

}

void updateFirmWare(char * param)
{
  String host;
  String url;
  int port;
  String sParameter=String(param);
  int ind1 = sParameter.indexOf(';');  //finds location of first ,
  if ((ind1)>0)
  {
    host = sParameter.substring(0,ind1);
    int ind2 = sParameter.indexOf(';', ind1+1 );   //finds location of second ,
    port = sParameter.substring(ind1+1,ind2).toInt();
    ind1 = sParameter.indexOf(';',ind2+1);
    url = sParameter.substring(ind2+1,ind1);
    if ((host==NULL)||port==0||url==NULL)
    {
        ESP_LOGI(TAG,"[Update] - error with param %s",param);
        doDeepSleep(60);
    }
    execOTA(host,url,port,0);
    ESP_LOGI(TAG,"[Update] - Error during update");
    doDeepSleep(60);
  }  
  ESP_LOGI(TAG,"[Update] - error with param %s",param);
  doDeepSleep(60);
}


void doAction(AWS_IoT_Client *pClient,char *action, char *param) {
  char message[]="{\"result\":\"%s\",\"Message\":\"%s\"}";
  char fullMessage[200];
  char *res;

  if (strcmp(action,"getPicture")==0) {
    res = getPicture();
    if (res!=NULL) {
      snprintf(fullMessage,200,message,"OK",res);
      free(res);
      iot_publish_message(pClient,fullMessage);

    } else {
      snprintf(fullMessage,200,message,"KO","Error reading result");
      iot_publish_message(pClient,fullMessage);
    }
  }
  else if (strcmp(action,"deepSleep")==0)
      {
        // ------------------------------------------------
        // Deep Sleep
        // ------------------------------------------------
        int tts = atoi(param);
        doDeepSleep(tts);
      } 
      else if (strcmp(action,"updateFW")==0)
        updateFirmWare(param);

}


// *****************************************************************
// * MQTT callback handler when a message is receive
// ***************************************************************** 
void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    
const cJSON *thing = NULL;
const cJSON *action = NULL;
const cJSON *param = NULL;
ESP_LOGI(TAG,"+++ Free Memory: %d\n",ESP.getFreeHeap());
ESP_LOGI(TAG, "Subscribe callback");
ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);
cJSON *json = cJSON_Parse((char *)params->payload);
if (json != NULL)
{
  thing = cJSON_GetObjectItemCaseSensitive(json, "thing");
  if  ((thing!=NULL)&& cJSON_IsString(thing) && (thing->valuestring != NULL))
  {
    if (strcmp(thing->valuestring,CLIENT_ID)==0) {
        ESP_LOGI(TAG, "Message FOR ME : %s / %s",thing->valuestring,CLIENT_ID);
        action = cJSON_GetObjectItemCaseSensitive(json, "action");
        param = cJSON_GetObjectItemCaseSensitive(json, "param");
        if  (cJSON_IsString(thing) && (thing->valuestring != NULL) &&cJSON_IsString(param) && (param->valuestring != NULL) ) {
          doAction(pClient,action->valuestring,param->valuestring);
        }
        LOOPCTRL++;
        if (LOOPCTRL>10)
          doDeepSleep(60);
    }
  }
}

ESP_LOGI(TAG,"+++ Memory before JSON destr: %d\n",ESP.getFreeHeap());

    cJSON_Delete(json);
    ESP_LOGI(TAG,"+++ Memory after JSON destr: %d\n",ESP.getFreeHeap());

}
// *****************************************************************
// * Entry Point
// ***************************************************************** 

void setup()
{  
  // ------------------------------------------------
  // Open Serial
  // ------------------------------------------------
  psramInit();

  #if defined(DEBUG) && DEBUG > 0
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  #endif

  initWifi();
  WiFi.onEvent(WiFiEvent);

  // ------------------------------------------------
  // Camera configuration for AI Thinker ESP 32
  // ------------------------------------------------
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 15;
  if (psramFound()) {
  config.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10;
  config.fb_count = 2;
  } else {
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  }


  ESP_LOGI(TAG,"INIT CAMERA");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
  ESP_LOGI(TAG,"Camera init failed with error %d",(err));
  esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
  ESP_LOGI(TAG,"DEEPSLEEP FOR %d s\n",60);
  esp_deep_sleep_start();
  }



    ESP_LOGI(TAG, "Mounting SD card...");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 3,
  };
  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
      abort();
  }

  // Get id from id file
  FILE *f = fopen("/sdcard/param", "r");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open param file for reading");
      doDeepSleep(60);
  }
  fgets(CLIENT_ID, sizeof(CLIENT_ID), f);

  char* pos = strchr(CLIENT_ID, '\n');
  if (pos) {
      *pos = '\0';
  }
  ESP_LOGI(TAG, "Read thing Id : %s", CLIENT_ID);


  fgets(CLIENT_TOPIC, sizeof(CLIENT_TOPIC), f);
  pos = strchr(CLIENT_TOPIC, '\n');
  if (pos) {
      *pos = '\0';
  }
  ESP_LOGI(TAG, "Read TOPIC  : %s", CLIENT_TOPIC);

  fclose(f);


  // ------------------------------------------------
  // Init Camera
  // ------------------------------------------------
  aws_iot_task(iot_subscribe_callback_handler,CLIENT_ID, CLIENT_TOPIC);

  esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
  ESP_LOGI(TAG,"END OF SETUP, DEEPSLEEP FOR %d s\n",60);
  esp_deep_sleep_start();
}
// *****************************************************************
// * Never called but necessary to Arduino function
// ***************************************************************** 
void loop()
{

}

