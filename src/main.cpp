#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <ESP32_FTPClient.h>
#include "SPIFFS.h"
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "user-defined-variables.h"
// Sunrise/Sunset
#include <sunset.h>
#include <TimeLib.h>

#include <StreamUtils.h>

#define uS_TO_S_FACTOR 1000000ULL //Conversion factor for micro seconds to seconds

//                 "2.0.1"; // Fixed null values when inverter is off line.
//                 "2.0.2"; // Minor error corrections
//                 "2.0.3"; // Major corrections, DST, Initiation, and FTP Backup
const String rel = "3.0.0"; // Change from Arduino EDI, to Visual Code EDI

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

HttpClient http(wifiClient, kHostname);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
ESP32_FTPClient ftp(ftp_server, ftp_user, ftp_pass);

// Retrieval of sunrise and sunset data.
static const char ntpServerName[] = "us.pool.ntp.org";
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
WiFiUDP Udp;
unsigned int localPort = 8888; // local port to listen for UDP packets
SunSet sun;

// Date variables
String formattedDate;
String formattedFroniusDate;
String dayStamp;
int yearStamp;
String timeStamp;
String timeStamp1; // maybe not used
int thisDay;
int sunset_hour;
int sunrise_hour;
int thisDayInMonth;
int thisMonthInMonth;
int thisYearInMonth;
int thisHourInMonth;
int thisMinuteInMonth;
bool lastDayOfMonth = false;
// This variable to true, so that we get the production of the day, when program starts.
bool firstRun = true;

// Various Energy variables
float yearToLastMonthEnergy;
float thisMonthsEnergy;
float twelveLast[14];
float twelveLastTotal;
float yearsLast[14];
float yearEnergy;

//Various variables
String thisMonthInt;
String lastMonthInt;
String lastDayInt;
String readString;
int statusFroniusInt;
String timeStampFronius;
bool PAC_eq_0;
bool UDC_gt_0;
char buffer[1792];
DynamicJsonDocument docSave(1792);

// This variable is used for check of network connection.
long rssi;

// Path to download data from the Fronius Datamanager, (this is the bit after the hostname in the URL)
const char kPath[] = "/solar_api/v1/GetInverterRealtimeData.cgi?Scope=Device&DeviceId=1&DataCollection=CommonInverterData";
// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;
float producedNow = 0.00;

// Start Subroutines
#include <go-to-sleep-5-minutes.h>
#include <file-management.h>
#include <initialize.h>
#include <sunrise-sunset.h>
#include <get-ftp-message.h>
#include <last-year-data.h>
#include <get-last-12-month-data.h>
#include <get-fronius-json-data.h>
#include <connect-to-network.h>

void setup()
{
  // initialize serial communication:
  Serial.begin(115200);
  Serial.println(" ");
  Serial.print("Release version: ");
  Serial.println(rel);

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("Connected to network");
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  rssi = WiFi.RSSI();
  Serial.print("RSSI: ");
  Serial.println(rssi);

  // Mount file system
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    goToDeepSleepFiveMinutes();
  }
  listDir(SPIFFS, "/", 0);

  // Initiate historic variables, at first start of program, or if adjustments are needed.
  if (initiate)
  {
    Initialize();
  }

  // Now we start reading the files..
  readFile(SPIFFS, "/lastDay.txt");
  lastDayInt = readString;
  readString = "";
  readFile(SPIFFS, "/lastMonth.txt");
  lastMonthInt = readString;
  readString = "";
  Serial.println(lastMonthInt);
  readFile(SPIFFS, "/yearToLastMonthEnergy.txt");
  yearToLastMonthEnergy = readString.toFloat();
  Serial.print("Energi til sidste måned: ");
  Serial.println(yearToLastMonthEnergy);

  /* Get our time sync started */
  /* Set our position and a default timezone value */
  sun.setPosition(LATITUDE, LONGITUDE, DST_OFFSET);
  sun.setTZOffset(DST_OFFSET);
  setSyncProvider(getNtpTime);
  setSyncInterval(60 * 60);
}
// END Setup

void loop()
{

  // Test for wifi connection
  rssi = WiFi.RSSI();
  if (rssi == 0)
  {
    WiFi.disconnect();
    connectToNetwork();
  }

  timeClient.setTimeOffset(gmtOffset_sec);
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

#include <time-manager.h>

  // Get the push data json containing the produced today data, from your ftp server.
  // We only get this info once per hour, from the Fronius Data manager.

  // Get the data if time is either one minute past the hour, or this is the first run of the program
  if (thisMinute == 01 or firstRun == true)
  {
    getFTPmessage();
  }

  // We need the data from the last years production
  getLastYearsData();

  // We need the data from the last 12 months.
  getLast12monthsData();
  Serial.print("M1 check twelve last: ");
  Serial.println(twelveLast[1]);

  // We need the data from the Fronius Data Manager
  getFroniusJSONData();
  Serial.print("M2 check twelve last: ");
  Serial.println(twelveLast[1]);

  // Wait 20 seconds
  delay(20000);
}
