// Functions to handle SQL client
#if SQL_CLIENT>0
void SendDataSQL(void) {
  // Sends data to SQL server
  byte steller;
  unsigned long  tiTimeout;
  bool stTimeout=false;
  String _reqString;

  if (ETH_State > ETH_FAIL) {
    // Get Date String in format for SQL Server: "YYYY-MM-DD_HH:MM:SS"
    String DateTimeSQL = MeasTimeStr;
    DateTimeSQL.replace(" ", "_");

    // Connect to SQL server
    int res = client.connect(ExternalServer, 80);
    if (res >= 0) {
      // If connection can be established
      #if LOGLEVEL & LOGLVL_SQL
        WriteSystemLog(MSG_MED_DEBUG,"Connected to SQL server "+ExternalServer);
      #endif
      
      // Make a HTTP request to write data to SQL Server
      _reqString = "GET /joomla/templates/";
      _reqString = _reqString + SQLDataBaseName;
      _reqString = _reqString + "/add.php?Zeit=";
      client.print(_reqString);
      client.print(DateTimeSQL);  // Format: "YYYY-MM-DD_HH:MM:SS"
      client.print("&hWaterActual=");
      client.print(String(hWaterActual));
      client.print("&volActual=");
      client.print(volActual);
      client.print("&rSignalHealth=");
      client.print(String(rSignalHealth));
      client.print("&stError=");
      client.print(stError);
      client.print("&volStdDev=");
      client.print(volStdDev);
      client.print("&volRefillTot=");
      client.print(String(SettingsEEP.settings.volRefillTot));
      client.print("&volRain24h=");
      client.print(String(volRain24h));
      client.print("&volRefillDiag1h=");
      client.print(String(volRefillDiag1h));
      client.print("&volDiffDiag1h=");
      client.print(String(volDiffDiag1h));
      client.print("&volDiffCalc1h=");
      client.print(String(volDiffCalc1h));
      client.print("&volDiff1h=");
      client.print(volDiff1h);
      client.print("&stFiltChkErr=");
      client.print(stFiltChkErr);
      client.print("&volRainDiag1h=");
      client.print(String(volRainDiag1h));
      
      client.println("Host: "+String(ExternalServer));
      client.println("Connection: close");
      client.println();
    }
    else {
      // Negative values marking an error
      // if you didn't get a connection to the server:
      WriteSystemLog(MSG_MED_ERROR,"connection failed to SQL server "+String(ExternalServer));
    }
    
    // if there are incoming bytes available
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      //Trigger WDT
      TriggerWDT();
      #if LOGLEVEL & LOGLVL_SQL
        Serial.print(c);
      #endif
    }
    
    // if the server's disconnected, stop the client:
    tiTimeout = millis()+5000;
    while (client.connected() && (stTimeout==false)) {
      //Trigger WDT
      TriggerWDT();
      if (millis()>tiTimeout) {
        //Timeout
        stTimeout=true;
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(MSG_MED_ERROR,"Timeout: SQL server connection not ended");
        #endif
      }
    }
    if (!client.connected()) {
      #if LOGLEVEL & LOGLVL_SQL
        WriteSystemLog(MSG_MED_DEBUG,"disconnecting from SQL server");
      #endif  
      client.stop();
    }
  }
  else {
     #if LOGLEVEL & LOGLVL_NORMAL
       WriteSystemLog(MSG_MED_ERROR,"No connection to SQL server");
     #endif
  }
}
#endif
