//EEP_functions: Functions to handle EEP Data

void ReadEEPData(void)
{
  //Reads EEPData from EEP
  int i;
  byte _id;
  byte chk;

  // EEPROM Test active in LOGLVL_EEP
  #if LOGLEVEL & LOGLVL_EEP          
    // Test EEPROM
    TestEEP();
    // Show dump
    DumpEEPData();
  #endif
  
  //Read data stream
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(MSG_INFO,F("Reading settings from EEP"));
  #endif  
  for (i = 0; i < EEPSize; i++) {
    SettingsEEP.SettingStream[i] = EEPROM.read(i);    
  }  

  // Check plausibility
  chk = checksum(&SettingsEEP.SettingStream[1], EEPSize - 1);
  if (SettingsEEP.settings.stat == chk) {
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(MSG_INFO,F("EEP checksum ok "));
      WriteSystemLog(MSG_DEBUG,"EEPSize " + String(EEPSize) + " Byte");
    #endif      
  }
  else {    
   //Initialise new: Automatically takes the init data    
   #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(MSG_WARNING,F("EEP checksum wrong "));
      WriteSystemLog(MSG_INFO,"EEPSize " + String(EEPSize) + " Byte");
      WriteSystemLog(MSG_DEBUG,"Checksum in EEPROM : " + String(SettingsEEP.settings.stat,HEX));
      WriteSystemLog(MSG_DEBUG,"Checksum calculated: " + String(chk,HEX));
      WriteSystemLog(MSG_INFO,F("Initializing EEPROM"));
   #endif  
   SettingsEEP.settings.cycle_time = 60;            //Calculating base cycle time [s]
   SettingsEEP.settings.vSound = 3433;              //Velocity of sound [mm/sec]
   SettingsEEP.settings.hOffset = -25;              //Runtime offset [mm]
   SettingsEEP.settings.aBaseArea = 31416;          //Base area of reservoir [cm*cm]
   SettingsEEP.settings.hSensorPos = 240;           //Height of sensor above base area [cm]
   SettingsEEP.settings.hOverflow = 200;            //Height of overflow device [cm]
   SettingsEEP.settings.hRefill = 20;               //Height for refilling start [cm]
   SettingsEEP.settings.volRefill = 20 ;            //Volume to refill [l]
   SettingsEEP.settings.dvolRefill = 20;            //Mass flow of refiller [l/sec]  
   SettingsEEP.settings.volRefillTot=0;             //Refilling volume total [l]
   SettingsEEP.settings.aRoof=100;                  //Area of roof [m^2]
   //Filter efficiency [%]
   SettingsEEP.settings.prcFiltEff_x[0]=1;
   SettingsEEP.settings.prcFiltEff_x[1]=5;
   SettingsEEP.settings.prcFiltEff_x[2]=7;
   SettingsEEP.settings.prcFiltEff_x[3]=10;
   SettingsEEP.settings.prcFiltEff_x[4]=20;
   SettingsEEP.settings.prcFiltEff_y[0]=100;
   SettingsEEP.settings.prcFiltEff_y[1]=100;
   SettingsEEP.settings.prcFiltEff_y[2]=90;
   SettingsEEP.settings.prcFiltEff_y[3]=40;
   SettingsEEP.settings.prcFiltEff_y[4]=20;
   SettingsEEP.settings.prcVolDvtThres=20;          //Threshold for diagnosis
   SettingsEEP.settings.volRainMin=1;               //Minimum 2Liter/1h to activate filter diagnosis
   SettingsEEP.settings.tiRefillReset=timeClient.getEpochTime();
   for (_id=0;_id<NUM_USAGE_AVRG;_id++) {           //History of usage 
      SettingsEEP.settings.volUsage24h[_id] = 32767;
   }
   SettingsEEP.settings.iDay=0;                     //Start with day 0

   //History sample data
   for (_id=0;_id<EEP_NUM_HIST;_id++) {
     SettingsEEP.settings.volRain1h[_id] = SAMPLES_VOLRAIN[_id];
     SettingsEEP.settings.prcActual[_id] = byte(SAMPLES_PRCVOL[_id]);    
     SettingsEEP.settings.stSignal[_id]  = SAMPLES_STSIGNAL[_id];
   }
   SettingsEEP.settings.iWrPtrHist = 0;

   //Write data
   WriteEEPData();
  }
}

