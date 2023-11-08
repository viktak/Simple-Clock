#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

extern void PublishData(const char *topic, const char *payload, bool retained);
extern void ConnectToMQTTBroker();
extern void SendHeartbeat();

extern void setupMQTT();
extern void loopMQTT();

#endif
