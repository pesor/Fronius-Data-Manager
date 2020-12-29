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