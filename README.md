To install dev environment, please check the file INSTALL.

This is the firmware for things related to the project movit. This firmware is suitable for all the things, all the config are stored on the sdcard.
/sdcard/param 
	First line : thing id
	second line : MQTT topic to publish
/sdcard/certificate.pem.crt 
	AWS public thing certificate (generated for the thing in IoT Core service)
/sdcard/private.pem.key
	AWS private thing key (generated for the thing in IoT Core Service)
/sdcard/aws-root-ca.pem
	AWS Public Certificate

The certificates path can be changed through idf.py menuconfig (->Example Configuration).

All the thing configuration is located on the SD Card to make the firmware totally suitable for all the things, to allow OTA firmware update.
The thing is controlled through MQTT command :

Take a picture and send it to the movit lambda
    {
      "thing": "SENSORNAME",
      "action": "getPicture",
      "param": ""
    }

DeepSleep for X seconds
    {
      "thing": "SENSORNAME",
      "action": "deepSleep",
      "param": "X"
    }

Firmware Update
    {
      "thing": "SENSORNAME",
      "action": "updateFW",
      "param": "HOST;PORT;/URI"
    }

