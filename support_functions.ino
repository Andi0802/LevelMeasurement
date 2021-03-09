//support_functions: Collection of multiple support functions


//--- Time functions ----------------------------------------------------------------------------------------------------
time_t getNtpTime()
//Receive time from NTP Server
{
  time_t _epochTime;
  bool _updateDone;

  // Get time from NTP server
  _updateDone = timeClient.update();
  _epochTime = timeClient.getEpochTime();

  if (_updateDone) {
    //NTP update done
    NTP_State = NTP_RECEIVED;
  }
  else {
    //NTP update failed due to timeout
    NTP_State = NTP_FAIL;
  }

  return _epochTime;
}

time_t getLocTime()
//Receive local time
{
  time_t _t;

  //Get time
  _t=now();

  //Convert to local time
  _t = CE.toLocal(_t);

  return _t;
}

//--- String functions to create formateted strings ---------------------------------------------------------------------
String getDateTimeStr(void) {
  //Get date and time in Format YYY-MM-DD hh:mm:ss
  char _datetime[20];
  time_t _t;

  _t = getLocTime();
  sprintf(_datetime, "%4d-%02d-%02d %02d:%02d:%02d", year(_t), month(_t), day(_t), hour(_t), minute(_t), second(_t));
  return String(_datetime);
}

String time2DateTimeStr(time_t t) {
  //Converts time t into format YYY-MM-DD hh:mm:ss
  char _datetime[20];
  sprintf(_datetime, "%4d-%02d-%02d %02d:%02d:%02d", year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(_datetime);
}

String IPAddressStr(IPAddress ipadr)
{
  String _str;
  //Prints IP Address
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    _str = _str + String(ipadr[thisByte]);
    if (thisByte < 3)
      _str = _str + ".";
  }
  return _str;
}

//--- Ethernet functions ----------------------------------------------------------------------------------------------
void EthConnect(void)
{
  #if DHCP_USAGE==1
  // Try to start the Ethernet connection with DHCP
  if (Ethernet.begin(mac) == 0) {
    // If DHCP is not available try backup IP
    Ethernet.begin(mac, ip);
    ETH_StateConnectionType = ETH_BACKUPIP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(MSG_WARNING,F("Failed to configure Ethernet using DHCP, using backup IP"));
    #endif
  }
  else {
    ETH_StateConnectionType = ETH_DHCP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(MSG_INFO,F("DHCP Address received"));
    #endif  
  }

  #else 
  //Use fixed IP Adress
    Ethernet.begin(mac, ip,dns,gateway,subnet);
    ETH_StateConnectionType = ETH_FIXEDIP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(MSG_INFO,F("Using Fixed IP"));
    #endif  
  #endif
  
  // Write Network data to EEPROM
  ip = Ethernet.localIP();
  gateway = Ethernet.gatewayIP();
  subnet = Ethernet.subnetMask();  
  //NetEEPROM.writeNet(mac, ip, gateway, subnet);

  // print your local IP config
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(MSG_INFO,"IP-Address : " + IPAddressStr(ip));
    WriteSystemLog(MSG_DEBUG,"Gateway    : " + IPAddressStr(gateway));
    WriteSystemLog(MSG_DEBUG,"Subnet Mask: " + IPAddressStr(subnet));
  #endif  
  
  delay(1000);
}

void Workaround_CS_ETH2SD(int pos)
//Workaround when ETH CS is not reset before SD CS is activated
{ 
//  File logFile; 
//  String date_time;
//  String LogStr;
//  
//  //Test code to catch web-access stalling issue: 
//  if (digitalRead(ETH__SS_PIN)==LOW) {    
//     //Get Time
//     date_time = getDateTimeStr();
//
//     //Log data to SD Card
//     LogStr = "[";
//     LogStr.concat(date_time);
//     LogStr.concat("] ");
//     LogStr.concat("Trap: ETH__SS_PIN==LOW (ACTIVE) detected before SD-Card access. Position ");
//     LogStr.concat(String(pos));
//     
//     //Workaround: Switch off CS from Eth, and switch on CS from SD
//     digitalWrite(ETH__SS_PIN,HIGH);
//     digitalWrite(SD_CARD_PIN,LOW);
//     
//     //Write to systemlog
//     #if LOGLEVEL & LOGLVL_TRAP
//       logFile = SD.open(LOGFILE, FILE_WRITE);
//       logFile.println(LogStr);
//       logFile.close();    
//     #endif
//  }  
//  else {
//     digitalWrite(SD_CARD_PIN,LOW);
//  }
}

