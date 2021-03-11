// Konfiguration Andreas Testsystem
// Nutzer:  Andreas 
// Eigenschaften: Testsystem mit Display, mit Homematic, 192.168.178.4, max. Logging
//
//--- Configuration switches ---------------------------------------------------------------------------------
// --- Test system ------------------------------------------------------------------------
// Test-System if USE_TEST_SYSTEM is defined (0 = No test system used, productive system)
// Bit  
//   0: IP Address is fixed, diferent MAC address
//   1: 60min Task reduced to 2min
//   2: Simulated measurement results are used
#define USE_TEST_SYSTEM 7

// --- Network settings -------------------------------------------------------------------
//DHCP cient active
//  0: Fixed IPV4 Adress
//  1: DHCP
#define DHCP_USAGE   0

//IP Adress for normal system if no DHCP is used or DHCP fails
#define IP_ADR_FIXED 192, 168, 178, 5
#define IP_DNS_FIXED 192, 168, 178, 1
#define IP_GWY_FIXED 192, 168, 178, 1
#define IP_SUB_FIXED 255, 255, 255, 0

//IP Adress for test system if no DHCP is used or DHCP fails
#define IP_ADR_TEST 192, 168, 178, 4
#define IP_DNS_TEST 192, 168, 178, 1
#define IP_GWY_TEST 192, 168, 178, 1
#define IP_SUB_TEST 255, 255, 255, 0

// --- Homematic CCU Settings --------------------------------------------------------------
//Switch on coupling to Homematic
//  0: inactive
//  1: active
#define HM_ACCESS_ACTIVE    1 

//IP Adress of Homematic 
#define IP_ADR_HM 192, 168, 178, 60  

// Datapoint numbers for HM
#define HM_DATAPOINT_RAIN24       14245  
#define HM_ZISTERNE_volActual     13596
#define HM_ZISTERNE_rSignalHealth 13598
#define HM_ZISTERNE_prcActual     13599
#define HM_ZISTERNE_stError       13601
#define HM_ZISTERNE_volUsage      13712
#define HM_ZISTERNE_volUsage10d   13713
#define HM_ZISTERNE_stFiltChkErr  13602

// --- Display settings ---------------------------------------------------------------------
//   0: No display
//   1: ILI9341 with SPI touchscreen
#define DISP_ACTIVE  1

// --- SQL Server settings ------------------------------------------------------------------
//SQL Client active
#define SQL_CLIENT 0

//IP-Adress of Server and Name of Folder with Web-Form to access SQL Database
#define SQL_SERVER 192,168,178,6

//Page for operational DB
#define SQL_DATABASE  "HomeData"

// Table name 
#define SQL_TABLE "ZISTERNE"

//SQL Port
#define SQL_PORT 3307

//SQL User
#define SQL_USR "zisterne"

//SQL Passwort
#define SQL_PWD "A1b2c3d4e5f6_"

// --- MQTT Server settings -----------------------------------------------------------------
// MQTT Client active
#define MQTT_CLIENT 1

// MQTT Server
#define MQTT_SERVER "192.168.178.6"

// --- Logging level ------------------------------------------------------------------------
// Combine Bits for required logging level
// LOGLVL_NORMAL   Bit 0: Normal logging
// LOGLVL_WEB      Bit 1: Web server access
// LOGLVL_ALL      Bit 2: Single measurement values
// LOGLVL_CCU      Bit 3: CCU access
// LOGLVL_SYSTEM   Bit 4: System logging (NTP etc)
// LOGLVL_TRAP     Bit 5: Trap for ETH+SD CS signal setting
// LOGLVL_EEP      Bit 6: EEPROM Dump
// LOGLVL_SQL      Bit 7: SQL Server details
#define LOGLEVEL   LOGLVL_NORMAL + LOGLVL_SYSTEM + LOGLVL_EEP

//------------------------------------------------------------------------------------------------------------
