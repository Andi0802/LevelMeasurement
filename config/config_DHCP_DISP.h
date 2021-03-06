// Konfiguration Ralf
// Nutzer: Ralf 
// Eigenschaften: mit Display, ohne Homematic, DHCP, minimales Logging
//
//--- Configuration switches ---------------------------------------------------------------------------------
// --- Test system ------------------------------------------------------------------------
// Test-System if USE_TEST_SYSTEM is defined
// In Test system: IP Address is fixed, diferent MAC address, 60min Task reduced to 2min
//#define USE_TEST_SYSTEM

// --- Network settings -------------------------------------------------------------------
//DHCP cient active
//  0: Fixed IPV4 Adress
//  1: DHCP
#define DHCP_USAGE   1

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
#define HM_ACCESS_ACTIVE    0 

//IP Adress of Homematic 
#define IP_ADR_HM 192, 168, 178, 11  

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
#define SQLSERVER "192.168.178.6"

//Page for operational DB
#define DATABASE  "HomeData"

// --- Logging level ------------------------------------------------------------------------
// Combine Bits for required logging level
#define LOGLEVEL   LOGLVL_NORMAL

//------------------------------------------------------------------------------------------------------------
