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

#define uS_TO_S_FACTOR 1000000ULL  //Conversion factor for micro seconds to seconds

//                 "2.0.1"; // Fixed null values when inverter is off line.
//                 "2.0.2"; // Minor error corrections
//                 "2.0.3"; // Major corrections, DST, Initiation, and FTP Backup
const String rel = "3.0.0"; // Change from Arduino EDI, to Visual Code EDI

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

HttpClient http(wifiClient, kHostname);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
ESP32_FTPClient ftp (ftp_server, ftp_user, ftp_pass);


// Date variables
String formattedDate;
String formattedFroniusDate;
String dayStamp;
int yearStamp;
String timeStamp;
String timeStamp1;  // maybe not used
String timeMinute;
int thisDay;

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
float  producedNow = 0.00;

// Start Subroutines

void Initialize() {
  //When initialising the program, it is important to set the current months data (lastMonth = current month) after initialisation, all initialisation variables have to be commented out.

  //writeFile(SPIFFS, "/lastMonth.txt", "10");

  //writeFile(SPIFFS, "/lastDay.txt", "01");

  //writeFile(SPIFFS, "/yearToLastMonthEnergy.txt", "5363");

  //correctManualStart = 17.57;  // prduktion this month until yesterday

  //char Last12Months[] = R"raw({"last12":{"Jan":"87","Feb":"192","Mar":"540","Apr":"808","Maj":"903","Jun":"903","Jul":"756","Aug":"762","Sep":"452","Okt":"309","Nov":"91","Dec":"67","lastTwelve":"0"}})raw";
  //writeFile(SPIFFS, "/last12.txt", Last12Months);

  //  char AllYears[] = R"raw({"lastYears":{"2013":"5160","2014":"5668","2015":"5830","2016":"5820","2017":"5410","2018":"6150","2019":"5790","2020":"5440","2021":"0","2022":"0","2023":"0","2024":"0","2025":"0"}})raw";
  //  writeFile(SPIFFS, "/allYears.txt", AllYears);

  Serial.println("DID YOU REMEMBER TO UNCOMMENT / COMMENT THE VARIABLES YOU WANT TO USE / NOT USE ---- IMPORTANT!!!!!!!!!!!");

  // END Initialisation
}

// If anything is wrong with connections, we go to sleep for 5 minutes and restart.
void goToDeepSleepFiveMinutes()
{
  Serial.print("Going to sleep... ");
  Serial.print("30");
  Serial.println(" sekunder");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake us up!

  esp_sleep_enable_timer_wakeup(30 * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}

// Write files
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    goToDeepSleepFiveMinutes();
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Read files
void readFile(fs::FS &fs, const char * path) {
  //  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    goToDeepSleepFiveMinutes();
  }

  //  Serial.println("- read from file:");
  while (file.available()) {
    delay(2);  //delay to allow byte to arrive in input buffer
    char c = file.read();
    readString += c;
  }
  //  Serial.println(readString);
  file.close();
}

// List files on module
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    goToDeepSleepFiveMinutes();
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    goToDeepSleepFiveMinutes();
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

