#include "arduino.h"
// *******************************************************************************************************************************
// START userdefined data
// *******************************************************************************************************************************

// Turn logging on/off - turn read logfile on/off, turn delete logfile on/off ---> default is false for all 3, otherwise it can cause battery drainage.
const bool  logging =  true;
const bool  readLogfile = false;
const bool  deleteLogfile = true;

// Initiate FLAG, Set to true, when you want to initiate all the history variables, See Readme.md
bool initiate = false;

// Network constants
// define your SSID's, and remember to fill out variable ssidArrNo with the number of your SSID's
String ssidArr[] = {"Enterprise", "Enterprise_EXT", "Enterprise_EXTN", "Enterprise-pro"};
int ssidArrNo = 4;
const char* ssid = "Enterprise_EXT";
const char* password = "Password";


// Date and time syncronize
const char* ntpServer = "pool.ntp.org";

// Off-sets for time, each hour is 3.600 seconds.
const long  gmtOffset_sec = 3600;

// FTP constants   
char ftp_server[] = "192.168.1.64";
char ftp_user[]   = "admin";
char ftp_pass[]   = "Password";


// mqtt constants
const char broker[] = "192.168.1.64";
int        port     = 1883;
const char mqttuser[] = ""; //add eventual mqtt username
const char mqttpass[] = ""; //add eventual mqtt password

const char topic[]  = "Fronius/online";
const char last12[] = "Fronius/last12";
const char allYears[] = "Fronius/allYears";

// http constants
const char kHostname[] = "192.168.1.239";

float correctManualStart = 0.00;  // the current month production, in order to get figures right ok

/* Set your home coordinates and your offset from UTC time */
#define LATITUDE        55.772488
#define LONGITUDE       12.16211
#define DST_OFFSET      1

// *******************************************************************************************************************************
// END userdefined data
// *******************************************************************************************************************************