void Workaround_CS_SD2ETH(int pos)
//Workaround when SD CS is not reset before ETH CS is activated
{
//  File logFile; 
//  String date_time;
//  String LogStr;
//  
//  //Test code to catch web-access stalling issue: 
//  if (digitalRead(SD_CARD_PIN)==LOW) {    
//     //Get Time
//     date_time = getDateTimeStr();
//
//     //Log data to SD Card
//     LogStr = "[";
//     LogStr.concat(date_time);
//     LogStr.concat("] ");
//     LogStr.concat("Trap: SD_CARD_PIN==LOW (ACTIVE) after SD-Card access is finished. Position");
//     LogStr.concat(String(pos));
//     
//     //Workaround: Switch off CS from Eth, and switch on CS from SD
//     digitalWrite(ETH__SS_PIN,LOW);
//     digitalWrite(SD_CARD_PIN,HIGH);
//     
//     //Write to systemlog
//     #if LOGLEVEL & LOGLVL_TRAP
//       logFile = SD.open(LOGFILE, FILE_WRITE);
//       logFile.println(LogStr);
//       logFile.close();    
//     #endif
//  }  
//  else {
//     digitalWrite(ETH__SS_PIN,LOW);
//  }    
}    

//--- Reset and watchdog --------------------------------------------------------------------------------------
void resetFunc (void) 
//Reset function
{
  WriteSystemLog(MSG_INFO,"Reset program");  
  cli(); //irq's off
  wdt_enable(WDTO_1S); //wd on,15ms
  while(1); //loop
}

void TriggerWDT(void)
// Trigger watchdog timer
{  
  wdt_reset();
}

//--- Sort and statistic functions -----------------------------------------------------------------------------
void sort()
// Guter alter Bubblesort
{  
  //Serial.println(size);
  for (int i=0; i<(99); i++) {
    for (int o=0; o<(100-(i+1)); o++) {
      if(arr[o] > arr[o+1]) {
        int t = arr[o];
        arr[o] = arr[o+1];
        arr[o+1] = t;
      }
    }
  }
}

int CountValidValues()
// Checks plausibility of array arr
{
  int _i;
  int _cnt=0;
  for (_i = 25; _i < 75; _i++) {  
    if (arr[_i]>0) {
      _cnt++;
    }
  }
  return _cnt;
}

float Mean()
//Mean value
{
  int _i;
  float _mittel=0;
  int _cnt=0;
  
  for (_i = 25; _i < 75; _i++) {
    if (arr[_i]>0) {
      //Take only valid values
      _mittel = _mittel + float(arr[_i]);
      _cnt++;
    }
  }  
  if (_cnt>0) {
    _mittel = _mittel/_cnt;  
  }
  else {
    _mittel = 0;
  }
      
  return _mittel;   
}

double StdDev()
// Standard Deviation
{      
  double _var=0;
  int _i=0;
  int _cnt=0;
  
  for (_i = 25; _i < 75; _i++)
  {
    if (arr[_i]>0) {
      //Take only valid values
      _var = _var + (float(arr[_i])-hMean)*(float(arr[_i])-hMean);  
      _cnt++;
    }    
  }

  if (_cnt>0) {
    _var = sqrt(_var/_cnt);
  }
  else {
    _var=0;
  }

  return _var;
}

//--- Calculations ---------------------------------------------------------------------------------------------------------
unsigned long PositiveDistanceUL(unsigned long tiReceived,unsigned long tiSend)
{
  //calculates positive distance between received time and send time with overflow handling
  unsigned long _ti;
  
  if (tiReceived<tiSend) {
    //Overflow
    _ti = ULONG_MAX + tiReceived - tiSend;
  }
  else {
    _ti = tiReceived - tiSend;
  }
  return _ti;
}

int Curve(int x[],int y[], int siz, int xi)
//Curve to interpolate
{
  unsigned char _i;
  float _fac;
  
  //Find x-position _i which x[x_i]
  for (_i=0;_i<siz;_i++) {
    if (xi<x[_i])
    break;
  }

  //extrapolate lower limit constant
  if (_i==0) {
    return y[0];
  }

  //extrapolate upper limit constant
  if (_i==siz-1) {
    return y[siz-1];
  }

  //interpolate
  _fac = (xi-x[_i-1])/(x[_i]-x[_i-1]);
  return round(y[_i-1] + _fac*(y[_i]-y[_i-1]));  
}

