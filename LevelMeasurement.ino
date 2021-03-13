/*
  Level sensor for rain water reservoir
  Features:
    - Level sensing by US Sensor
    - Refill based on level
*/

//Software Version Info
#include "version_info.h"
const String prgVers = PRG_VERS;
const String prgChng = PRG_CHANGE_DESC;
const String cfgInfo = PRG_CFG;
//Sample data
#include "sample_data.h"

//Libraries
//AVR System libraries Arduino 1.6.7
#include <SPI.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <limits.h>

//Arduino Libraries Arduino 1.6.7
#include <Ethernet.h>       //Update auf Version 2.0.0
#include <Dns.h>
#include <SD.h>

//Local libraries
#include <NTPClient.h>           // https://github.com/arduino-libraries/NTPClient.git Commit 020aaf8
#include <TimeLib.h>             // https://github.com/PaulStoffregen/Time.git Commit 6d0Fc5E
#include <Timezone.h>            // https://github.com/JChristensen/Timezone
#include <NewEEPROM.h>           // Ariadne Bootloader https://github.com/codebndr/Ariadne-Bootloader commit 19388fa
#include <NetEEPROM.h>           // Ariadne Bootloader https://github.com/codebndr/Ariadne-Bootloader commit 19388fa
#include <NetEEPROM_defs.h>      // EEPROM Layout
#include <Ucglib.h>              // UCG Lib https://github.com/olikraus/ucglib V1.5.2 
#include <XPT2046_Touchscreen.h> // https://github.com/PaulStoffregen/XPT2046_Touchscreen Commit 1a27318
#include <MySQL_Connection.h>    // https://github.com/ChuckBell/MySQL_Connector_Arduino
#include <MySQL_Cursor.h>
#include <MQTT.h>                // https://github.com/256dpi/arduino-mqtt

//Logging level Bitwise
#define LOGLVL_NORMAL  1  //Bit 0: Normal logging
#define LOGLVL_WEB     2  //Bit 1: Web server access
#define LOGLVL_ALL     4  //Bit 2: Single measurement values
#define LOGLVL_CCU     8  //Bit 3: CCU access
#define LOGLVL_SYSTEM 16  //Bit 4: System logging (NTP etc)
#define LOGLVL_TRAP   32  //Bit 5: Trap for ETH+SD CS signal setting
#define LOGLVL_EEP    64  //Bit 6: EEPROM Dump
#define LOGLVL_SQL   128  //Bit 7: SQL Server details

//Test levels
#define TEST_MACIP     1  //Bit 0: Use different MAC and IP address
#define TEST_TASKTIME  2  //Bit 1: Reduce Task time to 2min
#define TEST_USDATA    4  //Bit 2: Use simulated esponse time

//Configuration
#include "config.h"

//Message types
#define MSG_SEV_ERROR 31
#define MSG_MED_ERROR 30
#define MSG_WARNING   20
#define MSG_INFO      10
#define MSG_DEBUG      0

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
#define ETH_SCK_PIN   13     // Pin D13 SCK
#define ETH_MISO_PIN  12     // Pin D12 MISO
#define ETH__MOSI_PIN 11     // Pin D11 /MOSI
#define ETH__SS_PIN   10     // Pin D10 /SS

//Display
#define DISP_LED_PIN      37   // Pin LED
#define DISP_SCK_PIN      52   // Pin D52 SCK bei HW SPI
#define DISP_CD_PIN       40   // Pin CD Command/Data
#define DISP_MOSI_PIN     51   // Pin D51 MOSI bei HW SPI
#define DISP_MISO_PIN     50   // Pin D50 bei HW SPI
#define DISP__SS_PIN      39   // Pin /SS        
#define DISP__RESET_PIN   36   // Pin /RESET
#define DISP_TIRQ_PIN     20   // Touchscreen IRQ
#define DISP_T_CS_PIN     34   // Touchscreen /CS

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
#define LOGFILE   F("Res_Ctl.log")     // Name of system logfile
#define DATAFILE  F("Res_Ctl.csv")     // Name of data logfile

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
#define PLS_TI_TIMEOUT 80000

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

//Diagnosis: Usage in 1h (20 Liter)
#define VOL_USAGE_MAX_1H 20

//Average in usage info: Number of values
#define NUM_USAGE_AVRG  10

