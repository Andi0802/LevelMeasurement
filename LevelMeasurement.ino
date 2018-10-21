
/*
  Level sensor for rain water reservoir

  Features:
    - Level sensing by US Sensor
    - Refill based on level
    - Web server for settings and monitoring

*/

//Software Version Info
#include "version_info.h"
const String prgVers = PRG_VERS;
const String prgChng = PRG_CHANGE_DESC;

//Libraries
//AVR System libraries Arduino 1.6.7
#include <SPI.h>
#include <EEPROM.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <limits.h>

//Arduino Libraries Arduino 1.6.7
#include <Ethernet.h>  //Update auf Version 2.0.0
#include <Dns.h>
#include <SD.h>

//Local libraries
#include <NTPClient.h>  // https://github.com/arduino-libraries/NTPClient.git Commit 020aaf8
#include <TimeLib.h>    // https://github.com/PaulStoffregen/Time.git Version 1.5+Commit 01083f8

// Switch between operational system and test system
//#define USE_TEST_SYSTEM

//Logging level Bitwise
#define LOGLVL_NORMAL  1  //Bit 0: Normal logging
#define LOGLVL_WEB     2  //Bit 1: Web server access
#define LOGLVL_ALL     4  //Bit 2: Single measurement values
#define LOGLVL_CCU     8  //Bit 3: CCU access
#define LOGLVL_SYSTEM 16  //Bit 4: System logging (NTP etc)
#define LOGLEVEL 1+16  

// SDCard
#define SD_CARD_PIN       4  // CS for SD-Card on ethernet shield
#define SD_BLOCK_SIZE   128  // Size of block to transfer over Network

// Pulse distance for measurement (100ms)
#define PULSE_DIST       5   // Distance of pulses in 100ms

// US Sensor Pins
#define US_TRIGGER_PIN    2  // Trigger pin
#define US_ECHO_PIN       3  // Echo pin, interrupt 1

//Digital input pins
#define BUTTON_PIN        6  // Pin for manual refilling button

// Digital out pins
#define SOLONOID_PIN      7  // Refiller
#define MEASLED_PIN       8  // LED indicating Measurement
#define MEASLEDCOL_PIN    9  // L0=green, HI=red
#define REFLED_PIN        5  // Refiller LED
 
//SD Card States
#define SD_UNDEFINED  0      // Status unknown
#define SD_OK         1      // SD Card ok
#define SD_FAIL      -1      // No SD Card available

// Ethernet Shield
#define DHCP_USAGE 0
#define ETH_SCK_PIN   13     // Pin D13 SCK
#define ETH_MISO_PIN  12     // Pin D12 MISO
#define ETH__MOSI_PIN 11     // Pin D11 /MOSI
#define ETH__SS_PIN   10     // Pin D10 /SS

//Ethernet States
#define ETH_UNDEFINED 0      // Status unknown
#define ETH_BACKUPIP  1      // No DHCP server, Backup IP used
#define ETH_FIXEDIP   2      // Fixed IP used
#define ETH_DHCP      3      // IP by DHCP server used
#define ETH_FAIL     -1      // No Ethernet connection
#define ETH_LOST     -2      // Connection lost

//NTP State
#define NTP_UNDEFINED  0     // Status unknown
#define NTP_RECEIVED   1     // Time received
#define NTP_WAITUPD    2     // Waiting for time update
#define NTP_FAIL      -1     // Error in NTP query

//Files (Max 8+3 Filenames)
#define LOGFILE  F("Res_Ctl.log")     // Name of system logfile
#define DATAFILE F("Res_Ctl.csv")     // Name of data logfile

//Diagnosis
#define SIGNAL_HEALTH_MIN 50          // Minimum signal health required
#define DEBOUNCE_EVT_MAX 3            // debounce count for deb/ok detection
#define ERRNUM_SIGNAL_HEALTH 1        // Signal health too low

//States for Pulses
#define PLS_IDLE          0   // Pulse waiting to be send
#define PLS_SEND          1   // Pulse send, but not received
#define PLS_RECEIVED_POS  2   // Positive edge of echo is received
#define PLS_RECEIVED_NEG  3   // Negative edge of echo is received
#define PLS_TIMEOUT       4   // Timout waiting for echo

//Timeout in us
#define PLS_TI_TIMEOUT 40000

//States for Filter diagnosis
#define FILT_OFF          0   // No Diagnosis done
#define FILT_DIAG         1   // Filter diagnosis done
#define FILT_USAGE        2   // Usage per day calulated

//States for Filter check
#define FILT_ERR_UNKNOWN 0    // Inknown
#define FILT_ERR_NOMEAS  1    // Measurement error
#define FILT_ERR_CLOGGED 2    // Filter clogged
#define FILT_ERR_OK      3    // Filter OK

//Rain detection
#define MAX_VOL_NORAIN  0.300 // Maximum rain in 24h in mm to detect "no rain"

//LED colors
#define GREEN 0
#define RED   1

//Definitions for Homematic CCU
#define HM_DATAPOINT_RAIN24 6614  // Datapoint for 24h rain

//Device name of HM Client
//IP of CCU
byte hm_ccu[] = { 192, 168, 178, 11 };
EthernetClient hm_client;

//Typedefs
struct settings_t {
  unsigned char stat;         //Status of EEPROM
  int cycle_time;             //Calculating base cycle time [s]
  int vSound;                 //Velocity of sound [cm/sec]
  int hOffset;                //Runtime offset [mm]
  int aBaseArea;              //Base area of reservoir [cm*cm]
  int hSensorPos;             //Height of sensor above base area [cm]
  int hOverflow;              //Height of overflow device [cm]
  int hRefill;                //Height for refilling start [cm]
  int volRefill;              //Volume to refill [l]
  int dvolRefill;             //Mass flow of refiller [l/sec]  
  unsigned long volRefillTot;  //Refilling volume total [l]
  unsigned long tiRefillReset; //Timestamp of last reset volRefillTot
  int aRoof;                   //Surface of roof
  int prcFiltEff;              //filter efficiency in % (1-overflow) 
  int prcVolDvtThres;          //Threshold for filter clogging error
  int volRainMin;              //Minimum Rain Volume in 24h to activate filter diagnosis
};

union setting_stream_t {
  settings_t settings;
  unsigned char SettingStream[sizeof(settings_t)];
};

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
#ifdef USE_TEST_SYSTEM
//MAC for testing system
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };

// Set the static IP address if DHCP fails or fixed address is used
IPAddress ip(192, 168, 178, 4);
#else
//MAC for operational system
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEB };

// Set the static IP address if DHCP fails or fixed address is used
IPAddress ip(192, 168, 178, 5);
#endif

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

//UDP Client for NTP
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Web Server
EthernetServer server(80);

//Variables for DNS
DNSClient dnsClient;
byte RouterIP[] = {192, 168, 178, 1};

// --- Globals ---------------------------------------------------------------------------------------------------

//Time
unsigned int Time_100ms = 0;
unsigned int cnt60 = 0;
unsigned int Time_60s = 0;
unsigned int cnt60min = 0;
unsigned int cntPulse = 0;

