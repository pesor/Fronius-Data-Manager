void Initialize() {
  //When initialising the program, it is important to set the current months data (lastMonth = current month) after initialisation, all initialisation variables have to be commented out.

  //writeFile(SPIFFS, "/lastMonth.txt", "10");

  //writeFile(SPIFFS, "/lastDay.txt", "01");

  //writeFile(SPIFFS, "/yearToLastMonthEnergy.txt", "5363");

  //correctManualStart = 17.57;  // prduktion this month until yesterday

  char Last12Months[] = R"raw({"last12":{"Jan":"87","Feb":"192","Mar":"540","Apr":"808","Maj":"903","Jun":"903","Jul":"756","Aug":"762","Sep":"452","Okt":"309","Nov":"120","Dec":"67","lastTwelve":"0"}})raw";
  writeFile(SPIFFS, "/last12.txt", Last12Months);

  //  char AllYears[] = R"raw({"lastYears":{"2013":"5160","2014":"5668","2015":"5830","2016":"5820","2017":"5410","2018":"6150","2019":"5790","2020":"5440","2021":"0","2022":"0","2023":"0","2024":"0","2025":"0"}})raw";
  //  writeFile(SPIFFS, "/allYears.txt", AllYears);

  Serial.println("DID YOU REMEMBER TO UNCOMMENT / COMMENT THE VARIABLES YOU WANT TO USE / NOT USE ---- IMPORTANT!!!!!!!!!!!");

  // END Initialisation
}