//History values in EEP
#define EEP_NUM_HIST 240
const PROGMEM unsigned char SAMPLES_PRCVOL[EEP_NUM_HIST]   = SAMPLES_PRCVOL_DATA;
const PROGMEM unsigned char SAMPLES_VOLRAIN[EEP_NUM_HIST]  = SAMPLES_VOLRAIN_DATA;
const PROGMEM unsigned char SAMPLES_STSIGNAL[EEP_NUM_HIST] = SAMPLES_STSIGNAL_DATA;
#define CONV_RAIN   0.3  //Conversion factor for rain
#define CONV_REFILL  20  //Conversion factor for refill quantity in 1h

//LED colors
#define GREEN 0
#define RED   1

//Device name of HM Client
//IP of CCU
#if HM_ACCESS_ACTIVE==1
  byte hm_ccu[] = { IP_ADR_HM };
  EthernetClient hm_client;
#endif

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
#if (USE_TEST_SYSTEM & 1)>0
  //MAC for testing system
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA };
  
  // Set the static IP address if DHCP fails or fixed address is used
  IPAddress ip(IP_ADR_TEST);
  IPAddress dns(IP_DNS_TEST);
  IPAddress gateway(IP_GWY_TEST);
  IPAddress subnet(IP_SUB_TEST);   
#else
  //MAC for operational system
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEB };
  
  // Set the static IP address if DHCP fails or fixed address is used
  IPAddress ip(IP_ADR_FIXED);
  IPAddress dns(IP_DNS_FIXED);
  IPAddress gateway(IP_GWY_FIXED);
  IPAddress subnet(IP_SUB_FIXED);  
#endif

#if (USE_TEST_SYSTEM & 2)>0
  // Reduce Main Task to 2min
  #define T_MAINTASK 2
#else   
  // Set Main Task in min
  #define T_MAINTASK 15  
#endif 

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
//EthernetClient client;

//UDP Client for NTP
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

//Web Server
EthernetServer server(80);

//Display ILI9341 on HWSPI
#if DISP_ACTIVE==1
  Ucglib_ILI9341_18x240x320_HWSPI ucg(/*cd=*/ DISP_CD_PIN, /*cs=*/ DISP__SS_PIN, /*reset=*/ DISP__RESET_PIN);
  XPT2046_Touchscreen ts(DISP_T_CS_PIN, DISP_TIRQ_PIN);
#endif

// Definitions for SQL server
#if SQL_CLIENT>0
  EthernetClient EthClientSQL;
  IPAddress sql_addr(SQL_SERVER);
  
  // MySQL Connector constructor
  MySQL_Connection MySQLCon((Client *)&EthClientSQL);      
#endif

// Definitions for MQTT client
#if MQTT_CLIENT>0
  EthernetClient EthClientMQTT;
  MQTTClient MQTT;
#endif

// --- Globals ---------------------------------------------------------------------------------------------------

//Time
unsigned int Time_100ms = 0;
unsigned int cnt60 = 0;
unsigned int Time_60s = 0;
unsigned int cnt60min = 0;
unsigned int cntPulse = 0;
#if DISP_ACTIVE==1
  unsigned int tiDispLED = 2;
#endif