//US Sensor measurement
unsigned char stPulse = PLS_IDLE;  //state of measurement pulse
unsigned long tiSend;              //Timestamp pulse sent
unsigned long tiReceived;          //Timestamp pulse received
unsigned int dauer=0;              //Duration of pulse single measurement
unsigned int entfernung=0;         //Distance measured single measurement
float hMean=0;                     //Mean value of level
unsigned int volActual=0;          //Actual volume
unsigned int volMax=0;             //Maximum volume
unsigned char  prcActual=0;        //Percentage of maximum volume
unsigned int hWaterActual=0;       //Actual level (average after diagnosis)
String MeasTimeStr ;               //Measurement time as string
unsigned long tiRefillerOff;       //Timestamp refilling off
unsigned long tiRefillerOn;        //Timestamp refilling on
unsigned char stRefill=0;          //Status Refilling in progress
unsigned char stRefillReq=0;       //Request for refilling step based on measurement
double volStdDev;                  //Standard deviation of measurements in volume
unsigned char rSignalHealth=0;     //Signal health (0 worst - 100 best)
char cntValidValues;               //Number of valid values
char cntDiagDeb;                   //Debounce counter diagnosis
unsigned char stError;             //Error status
unsigned char pos = 0;             //Array position to write measurement
int arr[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned char stButton=0;          //Refill button pressed
unsigned char stMeasAct=1;         //Request for measurement, start measurement immediately
unsigned char stFilterCheck=FILT_OFF; //Filter diagnosis
unsigned char stFiltChkErr;        //Result filter check
unsigned char st24hPast=0;         //Values 24h ago available
float volRainDiag24h;              //Rain in 24h for Diagnosis
float volRainDiag24h_old;          //Rain in 24h for Diagnosis 1 h ago
float volRain1h;                   //Rain in last 1h for Diagnosis
unsigned long volRefillFilt0;      //Stored value of Refilling volume 24h past
unsigned int  volActualFilt0;      //Stored actual reservoir volume 24h past
unsigned int  volUsageDiag24h;     //Water usage in 24h for Diagnosis
unsigned int  volRefill24h;        //Refilled volume in 24h
unsigned int  volRefillDiag24h;    //Refilled volume in 24h for Diagnosis
unsigned int  volOld;              //Old volume for tendency 
int volDiff1h;                     //Volume change in 1h (tendency)
float volRain24h;                  //Rain in last 24h
int volDiff24h;                    //Difference volume measurement in 24h
int volDiffDiag24h;                //Difference volume measurement in 24h for Diagnosis
//10 days storage for dayly water usage 
int volUsage24h[10]={32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767};  
int volUsageAvrg10d;               //10 days average of dayly water usage
unsigned char iDay;                //Day counter for moving average
String UsageTimeStr;               //String of last calculation water usage
String DiagTimeStr;                //String of last calculation water usage
int prcVolDvt24h;                  //Deviation between thoretical and real value in %
int volDiffCalc24h;                //Theoretical change of volume
unsigned char cntLED=0;            //Counter for LED 
String inString, IPString;         //Webserver Receive string, IP-Adress string
unsigned char cntTestFilt=0;       //Testing for filter diagnosis

//EEP Data
setting_stream_t SettingsEEP; //EEPData
//Size of EEP Data incl. checksum in byte
int EEPSize=sizeof(SettingsEEP.settings);

//State of components
int SD_State  = SD_UNDEFINED;
int ETH_State = ETH_UNDEFINED;
int ETH_StateConnectionType = ETH_UNDEFINED;
int NTP_State = NTP_UNDEFINED;

//Watchdog: last time the WDT was ACKd by the application
unsigned long lastUpdate = 0;
//Watchdog: time, in ms, after which a reset should be triggered
unsigned long timeout = 30000;

// --- Setup -----------------------------------------------------------------------------------------------------
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  //SD Card initialize
  pinMode(ETH__SS_PIN, OUTPUT);      //Workaround to deselect Wiznet chip
  Workaround_CS_ETH2SD();
  
  if (!SD.begin(SD_CARD_PIN)) {
    SD_State = SD_FAIL;
    WriteSystemLog(F("Initializing SD Card failed"));
  }
  else {
    SD_State = SD_OK;
    WriteSystemLog(F("Initializing SD Card ok"));
  }
 
  // Try Ethernet connection
  EthConnect();

  //Create ethernet server
  server.begin();

  //Set NTP as sync provider
  setSyncProvider(getNtpTime); //Sync provider
  setSyncInterval(3600);       //1 hour updates
  now();                       //Synchronizes clock

  //Read settings from EEPROM
  ReadEEPData();

  //Outputs
  pinMode(SOLONOID_PIN, OUTPUT);
  pinMode(US_TRIGGER_PIN, OUTPUT);
  pinMode(MEASLED_PIN, OUTPUT);
  pinMode(MEASLEDCOL_PIN, OUTPUT);
  pinMode(REFLED_PIN,OUTPUT);
  SetSystemLEDColor(GREEN);
  SetSystemLED(false);
  
  //Inputs
  pinMode(US_ECHO_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(US_ECHO_PIN), ReceivePulse, CHANGE);
  pinMode(BUTTON_PIN,INPUT_PULLUP);

  //Send Message New Start  
  WriteSystemLog(F("System startup"));
  
  //Initialize timer 1 as watchdog
  cli();          // disable global interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B

  // set compare match register to desired timer count:
  //1000ms*16MHz/1024=15625
  OCR1A = 15624;
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  sei();          // enable global interrupts:

  //Initialize timer 4 as global time system
  cli();          // disable global interrupts
  TCCR4A = 0;     // set entire TCCR5A register to 0
  TCCR4B = 0;     // same for TCCR5B

  // set compare match register to desired timer count:
  // 100ms*16MHz/256=6250
  OCR4A = 6250;
  // turn on CTC mode:
  TCCR4B |= (1 << WGM12);
  // Set  CS12 bits for 256 prescaler: 
  TCCR4B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK4 |= (1 << OCIE1A);
  sei();          // enable global interrupts:

  //Reset 100 msec Timer
  Time_100ms = 0;

} // End Setup

// --- ISR -----------------------------------------------------------------------------------------------------
ISR(TIMER4_COMPA_vect)
//ISR for 100msec Timer
{
  if (Time_100ms < 0xFFFF) {
    Time_100ms++;
  }
  else {
    Time_100ms = 0;
  }
}

ISR(TIMER1_COMPA_vect)
// Watchdog timer
{
  if (PositiveDistanceUL(millis(),lastUpdate)> timeout)
  {
    //enable interrupts so serial can work
    sei();

    //detach Timer1 interrupt so that if processing goes long, WDT isn't re-triggered
    TIMSK1 = TIMSK1 & (0xFF & ~(1 << OCIE1A));

    //flush, as Serial is buffered; and on hitting reset that buffer is cleared
    WriteSystemLog(F("WDT triggered"));
    Serial.flush();

    //call to bootloader / code at address 0
    resetFunc();
  }
}

void ReceivePulse(void)
//Receives a pulse on Echo Pin
{
  if (stPulse==PLS_SEND) {
    //Wait for pulse only if a pulse has been send previously
    if (digitalRead(US_ECHO_PIN)) {
      //Send time
      tiSend=micros();
      stPulse = PLS_RECEIVED_POS;
    }
  }
  else if (stPulse==PLS_RECEIVED_POS) {
    if (!digitalRead(US_ECHO_PIN)) {
      //Receive time
      tiReceived=micros();
      stPulse = PLS_RECEIVED_NEG;
    }
  }
}


