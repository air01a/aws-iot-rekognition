#ifndef H_AWS_SUBSCRIBE
#define H_AWS_SUBSCRIBE
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"


#define HEALTHCHECKDELAY 1000
void aws_iot_task(void (*iot_callback_handler)(AWS_IoT_Client * , char *, uint16_t ,
                                    IoT_Publish_Message_Params *, void *),char *CLIENT_ID, char *TOPIC);

#endif