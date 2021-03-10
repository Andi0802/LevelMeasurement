// Functions to handle MQTT

#if MQTT_CLIENT
  void MQTT_Init(void)
  // Init MQTT Server
  {
    // Init MQTT Server
    MQTT.begin(MQTT_SERVER, EthClientMQTT);
    
    // Receive handler
    MQTT.onMessage(MQTT_MsgRec);
  }
  
  void MQTT_Connect(void)
  // Connect to MQTT server
  {  
    WriteSystemLog(MSG_INFO,F("Connecting MQTT server."));
    while (!MQTT.connect("Zisterne", "public", "public")) {
      Serial.print(".");
      delay(1000);
    }
    
    // Subscribe
    MQTT.subscribe("/Zisterne/Parameter/RefillLevel");
  }
  
  void MQTT_MsgRec(String &topic, String &payload)
  // Handler for receive MQTT message
  {
    WriteSystemLog(MSG_INFO,"MQTT incoming: " + topic + " - " + payload);

    // Check topic
    if (topic=="/Zisterne/Parameter/RefillLevel") {
      SettingsEEP.settings.hRefill = payload.toInt();
      WriteEEPData();
    }
  }
  
  void MQTT_Publish(void)
  // Publish MQTT message
  {
    // Reconnect if connection is lost
    if (!MQTT.connected()) {
      MQTT_Connect();
    }
  
    // Publish
    MQTT.publish("Zisterne/Alive", MeasTimeStr);
    MQTT.publish("Zisterne/MeasTime", MeasTimeStr);
    MQTT.publish("Zisterne/Level", String(hWaterActual));
    MQTT.publish("Zisterne/Volume", String(volActual));
    MQTT.publish("Zisterne/SignalHealth", String(rSignalHealth));
    MQTT.publish("Zisterne/Error", String(stError));
    MQTT.publish("Zisterne/RefillTot", String(SettingsEEP.settings.volRefillTot));
    MQTT.publish("Zisterne/RainTot", String(volRain24h));
    MQTT.publish("Zisterne/DiffPer", String(volDiff1h));      
  }
#endif    