// --- Loop -----------------------------------------------------------------------------------------------------
void loop()
{
  String strLog;
  byte PulseCntr;
  double _hStdDev;   

  //--- 100ms Task -------------------------------------------------------------------
  if (Time_100ms > 0) {
    Time_100ms--;
  
    //60sec Task 
    if (cnt60 < 600) {
      cnt60++;
    }
    else {
      Time_60s++;
      cnt60 = 0;
    }  
    
    //60min Task 
    if (cnt60min < 60*600) {
      cnt60min++;
    }
    else {
      //Activate measurement
      stMeasAct=1;
      cnt60min = 0;
    }  

    //Pulse Task only if measurement active
    if (stMeasAct) {
      if (cntPulse < PULSE_DIST-1) {
        cntPulse++;
      }
      else {
        PulseCntr++; 
        cntPulse=0;     
      }       
    }

    //Read Button only if refilling action is completed
    if (stButton==0) {
      stButton = !digitalRead(BUTTON_PIN);    
      delay(1);
      stButton = stButton & (!digitalRead(BUTTON_PIN));
      
      //Hint for Refill reason
      if (stButton>0) {
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(F("Refilling requested by manual button"));
        #endif  
      } 
    }
      
    //Check if Refilling is needed
    if ((stRefillReq>0) || (stRefill>0) || (stButton>0)) {          
      stRefill = Refill(int(SettingsEEP.settings.volRefill));
  
      //If refilling is ready and was triggered by button, reset Button
      if ((stButton>0) && (stRefill==0)) {
        stButton=0;      
      }

      //If refilling is ready and was requested by measurement, reset request
      if ((stRefillReq>0) && (stRefill==0)) {
        stRefillReq=0;
      }
    }

    //Level indication LED
    if (cntLED==20) {
      //LED on
      SetSystemLED(true);
      //Reset counter
      cntLED=0;
    } 
    else {
      //Increment counter, conting 0-20 -> Period 2sec
      cntLED++;
    }    
    if (5*cntLED>=prcActual) {
      //LED off
      SetSystemLED(false);
    }
       
    // Trigger Watchdog    
    TriggerWDT();  

    
  } // 100ms Task

  //--- Measurement pulse Task -------------------------------------------------------
  //Start measurement puls
  if (PulseCntr>0) {  
    //Send pulse and measure delay time  
    PulseCntr--;
    if (pos<100) {      
      if (stPulse==PLS_IDLE) {
        //Send a pulse only if previous pulse has beed received                
        digitalWrite(US_TRIGGER_PIN, HIGH);        
        delay(2);
        digitalWrite(US_TRIGGER_PIN, LOW);                  
        stPulse = PLS_SEND;
      }
      else if (stPulse==PLS_RECEIVED_NEG) {
        //Calculated distance only if pulse has been received
        dauer = PositiveDistanceUL(tiReceived,tiSend);        
  
        //Calculate distance in mm
        entfernung = ((unsigned long)(dauer/2) * SettingsEEP.settings.vSound)/10000 - SettingsEEP.settings.hOffset;

        #if LOGLEVEL & LOGLVL_ALL
          WriteSystemLog("Duration measured: " + String(dauer) + "us Send: " + String(tiSend) + " Received: " + String(tiReceived)+ " Entfernung: " + String(entfernung) + " mm");
        #endif
        
        //Plausibility check: Signal cannot be higher than sensor position and not lower than distance btw sensor and overflow  
        if (entfernung >= SettingsEEP.settings.hSensorPos*10 || entfernung <= (SettingsEEP.settings.hSensorPos-SettingsEEP.settings.hOverflow)*10)
        {
          //Measurement not plausible, set dummy value 0
          arr[pos] = 0;
        }
        else
        {
          //Measurement plausible, collect measurement
          arr[pos] = entfernung;
        }
        pos++;
        stPulse = PLS_IDLE;
      }
      else if (stPulse==PLS_RECEIVED_POS) {
        //Check for timeout
        if (PositiveDistanceUL(micros(),tiSend)>PLS_TI_TIMEOUT) {
          //Timeout detected
          #if LOGLEVEL & LOGLVL_NORMAL
            WriteSystemLog(F("Timeout US measurement detected"));
          #endif  
          dauer = PLS_TI_TIMEOUT;
          entfernung = 0;
          arr[pos]=0;
          pos++;
          stPulse = PLS_IDLE;
        }
      }
    }      
    else {
      // Evaluate measured delay times      
      //Capture measurement time
      MeasTimeStr = getDateTimeStr(); 

      //If 100 measurements are done calculate new result
      // Handle array arr with 100 measurements
      //sort values
      sort();      

      // Count values >0
      cntValidValues = CountValidValues();

      //Calculate average and StdDev, discard 25% of lowest and highest values
      hMean = Mean();      
      _hStdDev = StdDev();
      volStdDev = Height2Volume(_hStdDev);

      //Calculate maximum Volume in liter
      volMax = (long (SettingsEEP.settings.aBaseArea) * long (SettingsEEP.settings.hOverflow))/1000;      

      //Plausibility check of signal
      // Minimum 40 of 50 values are valid
      // minimum 50liter stdDev is reached
      if (cntValidValues>40) {
        rSignalHealth = 100 - 6*volStdDev/volMax*1000;      
        if (rSignalHealth<0) {
          rSignalHealth=0;
        }
      }
      else {
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog("Only " + String(cntValidValues) + " valid measurement values detected (Threshold=40), no new volume calculated");
        #endif
        rSignalHealth = 1;
      }
      
      //Convert to volume
      if (rSignalHealth>50) {
        hWaterActual = SettingsEEP.settings.hSensorPos*10-hMean;      
        //Remember old volume
        volOld = volActual;
        //new volume
        volActual = Height2Volume(hWaterActual);
        //difference last hour
        volDiff1h = volActual-volOld;
      }
                  
      //Relative Level in %
      prcActual = ((long) volActual*100)/volMax;      
      
      //Diagnostic debounce counter
      if (rSignalHealth<SIGNAL_HEALTH_MIN) {
        if (cntDiagDeb<=DEBOUNCE_EVT_MAX) {
          cntDiagDeb++;
          #if LOGLEVEL & LOGLVL_NORMAL
            WriteSystemLog("Error detected, debouncing "+String(cntDiagDeb));
          #endif 
          
          //Trigger next measurement immediately
          stMeasAct = 1;
        }      
        else {
          //Error detected
          SetError(ERRNUM_SIGNAL_HEALTH,true);
          
          //Stop measuerement
          stMeasAct=0;
        }
      }        
      else {
        if (cntDiagDeb>0) {      
          cntDiagDeb--;
          
          #if LOGLEVEL & LOGLVL_NORMAL
            WriteSystemLog("Good signal detected, debouncing "+String(cntDiagDeb));
          #endif 

          //Trigger next measurement immediately
          stMeasAct = 1;
        }
        else {
          //Error healed
          SetError(ERRNUM_SIGNAL_HEALTH,false);

          //Stop measuerement
          stMeasAct=0;
        }
      }

      //Write Log File
      #if LOGLEVEL & LOGLVL_NORMAL
        WriteSystemLog("Measured distance: " + String(hMean) +" mm");
        WriteSystemLog("Actual Level     : " + String(hWaterActual) + " mm");
        WriteSystemLog("Actual volume    : " + String(volActual) + " Liter, StdDev: " +String(volStdDev) + " Liter");
        WriteSystemLog("Error status     : " + String(stError));
      #endif          
      
      //Get rain volume for last 24h
      volRainDiag24h_old = volRain24h;
      volRain24h = hm_get_datapoint(HM_DATAPOINT_RAIN24);
      //Offen: Updatezeit lesen, Datenpunktnummer ermitteln
      //falls Wert nicht aktuell: Fehler: keine Aktuelle Regenmenge     

      //Calculate rain within last 1h
      volRain1h = max(volRain24h-volRainDiag24h_old,0);

       #if LOGLEVEL & LOGLVL_NORMAL
        WriteSystemLog("Rain in last 24h from weather station [mm]: "+String(volRain24h));
        WriteSystemLog("Rain in last  1h from weather station [mm]: "+String(volRain1h));
      #endif

      //Check filter if rain within last hour is above threshold and volume is below maximum volume
      if ((volRain1h>SettingsEEP.settings.volRainMin) && (volActual<volMax)) {
        CheckFilter();
      }

      //Calcluate usage per day if hour=0 and rain within last 24h is zero
      if ((volRain24h<MAX_VOL_NORAIN) & hour()==0) {
        CalculateDailyUsage();
      }           
      
      //Log data 
      LogData();
      
      //Send data to Homematic CCU
      hm_set_sysvar(F("HM_ZISTERNE_volActual"), volActual);
      delay(1); //nicht optimal, aber sonst stürzt HM ab
      hm_set_sysvar(F("HM_ZISTERNE_rSignalHealth"), rSignalHealth);
      delay(1); //nicht optimal, aber sonst stürzt HM ab
      hm_set_sysvar(F("HM_ZISTERNE_prcActual"), prcActual);           
      delay(1); //nicht optimal, aber sonst stürzt HM ab
      hm_set_sysvar(F("HM_ZISTERNE_stError"), stError);          
      
      //Reset pointer for next data collection
      pos = 0;

      //Reset last evaluation type Filter and usage
      stFilterCheck = FILT_OFF;

      // Check if refilling is required
      stRefillReq = ((hWaterActual<SettingsEEP.settings.hRefill*10) && (rSignalHealth>SIGNAL_HEALTH_MIN));
      if (stRefillReq) {
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(F("Refilling required"));
        #endif
      }
      #if TEST_FILT>0
        // Directly start next measurement
        stMeasAct=1;
      #endif
    }        
  }   
  
  //--- 60s Task ---------------------------------------------------------------------
  if (Time_60s > 0) {
    Time_60s--;

    //Monitor NTP
    if (NTP_State == NTP_RECEIVED) {
      //Write log entry and set State to WaitNextUpdate
      #if LOGLEVEL & LOGLVL_SYSTEM
        WriteSystemLog(F("Time updated by NTP"));
      #endif
      NTP_State = NTP_WAITUPD;
    }   
  } //60s Task

  //=== Idle Task ===
  //Activate web server for parameter settings and monitoring
  MonitorWebServer(); 

  //Update time
  now();

}


