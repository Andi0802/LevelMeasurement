//DataLog: Functions to log data on SD card

void LogData(void)
//Logs data to SD
{
  File logFile;
  int _i;

  //Log data to SD Card
  if (SD_State == SD_OK) {
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(MSG_INFO,F("Writing measurement data to SD Card"));
    #endif
        
    Workaround_CS_ETH2SD(20);    
    
    logFile = SD.open(DATAFILE, FILE_WRITE);    
    //Log Measurement data
    logFile.print(MeasTimeStr);      logFile.print(F(";"));
    logFile.print(hWaterActual);     logFile.print(F(";"));
    logFile.print(volActual);        logFile.print(F(";"));
    logFile.print(rSignalHealth);    logFile.print(F(";"));
    if (stError) {
      logFile.print(F("Fehler erkannt"));            
    }
    else {
      logFile.print(F("Kein Fehler erkannt"));
    }
    logFile.print(F(";"));
    logFile.print(volStdDev);        logFile.print(F(";"));
    logFile.print(SettingsEEP.settings.volRefillTot);   logFile.print(F(";"));
    logFile.print(volRain24h);        logFile.print(F(";"));

    //Check if Diagnosis log is required
    if (stFilterCheck==FILT_DIAG) {      
      logFile.print(volRefillDiag1h);  logFile.print(F(";"));      
      logFile.print(volDiffDiag1h);    logFile.print(F(";"));
      logFile.print(volDiffCalc1h);    logFile.print(F(";"));
      logFile.print(prcVolDvt1h);      logFile.print(F(";"));
      switch (stFiltChkErr) {
        case FILT_ERR_UNKNOWN:
          logFile.print(F("Unbekannt"));            
          break;
        case FILT_ERR_NOMEAS:
          logFile.print(F("Ungültige Pegelmessung"));            
          break;
        case FILT_ERR_CLOGGED:
          logFile.print(F("Verstopft"));            
          break;
        case FILT_ERR_OK:
          logFile.print(F("OK"));            
          break;
      }
      logFile.print(F(";"));            
    }
    else {
      //Empty cells
      logFile.print(F(";;;;;"));            
    }

    //Check if Usage log is required
    if (stFilterCheck==FILT_USAGE) {
      logFile.print(SettingsEEP.settings.volUsage24h[SettingsEEP.settings.iDay]); logFile.print(F(";"));
      logFile.print(volUsageAvrg10d);   logFile.print(F(";"));      
    }
    else {
      //Empty cells
      logFile.print(F(";;"));            
    }
    
    //Newline and Close
    logFile.println();
    logFile.flush();
    logFile.close();

    Workaround_CS_SD2ETH(20);   
  
  }
  else {
    WriteSystemLog(MSG_MED_ERROR,F("Writing to SD Card failed"));
  }
}

void LogDataHeader(void)
//Write log data headline to SD
{
  File logFile;

  //Log data to SD Card
  if (SD_State == SD_OK) {
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(MSG_INFO,F("Writing headline for measurement data to SD Card"));
    #endif  

    Workaround_CS_ETH2SD(30);    
          
    logFile = SD.open(DATAFILE, FILE_WRITE);  
    logFile.print(F("Date;"));
    logFile.print(F("Level [mm];"));
    logFile.print(F("Quantity [Liter];"));
    logFile.print(F("Signal health [-];"));
    logFile.print(F("Error [-];"));
    logFile.print(F("StdDev [Liter];"));
    logFile.print(F("Refill Quantity [Liter];"));  
    logFile.print(F("Rain quantity 24h [Liter];"));  
    logFile.print(F("Refill quantity last period [Liter];"));  
    logFile.print(F("Measured quantity change last period [Liter];")); 
    logFile.print(F("Calculated quantity change last period [Liter];"));   
    logFile.print(F("Quantity difference [%];"));   
    logFile.print(F("Filter diagnosis [-];"));   
    logFile.print(F("Usage in last 24h [Liter];"));   
    logFile.print(F("Usage average in last 10d [Liter]"));   
    logFile.println();  
    logFile.flush();    
    logFile.close();
    
    Workaround_CS_SD2ETH(30);
  }
  else {
    WriteSystemLog(MSG_MED_ERROR,F("Writing to SD Card failed"));
  }
}

void WriteSystemLog(byte Type, String LogText)
// Write System log entry
//#define MSG_SEV_ERROR 31
//#define MSG_MED_ERROR 30
//#define MSG_WARNING   20
//#define MSG_INFO      10
//#define MSG_DEBUG      0
{  
  File logFile;
  String date_time;
  String LogStr;

  //Get Time
  date_time = getDateTimeStr();

  //Log data to SD Card
  LogStr = "[";
  LogStr.concat(date_time);
  
  switch (Type) {
    case MSG_DEBUG:
      LogStr.concat(" DEBUG");
      break;
    case MSG_INFO:
      LogStr.concat(" INFO");
      break;      
    case MSG_WARNING:
      LogStr.concat(" WARNING");
      break;      
    case MSG_MED_ERROR:
      LogStr.concat(" ERROR");
      break;      
    case MSG_SEV_ERROR:
      LogStr.concat(" SEVERE");
      break;         
  }
  LogStr.concat("] ");
  
  LogStr.concat(LogText);

  if (SD_State == SD_OK) {    
    Workaround_CS_ETH2SD(50);
      
    logFile = SD.open(LOGFILE, FILE_WRITE);
    logFile.println(LogStr);
    logFile.flush();
    logFile.close();    
        
    Workaround_CS_SD2ETH(50);    
  }

  //Write to serial
  Serial.println(LogStr);

  //Trigger watchdog
  TriggerWDT();  

  //To Display
  #if DISP_ACTIVE==1
    DispMessage(Type, LogStr);
  #endif

}
