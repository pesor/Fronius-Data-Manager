#include "arduino.h"
// *******************************************************************************************************************************
// START userdefined data
// *******************************************************************************************************************************

// Initiate FLAG, Set to true, when you want to initiate all the history variables, See Readme.md
bool initiate = false;

// Network constants
// define your SSID's, and remember to fill out variable ssidArrNo with the number of your SSID's
String ssidArr[] = {"Enterprise-pro", "Enterprise_EXT", "Enterprise_EXTN", "Enterprise" };
int ssidArrNo = 4;
const char* ssid = "Enterprise_EXT";
const char* password = "password";


// Date and time syncronize
const char* ntpServer = "pool.ntp.org";

// Off-sets for time, each hour is 3.600 seconds.
const long  gmtOffset_sec = 3600;

// FTP constants   
char ftp_server[] = "192.168.1.64";
char ftp_user[]   = "user";
char ftp_pass[]   = "password";


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

// *******************************************************************************************************************************
// END userdefined data
// *******************************************************************************************************************************