// --- Functions -----------------------------------------------------------------------------------------------------
void CalculateDailyUsage() {
    byte _id;  //Day counter for average
    
    //Remember type of evaluation
    stFilterCheck = FILT_USAGE;
    
    //Calculate water usage      
    //Take timestamp
    UsageTimeStr = getDateTimeStr(); 

    //Calculate dayly refilling volume
    volRefill24h = PositiveDistanceUL(SettingsEEP.settings.volRefillTot,volRefillFilt0);

    //Difference Volume
    volDiff24h =  volActual - volActualFilt0;
    
    //Calculate dayly water usage amd store in array
    iDay=(iDay+1)%10;
    volUsage24h[iDay] =  volRefill24h - volDiff24h;  
    

    //Average over 10 days
    volUsageAvrg10d=0;
    for (_id=0;_id<10;_id++) {
      //Check if init value 32767 Liter is there and replace by currently measured value
      if (volUsage24h[_id]==32767) {
        volUsage24h[_id] = volUsage24h[iDay];
      }
      volUsageAvrg10d = volUsageAvrg10d + volUsage24h[_id];
    }
    volUsageAvrg10d = volUsageAvrg10d/10;       

    //Send result to Homematic
    hm_set_sysvar(F("HM_ZISTERNE_volUsage"), volUsage24h[iDay]);           
    delay(1); //nicht optimal, aber sonst stürzt HM ab
    //Send result to Homematic
    hm_set_sysvar(F("HM_ZISTERNE_volUsage10d"), volUsageAvrg10d);                   
}

void CheckFilter() {
    long _volDvt24h;      //Deviation between theoretical change and real change    
      
    //Remember type of evaluation
    stFilterCheck = FILT_DIAG;
    
    //Calculate filter diagnosis
    //Take timestamp
    DiagTimeStr = getDateTimeStr(); 
    
    //Calculate dayly refilling volume
    volRefillDiag24h = SettingsEEP.settings.volRefillTot - volRefillFilt0;
    
    //Difference Volume
    volDiffDiag24h =  volActual - volActualFilt0;
    
    //Take over Rain measurement
    volRainDiag24h = volRain24h;
    
    //Theoretical volume change
    volDiffCalc24h = volRainDiag24h*SettingsEEP.settings.aRoof*SettingsEEP.settings.prcFiltEff/100 + volRefillDiag24h - volUsageAvrg10d;
    
    //Difference between calculated and real value
    _volDvt24h = volDiffCalc24h - volDiffDiag24h;
    
    //Relative deviation to theoretical quantity
    prcVolDvt24h = (100*_volDvt24h)/volDiffCalc24h;
    
    //Diagnois result
    if (prcVolDvt24h>SettingsEEP.settings.prcVolDvtThres) {
      stFiltChkErr = FILT_ERR_CLOGGED;
    }
    else {
      stFiltChkErr = FILT_ERR_OK;
    }

    //Send result to Homematic
    hm_set_sysvar(F("HM_ZISTERNE_stFiltChkErr"), stFiltChkErr);           
    //delay(1); //nicht optimal, aber sonst stürzt HM ab         
    
    //Reset Counters and store actual values
    volRefillFilt0 = SettingsEEP.settings.volRefillTot;      
    if (rSignalHealth>50) {
      //Take actual volume only if actual measurement is valid
      volActualFilt0 = volActual;
      st24hPast = 1;                
    }
} 

void SetError(int numErr,int stErr)
//Sets or resets error in system
{
  String _txtError;  
  String _envInfo;

  if (stError!=stErr) {
    //Only if there is a change in error status   
    //Define Type
    if (stErr) {
      _txtError = "ERROR: ";
    }
    else {
      _txtError = "HEALED: ";
    }
  
    // Insert error number
    _txtError = _txtError + String(numErr) + " - ";
  
    //Define Error text
    switch(numErr)  {
      case 1: _txtError = _txtError  + F("Bad signal health");
      break;
    }  
  
    //Define environment info
      _envInfo = ""; //F("Measured distance: ") + String(hMean) +F(" mm\n") + F("Actual Level: ") + String(hWaterActual) + F(" mm\n") + F("Actual volume: ") + String(volActual) + F(" Liter, StdDev: ") +String(volStdDev) + F(" Liter\nSignal health: ") + String(rSignalHealth) + F("\nError status: ") + String(stError); 
  
  
    //LED to red
    if (stErr) {
      SetSystemLEDColor(RED);
    }
    else {
      SetSystemLEDColor(GREEN);
    }
    
    //Report to LogFile
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(_txtError);
      WriteSystemLog(_envInfo);
    #endif
        
    //Set global Error information
    stError = stErr;
  }  
}

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