// Read the Fronius daily output, which on the Data Manager is FTP'ed to your FTP Server.
void getFTPmessage() {
  firstRun = false;
  producedNow = 0;

  ftp.OpenConnection();

  //Change directory
  ftp.ChangeWorkDir("/docker/fronius");
  //Download the text file or read it
  String response = "";
  ftp.InitFile("Type A");
  ftp.DownloadString("fronius_push.json", response);
  Serial.println("The file content is: " + response);

  // Get the file size
  String       list[128];

  ftp.CloseConnection();

  DynamicJsonDocument docFTP(12544);

  // Parse JSON object
  DeserializationError errorFTP = deserializeJson(docFTP, response);
  if (errorFTP) {
    Serial.print(F("deserializeJson() FTP failed: "));
    Serial.println(errorFTP.c_str());
    goToDeepSleepFiveMinutes();
  }

  String producedToday = docFTP["Body"]["inverter/1"]["Data"]["EnergyReal_WAC_Sum_Produced"]["Values"].as<String>();
  // Serial.println(producedToday);
  // Define number of pieces
  int counter = 0;
  int lastIndex = 0;
  String partSum = "";
  for (int i = 0; i < producedToday.length(); i++) {
    // Loop through each character and check if it's a comma
    if (producedToday.substring(i, i + 1) == ",") {
      // Grab the piece from the last index up to the current position and store it
      partSum = producedToday.substring(lastIndex, i);
      if (i == 10) {
        // Serial.println(partSum);
        producedNow = producedNow + partSum.substring(9).toFloat();
      } else {
        producedNow = producedNow + partSum.substring(8).toFloat();
      }
      // Serial.println(producedNow);

      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if (i == producedToday.length() - 1) {
      // Grab the last part of the string from the lastIndex to the end
      partSum = producedToday.substring(lastIndex, i);
      producedNow = producedNow + partSum.substring(8).toFloat();
      //        Serial.println(producedNow);
    }
  }
}
// END getFTPmessage()

void getLastYearsData() {

  // Get the last years data, excl current year, which is not included.
  readString = "";
  readFile(SPIFFS, "/allYears.txt");
  Serial.print("readString all years: ");
  Serial.println(readString);

  DynamicJsonDocument lastYears(1024);

  // Parse JSON object
  DeserializationError errorYear = deserializeJson(lastYears, readString);
  if (errorYear) {
    Serial.print(F("deserializeJson() Last years production failed read: "));
    Serial.println(errorYear.c_str());
    goToDeepSleepFiveMinutes();
  }
  yearsLast[1] = lastYears["lastYears"]["2013"].as<float>();
  yearsLast[2] = lastYears["lastYears"]["2014"].as<float>();
  yearsLast[3] = lastYears["lastYears"]["2015"].as<float>();
  yearsLast[4] = lastYears["lastYears"]["2016"].as<float>();
  yearsLast[5] = lastYears["lastYears"]["2017"].as<float>();
  yearsLast[6] = lastYears["lastYears"]["2018"].as<float>();
  yearsLast[7] = lastYears["lastYears"]["2019"].as<float>();
  yearsLast[8] = lastYears["lastYears"]["2020"].as<float>();
  yearsLast[9] = lastYears["lastYears"]["2021"].as<float>();
  yearsLast[10] = lastYears["lastYears"]["2022"].as<float>();
  yearsLast[11] = lastYears["lastYears"]["2023"].as<float>();
  yearsLast[12] = lastYears["lastYears"]["2024"].as<float>();
  yearsLast[13] = lastYears["lastYears"]["2025"].as<float>();

}
// END getLastYearsData()



void getLast12monthsData() {
  // Get the last 12 months data, excl current month, which is not included.
  readString = "";
  readFile(SPIFFS, "/last12.txt");
  Serial.print("readString last 12: ");
  Serial.println(readString);

  DynamicJsonDocument last12Recv(1024);

  // Parse JSON object
  DeserializationError errorL12 = deserializeJson(last12Recv, readString);
  if (errorL12) {
    Serial.print(F("deserializeJson() Last 12 Recv failed: "));
    Serial.println(errorL12.c_str());
    goToDeepSleepFiveMinutes();
  }

  twelveLast[1] = last12Recv["last12"]["Jan"].as<float>();
  twelveLast[2] = last12Recv["last12"]["Feb"].as<float>();
  twelveLast[3] = last12Recv["last12"]["Mar"].as<float>();
  twelveLast[4] = last12Recv["last12"]["Apr"].as<float>();
  twelveLast[5] = last12Recv["last12"]["Maj"].as<float>();
  twelveLast[6] = last12Recv["last12"]["Jun"].as<float>();
  twelveLast[7] = last12Recv["last12"]["Jul"].as<float>();
  twelveLast[8] = last12Recv["last12"]["Aug"].as<float>();
  twelveLast[9] = last12Recv["last12"]["Sep"].as<float>();
  twelveLast[10] = last12Recv["last12"]["Okt"].as<float>();
  twelveLast[11] = last12Recv["last12"]["Nov"].as<float>();
  twelveLast[12] = last12Recv["last12"]["Dec"].as<float>();
  //  twelveLast[13] = last12Recv["LastTwelve"].as<float>();

  for (int i = 1; i < 13; i++) {
    twelveLastTotal += twelveLast[i];
    Serial.print("twelveLast(i)");
    Serial.println(twelveLast[i]);
  }
}
// END getLast12monthsData()

// Read the Fronius Data Module
void getFroniusJSONData() {
  // Get the JSON data from the Fronius Datamanager.
  if (!wifiClient.connect("192.168.1.239", 80)) {
    Serial.println(F("Connection failed"));
    goToDeepSleepFiveMinutes();
  }
  wifiClient.setTimeout(10000);

  Serial.println(F("Connected to Fronius Datamanager"));

  // Send HTTP request
  wifiClient.println(F("GET /solar_api/v1/GetInverterRealtimeData.cgi?Scope=Device&DeviceId=1&DataCollection=CommonInverterData HTTP/1.0"));
  wifiClient.println(F("Host: 192.168.1.239"));
  wifiClient.println(F("Connection: close"));
  if (wifiClient.println() == 0) {
    Serial.println(F("Failed to send request"));
    goToDeepSleepFiveMinutes();
  }

  Serial.println(F("Connected to Fronius Datamanager 1"));

  // Check HTTP status
  char status[32] = {0};
  wifiClient.readBytesUntil('\r', status, sizeof(status));
  Serial.print(F("wifiClient response: "));
  Serial.println(status);

  if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    goToDeepSleepFiveMinutes();
  }
  Serial.println(F("Connected to Fronius Datamanager 2"));
  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!wifiClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    goToDeepSleepFiveMinutes();
  }
  Serial.println(F("Connected to Fronius Datamanager 3"));
  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  // const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument doc(1792);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, wifiClient);
  if (error) {
    Serial.print(F("deserializeJson() doc failed: "));
    Serial.println(error.c_str());
    goToDeepSleepFiveMinutes();
  }
  Serial.println(F("Connected to Fronius Datamanager 4"));
  // Allocate a  JsonDocument
  StaticJsonDocument<1792> docSend;
  JsonObject root = docSend.to<JsonObject>();

  JsonObject now = root.createNestedObject("now");
  Serial.println(F("Connected to Fronius Datamanager 5"));
  // Set the values in the document
  Serial.print ("PAC ");
  String pacstr = doc["Body"]["Data"]["FAC"]["Value"].as<String>();
  Serial.println(pacstr);
  // Device changes according to device placement
  if (doc["Body"]["Data"]["PAC"]["Value"].as<String>() == "null") {
    now["PAC"] = 0;
  } else {
    now["PAC"] = doc["Body"]["Data"]["PAC"]["Value"].as<String>();
  }
  now["FAC"] = doc["Body"]["Data"]["FAC"]["Value"].as<String>();
  now["IAC"] = doc["Body"]["Data"]["IAC"]["Value"].as<String>();
  now["IDC"] = doc["Body"]["Data"]["IDC"]["Value"].as<String>();
  float totalEnergy = doc["Body"]["Data"]["TOTAL_ENERGY"]["Value"].as<float>() / 1000;
  now["TOTAL_ENERGY"] = String(totalEnergy);
  now["UAC"] = doc["Body"]["Data"]["UAC"]["Value"].as<String>();
  now["UDC"] = doc["Body"]["Data"]["UDC"]["Value"].as<String>();
  now["Day_Energy"] = String(producedNow / 1000);
  yearEnergy = doc["Body"]["Data"]["YEAR_ENERGY"]["Value"].as<float>() / 1000;
  now["YEAR_ENERGY"] = String(yearEnergy);
  String statusFronius = doc["Head"]["Status"]["Code"].as<String>();
  statusFroniusInt = statusFronius.toInt();
  now["Code"] = doc["Head"]["Status"]["Code"].as<String>();
  now["Reason"] = doc["Head"]["Status"]["Reason"].as<String>();
  now["UserMessage"] = doc["Head"]["Status"]["UserMessage"].as<String>();
  timeStampFronius = doc["Head"]["Timestamp"].as<String>();
  timeStampFronius = timeStampFronius.substring(0, 10) + " " + timeStampFronius.substring(11, 19);
  now["timeStamp"] = timeStampFronius;

  // END of data directly received from Fronius Converter
  Serial.println(F("Connected to Fronius Datamanager 6"));
  // *************************************
  // Now we have to check for month change
  // *************************************
  thisMonthInt = timeStampFronius.substring(5, 7);
  if (thisMonthInt != lastMonthInt) {
    Serial.println("Month is changing");
    // we need to move this month energy to the appropiate lastTwelve variable
    int lastMonthIntInt = lastMonthInt.toInt();
    twelveLast[lastMonthIntInt] = yearEnergy - yearToLastMonthEnergy - correctManualStart;
    Serial.print("Sidste måneds forbrug");
    Serial.println(twelveLast[lastMonthIntInt]);
    // update the yearToLastMonthEnergy to yearEnergy
    yearToLastMonthEnergy = yearEnergy - correctManualStart;
    Serial.print("Forbrug til månedsskivt");
    Serial.println(yearEnergy);
    writeFile(SPIFFS, "/yearToLastMonthEnergy.txt", String(yearToLastMonthEnergy).c_str());
    float twelveLastTotalLocal = 0;
    for (int i = 1; i < 13; i++) {
      twelveLastTotalLocal += twelveLast[i];
      Serial.print ("Last twelve months total: ");
      Serial.println(twelveLastTotalLocal);
    }
    // We move this integer to lastMonthInt, in order to maintain check for month change.
    lastMonthInt = thisMonthInt;
    writeFile(SPIFFS, "/lastMonth.txt", lastMonthInt.c_str());
    Serial.println(F("Connected to Fronius Datamanager 7"));
    // NEW NEW NEW NEW NEW

    File f12 = SPIFFS.open("/last12.txt", "w");


    StaticJsonDocument<512> doc12;
    JsonObject root = doc12.to<JsonObject>();

    JsonObject last12 = root.createNestedObject("last12");


    // Now we set the 12 last months production
    last12["Jan"] = String(twelveLast[1]);
    last12["Feb"] = String(twelveLast[2]);
    last12["Mar"] = String(twelveLast[3]);
    last12["Apr"] = String(twelveLast[4]);
    last12["Maj"] = String(twelveLast[5]);
    last12["Jun"] = String(twelveLast[6]);
    last12["Jul"] = String(twelveLast[7]);
    last12["Aug"] = String(twelveLast[8]);
    last12["Sep"] = String(twelveLast[9]);
    last12["Okt"] = String(twelveLast[10]);
    last12["Nov"] = String(twelveLast[11]);
    last12["Dec"] = String(twelveLast[12]);
    last12["lastTwelve"] = String(twelveLastTotalLocal);

    // Serialize JSON to file
    if (serializeJson(doc12, f12) == 0) {
      Serial.println(F("Failed to write to file"));
    }

    f12.close();
    Serial.println(F("Connected to Fronius Datamanager 8"));

    // END END END END END

    // We need to set the variables right.
    thisMonthsEnergy = 0;

    now["MONTH_ENERGY"] = String(thisMonthsEnergy);

  } else {
    // Så skal vi beregne månedsforbruget
    Serial.println("Vi er i maaneden");
    if (statusFroniusInt == 0) {

      thisMonthsEnergy = yearEnergy - yearToLastMonthEnergy + (producedNow / 1000);
      Serial.print("this months energy: ");
      Serial.println(thisMonthsEnergy);
      Serial.print("this year energy: ");
      Serial.println(yearEnergy);
      Serial.print("year to last month energy: ");
      Serial.println(yearToLastMonthEnergy);
      now["MONTH_ENERGY"] = String(thisMonthsEnergy);
    }
  }
  //  Serial.print(" check twelve last i maaneden: ");
  //  Serial.println(twelveLast[1]);
  Serial.println(F("Connected to Fronius Datamanager 9"));
  // Now we set the 12 last months production
  now["Jan"] = String(twelveLast[1]);
  now["Feb"] = String(twelveLast[2]);
  now["Mar"] = String(twelveLast[3]);
  now["Apr"] = String(twelveLast[4]);
  now["Maj"] = String(twelveLast[5]);
  now["Jun"] = String(twelveLast[6]);
  now["Jul"] = String(twelveLast[7]);
  now["Aug"] = String(twelveLast[8]);
  now["Sep"] = String(twelveLast[9]);
  now["Okt"] = String(twelveLast[10]);
  now["Nov"] = String(twelveLast[11]);
  now["Dec"] = String(twelveLast[12]);
  now["lastTwelve"] = String(twelveLastTotal);
  Serial.print("last twelve: ");
  Serial.println(twelveLast[13]);
  Serial.print("twelve last total: ");
  Serial.println(twelveLastTotal);
  twelveLastTotal = 0;

  // Now we set the last years production

  // First determinate which year:
  Serial.print("YearStamp 2020 ");
  Serial.println(yearStamp);

  if (yearStamp == 2020) {
    Serial.print("YearStamp 2020 ");
    Serial.println(yearStamp);
    yearsLast[8] = yearEnergy;
  }
  if (yearStamp == 2021) {
    yearsLast[9] = yearEnergy;
  }
  if (yearStamp == 2022) {
    yearsLast[10] = yearEnergy;
  }
  if (yearStamp == 2023) {
    yearsLast[11] = yearEnergy;
  }
  if (yearStamp == 2024) {
    yearsLast[12] = yearEnergy;
  }
  if (yearStamp == 2025) {
    yearsLast[13] = yearEnergy;
  }
  Serial.println(F("Connected to Fronius Datamanager 10"));
  now["thirteen"] = String(yearsLast[1]);
  now["fourteen"] = String(yearsLast[2]);
  now["fifteen"] = String(yearsLast[3]);
  now["sixteen"] = String(yearsLast[4]);
  now["seventeen"] = String(yearsLast[5]);
  now["eightteen"] = String(yearsLast[6]);
  now["nineteen"] = String(yearsLast[7]);
  now["twenty"] = String(yearsLast[8]);
  now["twentyone"] = String(yearsLast[9]);
  now["twentytwo"] = String(yearsLast[10]);
  now["twentythree"] = String(yearsLast[11]);
  now["twentyfour"] = String(yearsLast[12]);
  now["twentyfive"] = String(yearsLast[13]);

  // Connect to mqtt broker
  //  Serial.print("Attempting to connect to the MQTT broker: ");
  //  Serial.println(broker);
  if (!mqttClient.connected()) {
    Serial.println("You're NOT connected to the MQTT broker!");
    mqttClient.setServer(broker, 1883);
    if (!mqttClient.connect(broker, mqttuser, mqttpass)) {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.state());
      goToDeepSleepFiveMinutes();
    }
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Serial.println(F("Connected to Fronius Datamanager 11"));
  // Send to mqtt
  char buffer[1792];
  serializeJson(docSend, buffer);

  Serial.println("Sending message to topic: ");
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(buffer);

  // send message, the Print interface can be used to set the message contents
  bool retained = true;

  // Check for full transmission received
  if (doc["Head"]["Status"]["Code"].as<String>() != "8") {
  //  if (mqttClient.publish(topic, buffer, retained)) {
    if (mqttClient.publish(topic, buffer, retained)) {
      Serial.println("Message published successfully");
    } else {
      Serial.println("Error in Message, not published");
      mqttClient.disconnect();
      goToDeepSleepFiveMinutes();
    }

    Serial.println(F("Connected to Fronius Datamanager 12"));
  }

  Serial.println(F("Disconnect the MQTT broker"));
  mqttClient.disconnect();
  Serial.println(F("Connected to Fronius Datamanager 13"));
  Serial.println();


  if (thisDay != lastDayInt.toInt()) {

    // FTP to backup every day
    ftp.OpenConnection();
    //Change directory
    ftp.ChangeWorkDir("/docker/fronius");
    //upload the text file or read it
    String response = "";
    ftp.InitFile("Type A");
    ftp.NewFile("fronius_mqtt.json");
    ftp.Write(buffer);

    ftp.CloseConnection();
    lastDayInt = String(thisDay);
    writeFile(SPIFFS, "/lastDay.txt", lastDayInt.c_str());

  }
}
// END dailyUpdate()