//US Sensor measurement
unsigned char stPulse = PLS_IDLE;  //state of measurement pulse
unsigned long tiSend;              //Timestamp pulse sent
unsigned long tiReceivedPos;       //Timestamp pulse received rising edge
unsigned long tiReceived;          //Timestamp pulse received falling edge
unsigned int dauer=0;              //Duration of pulse single measurement
unsigned int entfernung=0;         //Distance measured single measurement
float hMean=0;                     //Mean value of level
unsigned int volActual=0;          //Actual volume
unsigned int volMax=0;             //Maximum volume
unsigned char  prcActual=0;        //Percentage of maximum volume
unsigned int hWaterActual=0;       //Actual level (average after diagnosis)
time_t MeasTime;                   //Measurement time 
time_t MeasTimeOld;                //Measurement time of last measurement
String MeasTimeStr ;               //Measurement time as string
unsigned long tiDiffMeasTime;      //Difference between last two measurements
unsigned long tiRefillerOff;       //Timestamp refilling off
unsigned long tiRefillerOn;        //Timestamp refilling on
unsigned char stRefill=0;          //Status Refilling in progress
unsigned char stRefillReq=0;       //Request for refilling step based on measurement
unsigned char stVentEx=0;          //Set to 1 if vent-excersice has been started, to provide multiple refillers in 1 hour
double volStdDev;                  //Standard deviation of measurements in volume
unsigned char rSignalHealth=0;     //Signal health (0 worst - 100 best)
int cntValidValues;                //Number of valid values
int cntDiagDeb;                    //Debounce counter diagnosis
unsigned char stError;             //Error status
unsigned char pos = 0;             //Array position to write measurement
int arr[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned char stButton=0;          //Refill button pressed
unsigned char stMeasAct=1;         //Request for measurement, start measurement immediately
unsigned char stFilterCheck=FILT_OFF; //Filter diagnosis
unsigned char stFiltChkErr;        //Result filter check
float volRainDiag1h;               //Rain in 1h for Diagnosis
float volRain24h_old=-1;           //Total rain, 1 period ago (-1: invalid)
float volRain1Per;                   //Rain in last 1h for Diagnosis
bool stRain1h;                     //Status of 1h rain measurements
unsigned long volRefillFilt24h;    //Stored value of Refilling volume 24h past
unsigned long volRefillFilt1h;     //Stored value of Refilling volume 1h past
unsigned int  volActualFilt24h=0;  //Stored actual reservoir volume 24h past
unsigned int  volActualFilt1h=0;   //Stored actual reservoir volume 1h past
unsigned int  volUsageDiag24h;     //Water usage in 24h for Diagnosis
int  volRefill24h;                 //Refilled volume in 24h
unsigned int  volRefill1h;         //Refilled volume in 1h
unsigned int  volRefillDiag1h;     //Refilled volume in 1h for Diagnosis
unsigned int  volOld;              //Old volume for tendency 
int volDiff1h;                     //Volume change in 1h (tendency)
float volRainTot=-1;               //Total rain (-1: invalid)
int volDiff24h;                    //Difference volume measurement in 24h calculated each hour
int volDiff24h1d;                  //Difference volume measurement in 24h calculated once a day
int volDiffDiag1h;                 //Difference volume measurement in 1h for Diagnosis
int volUsageAvrg10d;               //10 days average of daily water usage
int volUsageMax10d=-32768;         //Maximum out of 10days
int volUsageMin10d=32767;          //Minimum out of 10 days
String UsageTimeStr;               //String of last calculation water usage
String DiagTimeStr;                //String of last calculation filter diagnosis
int prcVolDvt1h;                   //Deviation between theoretical and real value in %
int volDiffCalc1h;                 //Theoretical change of volume
unsigned char cntLED=0;            //Counter for LED 
String inString;                   //Webserver Receive string, IP-Adress string
unsigned char cntTestFilt=0;       //Testing for filter diagnosis
byte PulseCntr;                    //Counter for US pulses
int idxCur;                        //Index for Curve

#if USE_TEST_SYSTEM & TEST_USDATA
  int test_time=3000;              //signal runtime for test 
  int test_noise=300;              //noise of signal runtime
#endif

//Typedefs
// Settings in EEPROM
struct settings_t {
  unsigned char stat;         //Status of EEPROM
  int cycle_time;             //Calculating base cycle time [s]
  int vSound;                 //Velocity of sound [cm/sec]
  int hOffset;                //Runtime offset [mm]
  int aBaseArea;              //Base area of reservoir [cm*cm]
  int hSensorPos;             //Height of sensor above base area [cm]
  int hOverflow;              //Height of overflow device [cm]
  int hRefill;                //Height for refilling start [cm]
  int volRefillMax;           //Maximum volume to refill [l]
  int dvolRefill;             //Mass flow of refiller [l/sec]  
  unsigned long volRefillTot;  //Refilling volume total [l]
  unsigned long tiRefillReset; //Timestamp of last reset volRefillTot
  int aRoof;                   //Surface of roof
  int prcFiltEff_x[5];         //filter efficiency in x-values: rain in 1h [mm]
  int prcFiltEff_y[5];         //filter efficiency in y-values: efficiency=(1-overflow) [%]
  int prcVolDvtThres;          //Threshold for filter clogging error
  int volRainMin;              //Minimum Rain Volume in 24h to activate filter diagnosis
  int volUsage24h[NUM_USAGE_AVRG]; //Last values of usage calculation
  unsigned char iDay;          //Last index of entry in volUsage24h
  unsigned char volRain1Per[EEP_NUM_HIST]; //1h Rain quantity history points 1 = 0.3 Liter > 255 = 85l
  byte prcActual[EEP_NUM_HIST]; //Volume history points
  unsigned char stSignal[EEP_NUM_HIST];  //Status of signal
  unsigned char iWrPtrHist;              //History write pointer
};

//EEPROM Settings as union
union setting_stream_t {
  settings_t settings;
  unsigned char SettingStream[sizeof(settings_t)];
};

//EEP Data
setting_stream_t SettingsEEP;      //EEPData
//Size of EEP Data incl. checksum in byte
int EEPSize=sizeof(SettingsEEP.settings);

//State of components
int SD_State  = SD_UNDEFINED;
int ETH_State = ETH_UNDEFINED;
int ETH_StateConnectionType = ETH_UNDEFINED;
int NTP_State = NTP_UNDEFINED;

// --- Setup -----------------------------------------------------------------------------------------------------
void setup() {
  // Initialize all CS signals of SPI Bus
  pinMode(ETH__SS_PIN, OUTPUT);    digitalWrite(ETH__SS_PIN,1);
  pinMode(SD_CARD_PIN, OUTPUT);    digitalWrite(SD_CARD_PIN,1);
  pinMode(DISP_T_CS_PIN, OUTPUT);  digitalWrite(DISP_T_CS_PIN,1);
  pinMode(DISP__SS_PIN, OUTPUT);   digitalWrite(DISP__SS_PIN,1);
  
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

 //Initialize Display
  #if DISP_ACTIVE==1
    DispInit();
  #endif
  
  //SD Card initialize  
  Workaround_CS_ETH2SD(1);
  
  if (!SD.begin(SD_CARD_PIN)) {
    SD_State = SD_FAIL;
    WriteSystemLog(MSG_MED_ERROR,F("Initializing SD Card failed"));
  }
  else {
    SD_State = SD_OK;
    WriteSystemLog(MSG_INFO,F("Initializing SD Card ok"));
  }
 
  // Try Ethernet connection
  EthConnect();

  //Create ethernet server
  server.begin();

  //Set NTP as sync provider
  setSyncProvider(getNtpTime); //Sync provider
  setSyncInterval(3600);       //1 hour updates
  now();                       //Synchronizes clock

  // Connect to SQL server
  #if SQL_CLIENT>0
    ConnectSQL();
  #endif

  // Activate MQTT Client
  #if MQTT_CLIENT>0
    MQTT_Init();
    MQTT_Connect();
  #endif
  
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
  WriteSystemLog(MSG_INFO,F("System startup"));
  
  //Enable watchdog
  wdt_enable(WDTO_8S);

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

void ReceivePulse(void)
//Receives a pulse on Echo Pin
{
  if (stPulse==PLS_SEND) {
    //Wait for pulse only if a pulse has been send previously
    if (digitalRead(US_ECHO_PIN)) {
      //Send time
      tiReceivedPos=micros();
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
  double _hStdDev;   
  int idx24h;
  int volRefillReq;

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
    if (cnt60min < T_MAINTASK*600) {
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
          WriteSystemLog(MSG_INFO,F("Refilling requested by manual button"));
        #endif  
      } 
    }
      
    //Check if Refilling is needed
    if ((stRefillReq>0) || (stRefill>0) || (stButton>0)) {          
      // Calculate required refill quantity and limit to minimum of 20l
      volRefillReq = int(max(20,long(SettingsEEP.settings.hRefill)*long(SettingsEEP.settings.aBaseArea)/1000-volActual));

      // Limit to maximum of volRefill
      volRefillReq = min(int(SettingsEEP.settings.volRefillMax), volRefillReq);      

      // Refill function
      stRefill = Refill(volRefillReq);
  
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
  	  // Simulation without US sensor attached
  	  #if (USE_TEST_SYSTEM & TEST_USDATA)>0
    		if (stPulse==PLS_SEND) {
    		  //Wait for pulse only if a pulse has been send previously
    		  //Send time
    		  tiReceivedPos=micros();
    		  stPulse = PLS_RECEIVED_POS;		
    		}
    		else if (stPulse==PLS_RECEIVED_POS) {		
    		  //Receive time: 10ms + 0,3ms*random
    		  tiReceived=tiReceivedPos+test_time-int(test_noise>>1)+random(test_noise);
    		  stPulse = PLS_RECEIVED_NEG;		
    	    }
  	  #endif
	  
      //State maschine for return pulse handling 
      switch (stPulse) {
        case PLS_IDLE: 
          #if LOGLEVEL & LOGLVL_NORMAL
          if (pos==0) {
            WriteSystemLog(MSG_DEBUG,"Sending first pulse. Waiting for reponse");        
          }
          #endif
          //Send a pulse only if previous pulse has beed received                
          digitalWrite(US_TRIGGER_PIN, HIGH);        
          delay(2);
          digitalWrite(US_TRIGGER_PIN, LOW);                  
          stPulse = PLS_SEND;
        break;
        
        case PLS_RECEIVED_NEG:
          //Calculated distance only if pulse has been received
          dauer = PositiveDistanceUL(tiReceived,tiReceivedPos);        
    
          //Calculate distance in mm
          entfernung = ((unsigned long)(dauer/2) * SettingsEEP.settings.vSound)/10000 - SettingsEEP.settings.hOffset;
  
          #if LOGLEVEL & LOGLVL_ALL
            WriteSystemLog(MSG_DEBUG,"Duration measured: " + String(dauer) + "us Send: " + String(tiReceivedPos) + " Received: " + String(tiReceived)+ " Entfernung: " + String(entfernung) + " mm");
          #endif
          
          //Plausibility check: Signal cannot be higher than sensor position and not lower than distance btw sensor and overflow  
          //With 100mm safety distance
          if (entfernung >= (SettingsEEP.settings.hSensorPos*10+100) || entfernung <= ((SettingsEEP.settings.hSensorPos-SettingsEEP.settings.hOverflow)*10)-100)
          {
            //Measurement not plausible, set dummy value 0
            arr[pos] = 0;
            #if LOGLEVEL & LOGLVL_NORMAL
              WriteSystemLog(MSG_DEBUG,"Duration measured: " + String(dauer) + "us Send: " + String(tiReceivedPos) + " Received: " + String(tiReceived)+ " Entfernung: " + String(entfernung) + " mm");
            #endif  
          }
          else
          {
            //Measurement plausible, collect measurement
            arr[pos] = entfernung;
          }
          pos++;
          stPulse = PLS_IDLE;
          break;
      
        case PLS_RECEIVED_POS:
          //Check for timeout if pulse is send and rising edge of answer has been detected, but falling edge is missing
          if (PositiveDistanceUL(micros(),tiReceivedPos)>PLS_TI_TIMEOUT) {
            //Timeout detected
            #if LOGLEVEL & LOGLVL_NORMAL
              WriteSystemLog(MSG_SEV_ERROR,F("Timeout US measurement detected: Falling edge of return missing"));
            #endif  
            dauer = PLS_TI_TIMEOUT;
            entfernung = 0;
            arr[pos]=0;
            pos++;
            stPulse = PLS_IDLE;
          }  
          break;
        case PLS_SEND:
          //Check for timeout if pulse is send but rising edge of answer is missing
          if (PositiveDistanceUL(micros(),tiSend)>PLS_TI_TIMEOUT) {
            //Timeout detected
            #if LOGLEVEL & LOGLVL_NORMAL
              WriteSystemLog(MSG_SEV_ERROR,F("Timeout US measurement detected: No return pulse received"));
            #endif  
            dauer = PLS_TI_TIMEOUT;
            entfernung = 0;
            arr[pos]=0;
            pos++;
            stPulse = PLS_IDLE;
          }
          break;  
      }	  	 
	  
      #if DISP_ACTIVE==1
        // Show progress bar
        DispMeasProg(pos);
      #endif
    } //if pos<100      
    else {
      #if DISP_ACTIVE==1
        // Del progress bar
        DelMeasProg();
      #endif
      // Evaluate measured delay times      
      //Capture measurement time and calculate difference time between last two measurements
      MeasTimeOld = MeasTime; 
      MeasTime = getLocTime();
      MeasTimeStr = time2DateTimeStr(MeasTime); 
      if (MeasTimeOld>0) {
        tiDiffMeasTime = PositiveDistanceUL(MeasTime,MeasTimeOld);
      }
      else {
        tiDiffMeasTime = 0;  
      }

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
        rSignalHealth = 100 - min(6*volStdDev/volMax*1000,100);      
        if (rSignalHealth<0) {
          rSignalHealth=0;
        }
      }
      else {
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(MSG_MED_ERROR,"Only " + String(cntValidValues) + " valid measurement values detected (Threshold=40), no new volume calculated");
          
        #endif
        rSignalHealth = 1;
      }
      
      //Convert to volume
      if (rSignalHealth>50) {
        hWaterActual = max(SettingsEEP.settings.hSensorPos*10-hMean,0);      
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
            WriteSystemLog(MSG_MED_ERROR,"Error detected, debouncing "+String(cntDiagDeb));
          #endif 
          
          //Trigger next measurement immediately
          stMeasAct = 1;
        }      
        else {
          //Error detected
          SetError(ERRNUM_SIGNAL_HEALTH,true);
          
          //Stop measurement
          stMeasAct=0;
        }
      }        
      else {
        if (cntDiagDeb>0) {      
          cntDiagDeb--;
          
          #if LOGLEVEL & LOGLVL_NORMAL
            WriteSystemLog(MSG_INFO,"Good signal detected, debouncing "+String(cntDiagDeb));
          #endif 

          //Trigger next measurement immediately
          stMeasAct = 1;
        }
        else {
          //Error healed
          SetError(ERRNUM_SIGNAL_HEALTH,false);

          //Stop measuerement
          stMeasAct =0;
        }
      }

      //Retrigger Watchdog
      TriggerWDT();  
      
      //Write Log File
      #if LOGLEVEL & LOGLVL_NORMAL
        WriteSystemLog(MSG_INFO,"Measured distance " + String(hMean) +" mm");
        WriteSystemLog(MSG_INFO,"Actual Level " + String(hWaterActual) + " mm");
        WriteSystemLog(MSG_INFO,"Actual volume " + String(volActual) + " Liter, StdDev: " +String(volStdDev) + " Liter");
        WriteSystemLog(MSG_INFO,"Error status " + String(stError));
      #endif          
      
      //Get total rain volume only if difference between last two measurements is more than one meas. period
      if (tiDiffMeasTime>max((T_MAINTASK-2),1)*60) {
        volRain24h_old = volRainTot;
        #if HM_ACCESS_ACTIVE==1
          volRainTot = hm_get_datapoint(HM_DATAPOINT_RAIN24);
          //Offen: Updatezeit lesen, Datenpunktnummer ermitteln
          //falls Wert nicht aktuell: Fehler: keine Aktuelle Regenmenge     
        #else
          //No Homematic access: deactivate
          volRainTot = -1; 
        #endif
  
        //Calculate rain within last 1h
        if (volRain24h_old>-1) {
          volRain1Per = max(volRainTot-volRain24h_old,0);
          stRain1h=true;
        }
        else {
          //after new start: Initialize with 0 until old value is available
          volRain1Per = 0;
          stRain1h=false;
        }
  
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(MSG_INFO,"Total rain from weather station [mm]: "+String(volRainTot));
          WriteSystemLog(MSG_INFO,"Rain in last period from weather station [mm]: "+String(volRain1Per));
        #endif

        //Calculate 1h refilling volume
        volRefill1h = SettingsEEP.settings.volRefillTot - volRefillFilt1h;
    
        //Check filter if rain within last hour is above threshold and volume is below 90% of maximum volume
        if ((volRain1Per>SettingsEEP.settings.volRainMin) && (volActual<0.9*volMax) && (volActualFilt1h>0)) {
          //Filter check
          CheckFilter();
        }
        //Reset Counters and store actual values
        volRefillFilt1h = SettingsEEP.settings.volRefillTot;  
            
        if (rSignalHealth>50) {
          //Take actual volume only if actual measurement is valid
          volActualFilt1h = volActual;
        }
        else {
          //Invalid old value
          volActualFilt1h = 0;
        }

        //Write data to EEP History
        WriteEEPCurrData(prcActual, volRain1Per, (rSignalHealth>SIGNAL_HEALTH_MIN), stRain1h, volRefill1h);              
      }

      //Calcluate usage per day if hour=0 and rain within last 24h is zero
      //Difference Volume in 24h
      idx24h = (SettingsEEP.settings.iWrPtrHist-1-24+EEP_NUM_HIST)%EEP_NUM_HIST;      
      volDiff24h =  volActual - (volMax* (long) SettingsEEP.settings.prcActual[idx24h])/100;      
      if (hour()==0) {
        if ((abs(volRainTot-volRain24h_old)<MAX_VOL_NORAIN) && (volActualFilt24h>0)) {
           //Usage calulation: Take over 24h Difference to store it
           volDiff24h1d = volDiff24h;
           CalculateDailyUsage();
        }
        if (rSignalHealth>50) {
          //Store old value of volume
          volActualFilt24h = volActual;        
        }
        else {
          //Invalid old value
          volActualFilt24h = 0;        
        }        
        //Store refilling quantity
        volRefillFilt24h = SettingsEEP.settings.volRefillTot;      
      }           
      //Calculate statistic of usage
      StatisticDailyUsage();
      
      //Log data 
      LogData();     

      // Send data to SQL Client
      #if SQL_CLIENT>0
        SendDataSQL();
      #endif

      // Publish MQTT messages
      #if MQTT_CLIENT>0
        MQTT_Publish();
      #endif

      //Write to display
      #if DISP_ACTIVE==1
        DispStatus(now(), prcActual, rSignalHealth, volActual, volDiff24h);
      #endif
      
      //Send data to Homematic CCU
      #if HM_ACCESS_ACTIVE==1
        hm_set_state(HM_ZISTERNE_volActual, volActual);
        delay(1); //nicht optimal, aber sonst stürzt HM ab
        hm_set_state(HM_ZISTERNE_rSignalHealth, rSignalHealth);
        delay(1); //nicht optimal, aber sonst stürzt HM ab
        hm_set_state(HM_ZISTERNE_prcActual, prcActual);           
        delay(1); //nicht optimal, aber sonst stürzt HM ab
        hm_set_state(HM_ZISTERNE_stError, stError);          
      #endif
      
      //Reset pointer for next data collection
      pos = 0;

      //Reset last evaluation type Filter and usage
      stFilterCheck = FILT_OFF;

      // Check if refilling is required
      if((hWaterActual<SettingsEEP.settings.hRefill*10) && (rSignalHealth>SIGNAL_HEALTH_MIN)) {
        stRefillReq = 1;
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(MSG_INFO,F("Refilling required due to level threshold"));
        #endif
      }

      // Start refilling every 1th of month at 12am
      if ((day()==1) && (hour()==12) && (stVentEx==0)) {
        stRefillReq = 1;
        stVentEx=1;
        #if LOGLEVEL & LOGLVL_NORMAL
          WriteSystemLog(MSG_INFO,F("Monthly refilling for vent exercise"));
        #endif
      }
      if ((day()==1) && (hour()==13)) {
        //Reset excersice bit for next month
        stVentEx=0;
      }      
    } // if pos==100        
  }    
  
  //--- 60s Task ---------------------------------------------------------------------
  if (Time_60s > 0) {
    Time_60s--;

    //Monitor NTP
    if (NTP_State == NTP_RECEIVED) {
      //Write log entry and set State to WaitNextUpdate
      #if LOGLEVEL & LOGLVL_SYSTEM
        WriteSystemLog(MSG_DEBUG,F("Time updated by NTP"));
      #endif
      NTP_State = NTP_WAITUPD;
    }   

    //Backlight timer
    #if DISP_ACTIVE==1
      //Display time
      DispTime();

      //Check backlight timer
      if (tiDispLED>0) {
        tiDispLED--;        
      }
      else {
        //Display backlight off
        DispLED(false);
      }
    #endif    
  } //60s Task

  //=== Idle Task ===
  //Activate web server for parameter settings and monitoring
  MonitorWebServer(); 

  #if DISP_ACTIVE==1
    if (ts.touched()) {
      // Turn the backlight on
      DispLED(true);      

      // Reset timer in min
      tiDispLED = 2; 
    }
  #endif

  #if MQTT_CLIENT>0
    // Handle incomming MQTT messages
    MQTT.loop();
  #endif

  //Update time
  now();

}


