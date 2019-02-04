// Konfiguration Andreas ohne Display mit Homematic, 192.168.178.5
//--- Configuration switches ---------------------------------------------------------------------------------
// --- Test system ------------------------------------------------------------------------
// Test-System if USE_TEST_SYSTEM is defined
// In Test system: IP Address is fixed, diferent MAC address, 60min Task reduced to 2min
//#define USE_TEST_SYSTEM

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
#define IP_ADR_HM 192, 168, 178, 11  

// Datapoint number for 24h rain
#define HM_DATAPOINT_RAIN24 6614  

// --- Display settings ---------------------------------------------------------------------
//   0: No display
//   1: ILI9341 with SPI touchscreen
#define DISP_ACTIVE  0

//------------------------------------------------------------------------------------------------------------