String getDateTimeStr(void) {
  //Get date and time in Format YYY-MM-DD hh:mm:ss
  char _datetime[20];
  sprintf(_datetime, "%4d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  return String(_datetime);
}

String time2DateTimeStr(time_t t) {
  //Converts time t into format YYY-MM-DD hh:mm:ss
  char _datetime[20];
  sprintf(_datetime, "%4d-%02d-%02d %02d:%02d:%02d", year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(_datetime);
}

String IPAddressStr()
{
  String _str;
  //Prints IP Address
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    _str = _str + String(Ethernet.localIP()[thisByte]);
    if (thisByte < 3)
      _str = _str + ".";
  }
  return _str;
}

unsigned char getOption(String str, String Option, double minVal, double maxVal, int *curVal )
{
  // Read options from HTTP GET request  
  int idxOption, lenOption, idxOptionEnd;
  double numberIn;
  String strRem;
  unsigned char stResult;
  String strLog;

  // Is Option available?
  strLog = "Option " + Option;
  strLog = strLog + ": ";

  Option = Option + "=";
  idxOption = str.indexOf(Option);
  lenOption = Option.length();
  if (idxOption > 0) {
    //Option found, Check remaining string for end of option
    strRem = str.substring(idxOption + lenOption - 1);
    idxOptionEnd = strRem.indexOf("&");
    if (idxOptionEnd < 0) {
      //Last argument
      idxOptionEnd = strRem.indexOf(" HTTP");
    }
    #if LOGLEVEL & LOGLVL_WEB
      Serial.println(F("Remaining string     : ") + strRem);
      Serial.println(F("Start index of option: ") + String(idxOption));
      Serial.println(F("End index of option  : ") + String(idxOption+idxOptionEnd));
    #endif

    if (idxOptionEnd == 1) {
      //Empty option, keep current value
      numberIn = *curVal;
      stResult = 0;
      strLog = strLog + "empty";
    }
    else {
      //Get value of option
      strRem = strRem.substring(1, idxOptionEnd);
      if (strRem.equals("on")) {
        numberIn = 1;
      }
      else {
        numberIn = strRem.toFloat();
      }
      
      //Serial.print("New value raw ");
      //Serial.println(strRem);

      //Check range
      if ((numberIn >= minVal) && (numberIn <= maxVal)) {
        strLog = strLog + F("New value ");
        strLog = strLog + numberIn;
        stResult = 1;
      }
      else {
        strLog = strLog + F("Value out of range (");
        strLog = strLog + numberIn;
        strLog = strLog + F(") keeping old value ");
        strLog = strLog + *curVal;
        numberIn = *curVal;
        stResult = 0;
      }
    }
  }
  else {
    numberIn = *curVal;
    strLog = strLog + F("No option found, keeping old value  ");
    strLog = strLog + numberIn;
    stResult = 0;
  }
  *curVal = numberIn;

  //Log entry
  #if LOGLEVEL & LOGLVL_WEB
    WriteSystemLog(strLog);
  #endif

  return stResult;
}


void hm_set_sysvar(String parameter, double value)
// Send information to Homematic CCU
// Using XML-RPC
{
  String _cmd;
  int _stConnection;

  //Generate command
  _cmd = F("GET /xy.exe?antwort=dom.GetObject('");
  _cmd = _cmd + parameter;
  _cmd = _cmd + F("').State(");
  _cmd = _cmd + value;
  _cmd = _cmd + F(")");

  //Connect and send
  _stConnection=hm_client.connect(hm_ccu, 8181);
  if (_stConnection) {    
    hm_client.println(_cmd);
    hm_client.println();
    hm_client.stop();
    #if LOGLEVEL & LOGLVL_CCU
      WriteSystemLog(_cmd);
    #endif
  } else {
    WriteSystemLog(F("Connection to CCU failed"));    
    WriteSystemLog("Result: " + String(_stConnection));
  }
}

void hm_set_state(int ise_id, double value)
// Change state of channel in Homematic CCU
// Use XML-API; use ise_id of channel
{
  String _cmd;

  //Generate command
  _cmd = "GET /config/xmlapi/statechange.cgi?ise_id=";
  _cmd = _cmd + ise_id;
  _cmd = _cmd + "&new_value=";
  _cmd = _cmd + value;

  //Connect and send
  if (hm_client.connect(hm_ccu, 80)) {    
    hm_client.println(_cmd);
    hm_client.println();
    hm_client.stop();
    #if LOGLEVEL & LOGLVL_CCU
      WriteSystemLog(_cmd);
    #endif  
  } else {    
    WriteSystemLog(F("Connection to CCU failed"));
  }
}

void hm_run_program(int ise_id)
// Runs program in Homematic CCU
// Use XML-API; use ise_id of program
{
  String _cmd;

  //Generate command
  _cmd = F("GET /config/xmlapi/runprogram.cgi?ise_id=");
  _cmd = _cmd + ise_id;

  //Connect and send
  if (hm_client.connect(hm_ccu, 80)) {    
    hm_client.println(_cmd);
    hm_client.println();
    hm_client.stop();
    #if LOGVEVEL & LOGLVL_CCU
      WriteSystemLog(_cmd);
    #endif
  } else {    
      WriteSystemLog(F("Connection to CCU failed"));
  }
}

double hm_get_datapoint(int dp_number)
// Gets information to Homematic CCU
// Using XML-API
{
  String _cmd, _ret, _cmpStr, _repStr;
  int idxStart, idxEnd;
  char c;
  int tOutCtr;

  //Generate command
  _cmd = F("GET /config/xmlapi/state.cgi?datapoint_id=");
  _cmd = _cmd + dp_number;
  _cmd = _cmd + F(" HTTP/1.1");

  //Connect and send
  if (hm_client.connect(hm_ccu, 80)) {
    //WriteSystemLog(_cmd);
    hm_client.println(_cmd);
    hm_client.print(F("Host: "));
    hm_client.print(hm_ccu[0]); hm_client.print(F("."));
    hm_client.print(hm_ccu[1]); hm_client.print(F("."));
    hm_client.print(hm_ccu[2]); hm_client.print(F("."));
    hm_client.println(hm_ccu[3]);
    hm_client.println(F("Connection: close"));
    hm_client.println();

    unsigned long _timeStart=micros();
    while (hm_client.connected()) {
        if (hm_client.available()) {
          char c = hm_client.read();
  
          //read char by char HTTP request
          if (_ret.length() < 1024) {
  
            //store characters to string
            _ret += c;           
          }
       }
    }
    hm_client.stop();
    unsigned long _timeStop=micros();
    //WriteSystemLog("<ANTWORT>"+_ret+"</ANTWORT>");
    //WriteSystemLog("Time: "+String(_timeStop-_timeStart)+" us");
    
    //Analyse return string
    _cmpStr = F("<datapoint ise_id='");
    _cmpStr = _cmpStr + dp_number;
    _cmpStr = _cmpStr + F("' value='");
    idxStart = _ret.indexOf(_cmpStr) + _cmpStr.length();
    _cmpStr = F("'/></state>");
    idxEnd = _ret.indexOf(_cmpStr);
    if ((idxStart > -1) && (idxEnd > -1)) {
      _ret = _ret.substring(idxStart, idxEnd);
      _repStr = F("Datapoint ");
      _repStr = _repStr + dp_number;
      _repStr = _repStr + F(" received ") + _ret;
      #if LOGLEVEL & LOGLVL_CCU
        WriteSystemLog(_repStr);
      #endif
    }
    else {
      _ret = F("NaN");
      WriteSystemLog(F("Transmission error while receiving datapoint"));
      WriteSystemLog(_ret);
    }   
  } else {
    WriteSystemLog(F("Connection to CCU failed"));
    _ret = F("NaN");
  }
  return _ret.toFloat();
}

void LogData(void)
//Logs data to SD
{
  File logFile;
  int _i;

  //Log data to SD Card
  if (SD_State == SD_OK) {
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(F("Writing measurement data to SD Card"));
    #endif
        
    Workaround_CS_ETH2SD();    
    
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
      logFile.print(volRefill24h);      logFile.print(F(";"));      
      logFile.print(volDiffDiag24h);    logFile.print(F(";"));
      logFile.print(volDiffCalc24h);    logFile.print(F(";"));
      logFile.print(prcVolDvt24h);      logFile.print(F(";"));
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
      logFile.print(F(";;;;;;"));            
    }

    //Check if Usage log is required
    if (stFilterCheck==FILT_USAGE) {
      logFile.print(volUsage24h[iDay]); logFile.print(F(";"));
      logFile.print(volUsageAvrg10d);   logFile.print(F(";"));      
    }
    else {
      //Empty cells
      logFile.print(F(";;"));            
    }
    
    //Newline and Close
    logFile.println();
    logFile.close();

    Workaround_CS_SD2ETH();   
  
  }
  else {
    WriteSystemLog(F("Writing to SD Card failed"));
  }
}

void LogDataHeader(void)
//Write log data headline to SD
{
  File logFile;

  //Log data to SD Card
  if (SD_State == SD_OK) {
    #if LOGLEVEL & LOGLVL_NORMAL
      WriteSystemLog(F("Writing headline for measurement data to SD Card"));
    #endif  

    Workaround_CS_ETH2SD();    
          
    logFile = SD.open(DATAFILE, FILE_WRITE);  
    logFile.print(F("Date;"));
    logFile.print(F("Level [mm];"));
    logFile.print(F("Quantity [Liter];"));
    logFile.print(F("Signal health [-];"));
    logFile.print(F("Error [-];"));
    logFile.print(F("StdDev [Liter];"));
    logFile.print(F("Refill Quantity [Liter];"));  
    logFile.print(F("Rain quantity 24h [Liter];"));  
    logFile.print(F("Refill quantity 24h [Liter];"));  
    logFile.print(F("Measured quantity change 24h [Liter];")); 
    logFile.print(F("Calculated quantity change 24h [Liter];"));   
    logFile.print(F("Quantity difference [%];"));   
    logFile.print(F("Filter diagnosis [-];"));   
    logFile.print(F("Usage in last 24h [Liter];"));   
    logFile.print(F("Usage average in last 10d [Liter]"));   
    logFile.println();      
    logFile.close();
    
    Workaround_CS_SD2ETH();
  }
  else {
    WriteSystemLog(F("Writing to SD Card failed"));
  }
}