// --- Functions -----------------------------------------------------------------------------------------------------
void CalculateDailyUsage() {
    //Calculates usage per day and stores in buffer       
    
    //Remember type of evaluation
    stFilterCheck = FILT_USAGE;
    
    //Calculate water usage      
    //Take timestamp
    UsageTimeStr = getDateTimeStr(); 

    //Calculate daily refilling volume
    volRefill24h = PositiveDistanceUL(SettingsEEP.settings.volRefillTot,volRefillFilt24h);   
    
    //Calculate daily water usage amd store in array
    SettingsEEP.settings.iDay = (SettingsEEP.settings.iDay+1)%NUM_USAGE_AVRG;
    SettingsEEP.settings.volUsage24h[SettingsEEP.settings.iDay] =  volRefill24h - volDiff24h1d;  

    //Write to EEPROM
    WriteEEPData();
}

void StatisticDailyUsage() {
    //Calculates mean, min and max of daily usage    

    byte _id;  //Day counter for average
    unsigned char _cntAvrg=0;

    //Average over NUM_USAGE_AVRG days
    volUsageAvrg10d=0;
    for (_id=0;_id<NUM_USAGE_AVRG;_id++) {
      //Check if init value 32767 Liter 
      if (SettingsEEP.settings.volUsage24h[_id]<32767) {
        //Average
        volUsageAvrg10d = volUsageAvrg10d + SettingsEEP.settings.volUsage24h[_id];
        _cntAvrg++;

        //Max
        if (SettingsEEP.settings.volUsage24h[_id]>volUsageMax10d) {
          volUsageMax10d = SettingsEEP.settings.volUsage24h[_id];
        }

        //Min
        if (SettingsEEP.settings.volUsage24h[_id]<volUsageMin10d) {
          volUsageMin10d = SettingsEEP.settings.volUsage24h[_id];
        }
      }      
    }
    
    volUsageAvrg10d = volUsageAvrg10d/max(1,_cntAvrg);       

    #if HM_ACCESS_ACTIVE==1
      //Send result to Homematic
      hm_set_state(HM_ZISTERNE_volUsage, SettingsEEP.settings.volUsage24h[SettingsEEP.settings.iDay]);           
      delay(1); //nicht optimal, aber sonst stürzt HM ab
      //Send result to Homematic
      hm_set_state(HM_ZISTERNE_volUsage10d, volUsageAvrg10d);                   
    #endif
}

