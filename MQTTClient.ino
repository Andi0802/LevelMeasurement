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
    MQTT.subscribe(F("Zisterne/Parameter/RefillLevel"));  
    #if (USE_TEST_SYSTEM & TEST_USDATA)
      //Subscribe simulated runtimes and noise for test system
      MQTT.subscribe(F("Zisterne/Simulation/testtime"));
      MQTT.subscribe(F("Zisterne/Simulation/testnoise"));      
    #endif
  }
  
  void MQTT_MsgRec(String &topic, String &payload)
  // Handler for receive MQTT message
  {
    WriteSystemLog(MSG_INFO,"MQTT incoming: " + topic + " - " + payload);

    // Check topic
    if (topic==F("Zisterne/Parameter/RefillLevel")) {
      SettingsEEP.settings.hRefill = min(max(payload.toInt(),0),60);
      WriteEEPData();
    }
    #if (USE_TEST_SYSTEM & TEST_USDATA)
      //Receive simulated runtimes and noise for test system
      if (topic==F("Zisterne/Simulation/testtime")) {
        test_time = min(max(payload.toInt(),0),50000);        
      }
      if (topic==F("Zisterne/Simulation/testnoise")) {
        test_noise = min(max(payload.toInt(),0),5000);        
      }      
    #endif
  }
  
  void MQTT_Publish(void)
  // Publish MQTT message
  {
    // Reconnect if connection is lost
    if (!MQTT.connected()) {
      MQTT_Connect();
    }
  
    // Publish
    MQTT.publish(F("Zisterne/Alive"), MeasTimeStr);
    MQTT.publish(F("Zisterne/MeasTime"), MeasTimeStr);
    MQTT.publish(F("Zisterne/Level"), String(hWaterActual));
    MQTT.publish(F("Zisterne/Volume"), String(volActual));
    MQTT.publish(F("Zisterne/SignalHealth"), String(rSignalHealth));
    MQTT.publish(F("Zisterne/Error"), String(stError));
    MQTT.publish(F("Zisterne/RefillTot"), String(SettingsEEP.settings.volRefillTot));
    MQTT.publish(F("Zisterne/RainTot"), String(volRain24h));
    MQTT.publish(F("Zisterne/DiffPer"), String(volDiff1h));      
  }
#endif    