// Actual routine to connect to the WIFI network
void connectToNetwork() {
  Serial.print("Size of SSID array ");
  Serial.println(ssidArrNo);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  bool breakLoop = false;
  for (int i = 0; i <= ssidArrNo; i++) {
    ssid = ssidArr[i].c_str();
    Serial.print("SSID name: ");
    Serial.println(ssidArr[i]);

    while ( WiFi.status() !=  WL_CONNECTED )
    {
      // wifi down, reconnect here
      WiFi.begin(ssid, password);
      int WLcount = 0;
      int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED )
      {
        delay( 100 );
        Serial.printf(".");
        if (UpCount >= 60)  // just keep terminal from scrolling sideways
        {
          UpCount = 0;
          Serial.printf("\n");
        }
        ++UpCount;
        ++WLcount;
        if (WLcount > 200) {
          Serial.println(" ");
          Serial.println("we should break");
          breakLoop = true;
          break;
        }
      }
      if (breakLoop) {
        breakLoop = false;
        break;
      }
    }
  }
  if (WiFi.status() !=  WL_CONNECTED) {
    goToDeepSleepFiveMinutes();
  }
}

void setup() {
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
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    goToDeepSleepFiveMinutes();
  }
  listDir(SPIFFS, "/", 0);

  // Initiate historic variables, at first start of program, or if adjustments are needed.
  if (initiate) {
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
}
// END Setup