void CheckFilter() {
    //Filter diagnosis based on 1h-rain and volume change
  
    long _volDvt1h;      //Deviation between theoretical change and real change    
    int _prcFiltEff;     //filter efficency based on 1h rain value
      
    //Remember type of evaluation
    stFilterCheck = FILT_DIAG;
    
    //Calculate filter diagnosis
    //Take timestamp
    DiagTimeStr = getDateTimeStr(); 

    //Store 1h rain value for diagnosis
    volRainDiag1h = volRain1Per;
    
    //Capture 1h refilling volume
    volRefillDiag1h = volRefill1h;
    
    //Difference Volume
    volDiffDiag1h =  volActual - volActualFilt1h;
        
    //Filter efficiency as function of rain volume in last hour
    _prcFiltEff = Curve(SettingsEEP.settings.prcFiltEff_x,SettingsEEP.settings.prcFiltEff_y,5,volRain1Per);
    
    //Theoretical volume change within 1h
    volDiffCalc1h = volRainDiag1h*SettingsEEP.settings.aRoof*_prcFiltEff/100 + volRefillDiag1h - VOL_USAGE_MAX_1H;
    
    //Difference between calculated and real value
    _volDvt1h = volDiffCalc1h - volDiffDiag1h;
    
    //Relative deviation to theoretical quantity, lower limit 0%    
    prcVolDvt1h = max(0,(100*_volDvt1h)/volDiffCalc1h);
    
    //Diagnois result
    if (prcVolDvt1h>SettingsEEP.settings.prcVolDvtThres) {
      stFiltChkErr = FILT_ERR_CLOGGED;
    }
    else {
      stFiltChkErr = FILT_ERR_OK;
    }

    #if HM_ACCESS_ACTIVE==1
      //Send result to Homematic
      hm_set_state(HM_ZISTERNE_stFiltChkErr, stFiltChkErr);           
      //delay(1); //nicht optimal, aber sonst stürzt HM ab                 
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
    _tiNow = getLocTime();
    
    //Get pin state of refiller    
    _stRefiller = digitalRead(SOLONOID_PIN);
    
    
    if (_stRefiller==0) {
      //Refilling not in progress: Start it
      digitalWrite(SOLONOID_PIN,HIGH);         
      digitalWrite(REFLED_PIN,HIGH);
    
      //Calculate required duration
      _tiRefillDes = (long(volRefillDes)*60)/(long (SettingsEEP.settings.dvolRefill));
      tiRefillerOn = _tiNow;
      tiRefillerOff = _tiNow + _tiRefillDes; 
      #if LOGLEVEL & LOGLVL_NORMAL      
        WriteSystemLog(MSG_INFO,"Refilling "+String(volRefillDes));      
        WriteSystemLog(MSG_INFO,"Refilling on. Duration "+String(_tiRefillDes) + " s");
        WriteSystemLog(MSG_INFO,"Off-Time: " + String(time2DateTimeStr(tiRefillerOff)));
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
          WriteSystemLog(MSG_INFO,"Refilling off, Refilling volume: " + String(_volRefill) + " liter");
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

float Height2Volume(float hIn)
//Converts height in mm to Volume in liter
{
 return hIn*float(SettingsEEP.settings.aBaseArea)/10000;      
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
      WriteSystemLog(MSG_MED_ERROR,_txtError);
      WriteSystemLog(MSG_MED_ERROR,_envInfo);
    #endif
        
    //Set global Error information
    stError = stErr;
  }  
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

