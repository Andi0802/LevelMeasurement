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
    int cntTout=5;
    
    WriteSystemLog(MSG_INFO,F("Connecting MQTT server."));
    while ((!MQTT.connect("Zisterne", "public", "public")) && (cntTout--)) {
      Serial.print(".");
      delay(1000);            
      // Trigger Watchdog
      TriggerWDT(); 
    }    

    if(cntTout>0) {
      // Subscribe   
      WriteSystemLog(MSG_INFO,F("MQTT: Subscribe topic Zisterne/Parameter/RefillLevel"));
      MQTT.subscribe(F("Zisterne/Parameter/RefillLevel"));  
      #if (USE_TEST_SYSTEM & TEST_USDATA)
        //Subscribe simulated runtimes and noise for test system
        WriteSystemLog(MSG_INFO,F("MQTT: Subscribe topic Zisterne/Simulation/testtime+testnoise"));
        MQTT.subscribe(F("Zisterne/Simulation/testtime"));
        MQTT.subscribe(F("Zisterne/Simulation/testnoise"));      
      #endif
    }
    else {
      // Timeout occured
      WriteSystemLog(MSG_INFO,F("MQTT: Timeout while connecting."));
    }
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
    MQTT.publish(F("Zisterne/PercLevel"), String(prcActual));
    MQTT.publish(F("Zisterne/Volume"), String(volActual));
    MQTT.publish(F("Zisterne/SignalHealth"), String(rSignalHealth));
    MQTT.publish(F("Zisterne/Error"), String(stError));
    MQTT.publish(F("Zisterne/RefillTot"), String(SettingsEEP.settings.volRefillTot));
    MQTT.publish(F("Zisterne/RainTot"), String(volRainTot));
    MQTT.publish(F("Zisterne/DiffPer"), String(volDiff1h));      
    WriteSystemLog(MSG_INFO,F("MQTT: Topics published"));
  }
#endif    