void loop() {

  // Test for wifi connection
  rssi = WiFi.RSSI();
  if (rssi == 0) {
    WiFi.disconnect();
    connectToNetwork();
  }

  timeClient.setTimeOffset(gmtOffset_sec);
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  yearStamp = dayStamp.substring(0, 4).toInt();
  Serial.print("YearStamp set ");
  Serial.println(yearStamp);
  dayStamp = dayStamp.substring(5);
  String dateMonth = dayStamp.substring(0, 2);
  String dateDay = dayStamp.substring(3, 5);
  Serial.print("dateMonth: ");
  Serial.println(dateMonth);
  Serial.print("dateDay: ");
  Serial.println(dateDay);
  dayStamp = dateDay + "-" + dateMonth;
  Serial.print("dayStamp ");
  Serial.println(dayStamp);
  //  config.date = dayStamp;
  // Extract time
  timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.print("timeStamp1 ");
  Serial.println(timeStamp1);
  //  config.time = timeStamp1.substring(0, 5);
  Serial.print("Day ");
  Serial.println(timeClient.getDay());
  // variables needed for DST test
  int thisHour = timeClient.getHours();
  thisDay = dateDay.toInt();
  int thisMonth = dateMonth.toInt();
  //  int thisMonth = 2; // Test purposes
  int thisWeekday = timeClient.getDay();
  bool dst = false;

  // Test for DST active
  if (thisMonth == 11 && thisDay < 8 && thisDay < thisWeekday)
  {
    Serial.println("1");
    dst = true;
  }

  if (thisMonth == 11 && thisDay < 8 && thisWeekday == 1 && thisHour < 1)
  {
    Serial.println("2");
    dst = true;
  }

  if (thisMonth < 11 && thisMonth > 3) {
    Serial.println("3");
    dst = true;
  }

  if (thisMonth == 3 && thisDay > 7 && thisDay >= (thisWeekday + 7))
  {
    Serial.println("4");
    if (!(thisWeekday == 1 && thisHour < 2)) {
      Serial.println("5");
      dst = true;
    }
  }

  if (dst) {
    Serial.println("IN SOMMERTIME");
    timeClient.setTimeOffset(gmtOffset_sec + 3600);
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    dayStamp = dayStamp.substring(5);
    String dateMonth = dayStamp.substring(0, 2);
    String dateDay = dayStamp.substring(3, 5);
    Serial.print("dateMonth: ");
    Serial.println(dateMonth);
    Serial.print("dateDay: ");
    Serial.println(dateDay);
    dayStamp = dateDay + "-" + dateMonth;
    Serial.print("dayStamp ");
    Serial.println(dayStamp);
    //  config.date = dayStamp;
    // Extract time
    timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
    Serial.print("timeStamp1 ");
    Serial.println(timeStamp1);
    //  config.time = timeStamp1.substring(0, 5);
    Serial.print("Day ");
    Serial.println(timeClient.getDay());
  }

  // Get the push data json containing the produced today data, from your ftp server.
  // We only get this info once per hour, from the Fronius Data manager.
  int tMinute = timeMinute.toInt();
  // Get the data if time is either one minute past the hour, or this is the first run of the program
  if (tMinute == 01 or firstRun == true) {
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


  delay(20000);
}