void WriteEEPData(void)
{
  //Reads EEPData from EEP
  unsigned int i;
  byte chk;

  chk = checksum(&SettingsEEP.SettingStream[1], EEPSize - 1);
  SettingsEEP.settings.stat = chk;
  for (i = 0; i < EEPSize; i++) {
    EEPROM.write(i, SettingsEEP.SettingStream[i]);
  }
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(MSG_INFO,F("Writing settings to EEP"));
    WriteSystemLog(MSG_DEBUG,"EEP Checksum "+String(chk,HEX));
  #endif
}

void DumpEEPData(void)
{
  //Writes EEP Bytes to Logfile and Serial
  File logFile;
  String LogStr;
  unsigned int i;
  byte eep_byte;
  
  WriteSystemLog(MSG_DEBUG,"EEP Data Dump");

  //Log data to SD Card
  LogStr = F("  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");    
    for (i = 0; i < EEPSize; i++) {
      if (i%16==0) {
        LogStr.concat("\n");
        LogStr.concat(String(i/16,HEX)+" ");
      }
      eep_byte = EEPROM.read(i);
      if (eep_byte<16) {
        LogStr.concat("0");
      }        
      LogStr.concat(String(eep_byte,HEX));  
      LogStr.concat(" ");
    }
    
  if (SD_State == SD_OK) {    
    Workaround_CS_ETH2SD(60);
    
    logFile = SD.open(LOGFILE, FILE_WRITE);
    
    logFile.flush();
    logFile.close();    
        
    Workaround_CS_SD2ETH(60);    
  }

  //Write to serial
  Serial.println(LogStr);
}

void WriteEEPCurrData(unsigned char prcActual, float volRain1h, bool stLevel, bool stRain, unsigned int volRefill1h)
//Writes current values into history buffer
{ 
  byte stSignal=0;

  //Calculate stSignal
  if (stLevel) {
    stSignal = 1;
  }
  if (stRain) {
    stSignal = stSignal | 2;
  }
  stSignal = stSignal | (round(volRefill1h/CONV_REFILL)<<2);

  //Write data into EEP
  SettingsEEP.settings.volRain1h[SettingsEEP.settings.iWrPtrHist] = float2uint8(volRain1h,CONV_RAIN);
  SettingsEEP.settings.prcActual[SettingsEEP.settings.iWrPtrHist] = prcActual;
  SettingsEEP.settings.stSignal[SettingsEEP.settings.iWrPtrHist]  = stSignal;
   
   
  // Increment history counter
  SettingsEEP.settings.iWrPtrHist = (SettingsEEP.settings.iWrPtrHist+1)%EEP_NUM_HIST;

  //Write into EEP
  WriteEEPData();
}

void TestEEP()
// Test function for EEPROM: Use only in test configuration to avoid unnecessary write cycles
{
  int len,ofs,i,ctrError;
  byte buf;
  
  // Determine length of EEPROM
  len = EEPROM.length();
  WriteSystemLog(MSG_DEBUG,"EEPROM Size "+String(len));

  // Determine offet
  ofs = EEPROM.GetOffset();
  WriteSystemLog(MSG_DEBUG,"EEPROM Offset "+String(ofs));

  // Test only on free space (not NetEEPROM reserved)
  len=len-ofs;

  // Read all data in buffer
  WriteSystemLog(MSG_DEBUG,F("Performing EEPROM test"));
  ctrError=0;
  for (i=0;i<len;i++) {
    // Store original data
    buf = EEPROM.read(i);
    
    //Write test data
    EEPROM.write(i,(len-i)%256);

    // Read and check test data
    if (EEPROM.read(i)!=(len-i)%256) {
      ctrError++;
    }
    
    // Write back original data
    EEPROM.write(i,buf);
  }

  // Show result
  if (ctrError>0) {
    WriteSystemLog(MSG_DEBUG,"EEPROM Error count: "+String(ctrError));
  }
  else {
    WriteSystemLog(MSG_DEBUG,"EEPROM Check ok");
  }
}

unsigned char checksum (unsigned char *ptr, size_t sz) 
//Calculate Checksum
{
  unsigned char chk = 0;
  while (sz-- != 0)
    chk -= *ptr++;
  return chk;
}

unsigned char float2uint8(float value, float cnv)
// Normalises and limits float variable to be stored in uint8
// cnv=0.3 means: value = (float) 0.3 => return (unsigned char) 1 
{  
  return (unsigned char) (min(value/cnv,255));  
}