void MonitorWebServer(void)
{
  unsigned int stOption;
  byte clientBuf[SD_BLOCK_SIZE];
  int clientCount = 0;

  //Web server code to change settings
  // Create a client connection
  EthernetClient client2 = server.available();
  if (client2) {
    while (client2.connected()) {
      if (client2.available()) {
        char c = client2.read();

        //read char by char HTTP request
        if (inString.length() < 255) {

          //store characters to string
          inString += c;
          //Serial.print(c);
        }

        //if HTTP request has ended
        if (c == '\n') {
          //print to serial monitor for debuging
          String pageStr = "";
          if (inString.indexOf("?") < 0) {
            //Serial.println(inString);
            pageStr = inString.substring(5, inString.indexOf(" HTTP"));
          }
          #if LOGLEVEL & LOGLVL_WEB
            WriteSystemLog("HTTP Request receives: " + pageStr);
            WriteSystemLog(inString);
          #endif 
          
          //Query options
          stOption = 0;
          stOption |= getOption(inString, "vSoundSet", 3000, 3500, &SettingsEEP.settings.vSound) << 2;
          stOption |= getOption(inString, "hOffsetSet", -500, 500, &SettingsEEP.settings.hOffset) << 3;
          stOption |= getOption(inString, "aBaseAreaSet", 25000, 35000, &SettingsEEP.settings.aBaseArea) << 4;
          stOption |= getOption(inString, "hSensorPosSet", 205, 300, &SettingsEEP.settings.hSensorPos) << 5;
          stOption |= getOption(inString, "hOverflowSet", 180, 220, &SettingsEEP.settings.hOverflow) << 6;
          stOption |= getOption(inString, "hRefillSet", 10, 60, &SettingsEEP.settings.hRefill) << 7;
          stOption |= getOption(inString, "volRefillSet", 10, 50, &SettingsEEP.settings.volRefill) << 8;
          stOption |= getOption(inString, "dvolRefillSet", 2, 20, &SettingsEEP.settings.dvolRefill) << 9;          
          stOption |= getOption(inString, "prcVolDvtThres", 0, 500, &SettingsEEP.settings.prcVolDvtThres) << 10;          
          stOption |= getOption(inString, "aRoof", 0, 500, &SettingsEEP.settings.aRoof) << 11;
          stOption |= getOption(inString, "prcFiltEff", 0, 65535, &SettingsEEP.settings.prcFiltEff) << 12;
          stOption |= getOption(inString, "volRainMin", 0, 10, &SettingsEEP.settings.volRainMin) << 13;
          
   
          //New option received
          if (stOption > 0) {
            // Write to EEPROM
            WriteEEPData();          
          }

          //clearing string for next read
          inString = "";

          if (pageStr.length() == 0) {
            // Print main page

            client2.println(F("HTTP/1.1 200 OK")); //send new page
            client2.println(F("Content-Type: text/html"));
            client2.println(F("Connection: close"));
            client2.println();

            client2.println(F("<HTML><HEAD>"));            
            client2.println(F("<TITLE>Zisterne</TITLE>"));
            client2.println(F("<style>table, td, th { border: 1px solid black; }</style>"));
            client2.println(F("</HEAD>"));
            client2.println(F("<BODY>"));           
            client2.println(F("<H1>Zisterne</h1>"));            

            //Status Info
            client2.println(F("<H2>Status</h2>"));
            client2.println(F("<H3>Messwerte</h3>"));
            client2.print(F("<table><tr><td>Messzeit (UTC)</td><td>"));
            client2.print(MeasTimeStr);
            client2.print(F("</td></tr>"));
            client2.print(F("<tr><td>Messung aktiv [%]</td><td>"));
            if (stMeasAct) {
              client2.print(String(pos));  
            }
            else {
              client2.print(F("-"));  
            }
            
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Pegel [mm]</td><td>"));
            client2.print(String(hWaterActual));
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Inhalt [Liter] </td><td>"));
            client2.print(volActual);
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Tendenz [Liter] </td><td>"));
            client2.print(volDiff1h);
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Standardabweichung [Liter] </td><td>"));
            client2.print(volStdDev);
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Fuellstand [%]</td><td>"));
            client2.print(prcActual);
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Signalqualitaet [-]</td><td>"));
            client2.print(String(rSignalHealth));            
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Fehlerzustand [-]</td><td>"));
            if (stError) {
              client2.print(F("Fehler erkannt"));            
            }
            else {
              client2.print(F("Kein Fehler erkannt"));
            }
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Regenmenge 24h [Liter]</td><td>"));
            client2.print(String(volRain24h));            
            client2.print(F("</td></tr></table>"));  
            client2.println(F("<H3>Nachspeisung</h3>"));
            client2.print(F("<table><tr><td>Nachspeisung aktiv</td><td>"));
            if (stRefill) {
              if (stButton) {
                client2.print(F("manuell"));  
              }
              else {
                client2.print(F("automatisch"));  
              }
            }
            else {
              client2.print(F("-"));  
            }
            
            client2.print(F("</td></tr>"));                     
            client2.print(F("<tr><td>Nachspeisung [Liter]</td><td>"));
            client2.print(String(SettingsEEP.settings.volRefillTot));
            client2.print(F("</td></tr>"));                     
            client2.print(F("<tr><td>Reset Nachspeisung am</td><td>"));
            client2.print(String(time2DateTimeStr(SettingsEEP.settings.tiRefillReset)));
            client2.print(F("</td></tr></table>"));                                 

            //Filter diagnosis
            client2.println(F("<H3>Filterdiagnose</h3>"));
            client2.print(F("<table><tr><td>Diagnosezeit (UTC)</td><td>"));
            client2.print(DiagTimeStr);
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Regenmenge 24h [mm]</td><td>"));
            client2.print(String(volRainDiag24h));
            client2.print(F("</td></tr>"));  
            client2.print(F("<tr><td>Nachspeisung 24h [Liter] </td><td>"));
            client2.print(String(volRefillDiag24h));
            client2.print(F("</td></tr>"));          
            client2.print(F("<tr><td>Verbrauch 24h [Liter]</td><td>"));
            client2.print(String(volUsageDiag24h));
            client2.print(F("</td></tr>"));                
            client2.print(F("<tr><td>Fuellstandsaenderung 24h [Liter] </td><td>"));
            client2.print(String(volDiffDiag24h));            
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Berechnete Fuellstandsaenderung 24h [Liter]</td><td>"));
            client2.print(String(volDiffCalc24h));
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Abweichung [%]</td><td>"));
            client2.print(String(prcVolDvt24h));
            client2.print(F("</td></tr>"));                          
            client2.print(F("<tr><td>Filterzustand [-]</td><td>"));
            switch (stFiltChkErr) {
              case FILT_ERR_UNKNOWN:
                client2.print(F("Unbekannt"));            
                break;
              case FILT_ERR_NOMEAS:
                client2.print(F("Ungueltige Pegelmessung"));            
                break;
              case FILT_ERR_CLOGGED:
                client2.print(F("Verstopft"));            
                break;
              case FILT_ERR_OK:
              client2.print(F("OK"));            
            }            
            client2.print(F("</td></tr>"));              
            client2.print(F("</td></tr></table>"));  

            //Verbrauchsberechnung
            client2.println(F("<H3>Verbrauchsberechnung</h3>"));
            client2.print(F("<table><tr><td>Berechungszeit (UTC)</td><td>"));
            client2.print(UsageTimeStr);
            client2.print(F("</td></tr>"));                        
            client2.print(F("<tr><td>Nachspeisung 24h [Liter] </td><td>"));
            client2.print(String(volRefill24h));
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Fuellstandsaenderung 24h [Liter] </td><td>"));
            client2.print(String(volDiff24h));
            client2.print(F("</td></tr>"));            
            client2.print(F("<tr><td>Verbrauch 24h [Liter]</td><td>"));
            client2.print(String(volUsage24h[iDay]));
            client2.print(F("</td></tr>"));              
            client2.print(F("<tr><td>Gleitender Mittelwert 10 Tage [Liter]</td><td>"));
            client2.print(String(volUsageAvrg10d));            
            client2.print(F("</td></tr>"));              
            client2.print(F("</td></tr></table>"));  
            
            //Settings          
            client2.println(F("<H2>Einstellungen</H2>"));
            client2.println(F("<form method='get'>"));
            client2.print(F("<table><tr><th>Parameter</th><th>Aktueller Wert</th><th>Neuer Wert</th></tr>"));            
            
            client2.print(F("<tr><td>Schallgeschwindigkeit [mm/s]: </td><td>"));
            client2.print(SettingsEEP.settings.vSound);            
            client2.println(F("</td><td> <input type='text' name='vSoundSet'></td></tr>"));

            client2.print(F("<tr><td>Messoffset [mm]: </td><td>"));
            client2.print(SettingsEEP.settings.hOffset);            
            client2.println(F("</td><td> <input type='text' name='hOffsetSet'></td></tr>"));
            
            client2.print(F("<tr><td>Grundflaeche [cm^2]: </td><td>"));
            client2.print(SettingsEEP.settings.aBaseArea);            
            client2.println(F("</td><td> <input type='text' name='aBaseAreaSet'></td></tr>"));

            client2.print(F("<tr><td>Sensor Hoehe [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hSensorPos);            
            client2.println(F("</td><td> <input type='text' name='hSensorPosSet'></td></tr>"));

            client2.print(F("<tr><td>Ueberlauf Hoehe [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hOverflow);            
            client2.println(F("</td><td> <input type='text' name='hOverflowSet'></td></tr>"));

            client2.print(F("<tr><td>Pegel fuer Nachspeisung [cm]: </td><td>"));
            client2.print(SettingsEEP.settings.hRefill);            
            client2.println(F("</td><td> <input type='text' name='hRefillSet'></td></tr>"));

            client2.print(F("<tr><td>Dachflaeche [m^2]: </td><td>"));
            client2.print(SettingsEEP.settings.aRoof);            
            client2.println(F("</td><td> <input type='text' name='aRoof'></td></tr>"));

            client2.print(F("<tr><td>Filtereffizienz [%]: </td><td>"));
            client2.print(SettingsEEP.settings.prcFiltEff);            
            client2.println(F("</td><td> <input type='text' name='prcFiltEff'></td></tr>"));
            
            client2.print(F("<tr><td>Schwellwert Diagnose Filter [%]: </td><td>"));
            client2.print(SettingsEEP.settings.prcVolDvtThres);            
            client2.println(F("</td><td> <input type='text' name='prcVolDvtThres'></td></tr>"));                
            
            client2.print(F("<tr><td>Nachspeisevolumen [l]: </td><td>"));
            client2.print(SettingsEEP.settings.volRefill);            
            client2.println(F("</td><td> <input type='text' name='volRefillSet'></td></tr>"));

            client2.print(F("<tr><td>Nachspeisung Volumenstrom [l/min]: </td><td>"));
            client2.print(SettingsEEP.settings.dvolRefill);            
            client2.println(F("</td><td> <input type='text' name='dvolRefillSet'></td></tr>"));                

            client2.print(F("<tr><td>Minimal erforderliche Regenmenge in 24h fuer Filterdiagnose [mm]: </td><td>"));
            client2.print(SettingsEEP.settings.volRainMin);            
            client2.println(F("</td><td> <input type='text' name='volRainMin'></td></tr>"));                
            
            // Submit button
            client2.println(F("</table><br><input type='submit' value='Submit'>"));

            //Data logger file
            client2.print(F("<h2>Logfiles</h2><a href='"));
            client2.print(DATAFILE);
            client2.print(F("' type='text/text' download='"));
            client2.print(DATAFILE);
            client2.print(F("'>Get Data Log File</a><br>"));

            //System log File
            client2.print(F("<a href='"));
            client2.print(LOGFILE);
            client2.print(F("' type='text/text' download='"));
            client2.print(LOGFILE);
            client2.print(F("'>Get System Log File</a>"));

            //Version Info
            client2.println(F("<H2>Programminfo</h2>"));
            client2.println("<p>" + prgVers + "</p>");
            client2.println("<p>" + prgChng + "</p>");
            
            client2.println(F("</BODY></HTML>\n"));            
            client2.println();
          }

          else {
            File webFile = SD.open(pageStr);        // open file
            client2.println(F("HTTP/1.1 200 OK"));
            client2.println(F("Content-Type: text/comma-separated-values"));
            client2.println(F("Content-Disposition: attachment;"));
            client2.println(F("Connection: close"));
            client2.println();

            if (webFile)
            {
              while (webFile.available())
              {
                //client2.write(webFile.read()); // send file to client byte by byte (slow)
                clientBuf[clientCount] = webFile.read();
                clientCount++;

                if (clientCount > SD_BLOCK_SIZE - 1)
                {
                  // Serial.println("Packet");
                  client2.write(clientBuf, SD_BLOCK_SIZE);
                  clientCount = 0;
                }
                TriggerWDT();  // Trigger Watchdog
              }
              //final <SD_BLOCK_SIZE byte cleanup packet
              if (clientCount > 0) client2.write(clientBuf, clientCount);
              webFile.close();

              //Remove downloaded file
              SD.remove(pageStr);
              if (pageStr.equals(DATAFILE)) {
                // Create new headline
                LogDataHeader();
              }
            }

            delay(1);                    // give the web browser time to receive the data

          }


          delay(1);
          //stopping client
          client2.stop();
        }
      }
    }
  }
}

void ReadEEPData(void)
{
  //Reads EEPData from EEP
  int i;
  unsigned char chk;
 
  //Read data stream
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(F("Reading settings from EEP"));
  #endif
  for (i = 0; i < EEPSize; i++) {
    SettingsEEP.SettingStream[i] = EEPROM.read(i);
  }

  // Check plausibility
  chk = checksum(&SettingsEEP.SettingStream[1], EEPSize - 1);
  if (SettingsEEP.settings.stat == chk) {
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(F("EEP checksum ok "));
      WriteSystemLog("EEPSize " + String(EEPSize) + " Byte");
    #endif  
  }
  else {
    //Initialise new: Automatically takes the init data    
   SettingsEEP.settings.cycle_time = 60;            //Calculating base cycle time [s]
   SettingsEEP.settings.vSound = 3433;              //Velocity of sound [cm/sec]
   SettingsEEP.settings.hOffset = -25;               //Runtime offset [mm]
   SettingsEEP.settings.aBaseArea = 31416;          //Base area of reservoir [cm*cm]
   SettingsEEP.settings.hSensorPos = 240;           //Height of sensor above base area [cm]
   SettingsEEP.settings.hOverflow = 200;            //Height of overflow device [cm]
   SettingsEEP.settings.hRefill = 20;               //Height for refilling start [cm]
   SettingsEEP.settings.volRefill = 20 ;            //Volume to refill [l]
   SettingsEEP.settings.dvolRefill = 20;            //Mass flow of refiller [l/sec]  
   SettingsEEP.settings.volRefillTot=0;             //Refilling volume total [l]
   SettingsEEP.settings.aRoof=100;                  //Area of roof [m^2]
   SettingsEEP.settings.prcFiltEff=90;              //Filter efficiency [%]
   SettingsEEP.settings.prcVolDvtThres=20;          //Threshold for diagnosis
   SettingsEEP.settings.volRainMin=2;               //Minimum 2Liter/24h to activate filter diagnosis
   SettingsEEP.settings.tiRefillReset=timeClient.getEpochTime();

    WriteEEPData();
  }
}

void WriteEEPData(void)
{
  //Reads EEPData from EEP
  int i;
  unsigned char chk;

  chk = checksum(&SettingsEEP.SettingStream[1], EEPSize - 1);
  SettingsEEP.settings.stat = chk;
  for (i = 0; i < EEPSize; i++) {
    EEPROM.put(i, SettingsEEP.SettingStream[i]);
  }
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(F("Writing settings to EEP"));
  #endif
}

unsigned char Refill(int volRefillDes)
//Refill desired quantity, returns:
//0: Refilling off
//1: Refilling in progress
{  
  unsigned long _tiRefillDes;
  unsigned long _tiRefill;
  unsigned long _volRefill;
  unsigned long _tiNow;
  unsigned char _res;
  byte _stRefiller;    // Switch is active high

  if (volRefillDes>0) {   
    // Desired quantity > 0    
    //Remember start of refilling
    _tiNow = timeClient.getEpochTime();
    
    //Get pin state of refiller
    _stRefiller = digitalRead(SOLONOID_PIN);
    
    if (_stRefiller==0) {
      //Refilling not in progress: Start it
      digitalWrite(SOLONOID_PIN,HIGH);         
      digitalWrite(REFLED_PIN,HIGH);
      //Calculate required duration
      _tiRefillDes = (volRefillDes*60)/(long (SettingsEEP.settings.dvolRefill));
      tiRefillerOn = _tiNow;
      tiRefillerOff = _tiNow + _tiRefillDes; 
      #if LOGLEVEL & LOGLVL_NORMAL
        WriteSystemLog("Refilling on. Duration "+String(_tiRefillDes) + " s");
        WriteSystemLog("Off-Time: " + String(time2DateTimeStr(tiRefillerOff)));
      #endif 
      _res=1;  
    }
 
    if (_tiNow>tiRefillerOff) {      
        //Refilling off
        digitalWrite(SOLONOID_PIN, LOW);        
        digitalWrite(REFLED_PIN,LOW);
        //Remember actually refilled volume
        _tiRefill = _tiNow - tiRefillerOn;
        _volRefill = (unsigned long) (_tiRefill*(long (SettingsEEP.settings.dvolRefill))/60);
        SettingsEEP.settings.volRefillTot = SettingsEEP.settings.volRefillTot + _volRefill;
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog("Refilling off, Refilling volume: " + String(_volRefill) + " liter");
        #endif  

        // Start measurement to determine new level
        stMeasAct=1;
        
        //Write to EEPROM        
        WriteEEPData();
        _res=0;  
      }    
  }
  else {
    _res=0;
  }
  return _res;
}

void WriteSystemLog(String LogText)
// Write System log entry
{  
  File logFile;
  String date_time;
  String LogStr;

  //Get Time
  date_time = getDateTimeStr();

  //Log data to SD Card
  LogStr = "[";
  LogStr.concat(date_time);
  LogStr.concat("] ");
  LogStr.concat(LogText);

  if (SD_State == SD_OK) {    
    Workaround_CS_ETH2SD();
    
    logFile = SD.open(LOGFILE, FILE_WRITE);
    logFile.println(LogStr);
    logFile.close();    
    //Test code to catch web-access stalling issue: 
    if (digitalRead(SD_CARD_PIN)==LOW) {
      logFile = SD.open(LOGFILE, FILE_WRITE);
      logFile.println("SD_CARD_PIN==LOW");
      logFile.close();          
    }
    Workaround_CS_SD2ETH();    
  }

  //Write to serial
  Serial.println(LogStr);
}

void EthConnect(void)
{
  #if DHCP_USAGE==1
  // Try to start the Ethernet connection with DHCP
  if (Ethernet.begin(mac) == 0) {
    // If DHCP is not available try backup IP
    Ethernet.begin(mac, ip);
    ETH_StateConnectionType = ETH_BACKUPIP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(F("Failed to configure Ethernet using DHCP, using backup IP"));
    #endif
  }
  else {
    ETH_StateConnectionType = ETH_DHCP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(F("DHCP Address received"));
    #endif  
  }

  #else 
  //Use fixed IP Adress
    Ethernet.begin(mac, ip);
    ETH_StateConnectionType = ETH_FIXEDIP;
    #if LOGLEVEL & LOGLVL_SYSTEM
      WriteSystemLog(F("Using Fixed IP"));
    #endif  
  #endif

  // print your local IP address:
  IPString = "IP-Address: " + IPAddressStr();
  #if LOGLEVEL & LOGLVL_SYSTEM
    WriteSystemLog(IPString);
  #endif  
  delay(1000);
}

unsigned char checksum (unsigned char *ptr, size_t sz) 
//Calculate Checksum
{
  unsigned char chk = 0;
  while (sz-- != 0)
    chk -= *ptr++;
  return chk;
}

void resetFunc (void) 
//Reset function
{
  asm volatile ("  jmp 0");  
}

void TriggerWDT(void)
// Trigger watchdog timer
{
  lastUpdate = millis();
}

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

float Height2Volume(float hIn)
//Converts height in mm to Volume in liter
{
 return hIn*float(SettingsEEP.settings.aBaseArea)/10000;      
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

void SetSystemLEDColor(int color)
// Sets color of system LED
{
  if (color==GREEN) {
    digitalWrite(MEASLEDCOL_PIN, LOW);
  }
  else {
    digitalWrite(MEASLEDCOL_PIN, HIGH);
  } 
}

void SetSystemLED(bool state)
//Switch system LED on or off
{
  bool _colorPinState;
  
  //Get color pin state
  _colorPinState = digitalRead(MEASLEDCOL_PIN);

  if (state)
  {
    //Switch on
    digitalWrite(MEASLED_PIN,!_colorPinState);
  }
  else {
    //Switch off
    digitalWrite(MEASLED_PIN,_colorPinState);
  }
}

void Workaround_CS_ETH2SD()
//Workaround when ETH CS is not reset before SD CS is activated
{ 
  File logFile; 
  String date_time;
  String LogStr;
  
  //Test code to catch web-access stalling issue: 
  if (digitalRead(ETH__SS_PIN)==LOW) {    
     //Get Time
     date_time = getDateTimeStr();

     //Log data to SD Card
     LogStr = "[";
     LogStr.concat(date_time);
     LogStr.concat("] ");
     LogStr.concat("Trap: ETH__SS_PIN==LOW (ACTIVE) detected before SD-Card access");
     
     //Workaround: Switch off CS from Eth, and switch on CS from SD
     digitalWrite(ETH__SS_PIN,HIGH);
     digitalWrite(SD_CARD_PIN,LOW);
     
     //Write to systemlog
     logFile = SD.open(LOGFILE, FILE_WRITE);
     logFile.println(LogStr);
     logFile.close();    
  }  
  else {
     digitalWrite(SD_CARD_PIN,LOW);
  }
}

void Workaround_CS_SD2ETH()
//Workaround when SD CS is not reset before ETH CS is activated
{
  File logFile; 
  String date_time;
  String LogStr;
  
  //Test code to catch web-access stalling issue: 
  if (digitalRead(SD_CARD_PIN)==LOW) {    
     //Get Time
     date_time = getDateTimeStr();

     //Log data to SD Card
     LogStr = "[";
     LogStr.concat(date_time);
     LogStr.concat("] ");
     LogStr.concat("Trap: SD_CARD_PIN==LOW (ACTIVE) after SD-Card access is finished");
     
     //Workaround: Switch off CS from Eth, and switch on CS from SD
     digitalWrite(ETH__SS_PIN,LOW);
     digitalWrite(SD_CARD_PIN,HIGH);
     
     //Write to systemlog
     logFile = SD.open(LOGFILE, FILE_WRITE);
     logFile.println(LogStr);
     logFile.close();    
  }  
  else {
     digitalWrite(ETH__SS_PIN,LOW);
  }    
}    
