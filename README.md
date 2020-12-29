# Fronius Data Logger
## Fronius Data Logger - MQTT autodiscover interface for Homeassistant

<a href="https://github.com/pesor/Fronius-Data-Manager/releases"><img src="https://img.shields.io/github/v/release/pesor/Fronius-Data-Manager?style=plastic"/> </a><a href="https://github.com/pesor/Fronius-Data-Manager/blob/master/LICENSE"><img src="https://img.shields.io/github/license/pesor/Fronius-Data-Manager?style=plastic"/></a>  <a href="https://github.com/pesor/Fronius-Data-Manager/stargazers"><img src="https://img.shields.io/github/stars/pesor/Fronius-Data-Manager?style=plastic"/></a>  <a href="https://github.com/pesor/Fronius-Data-Manager/releases"><img src="https://img.shields.io/github/downloads/pesor/Fronius-Data-Manager/total?style=plastic"/></a>

### Getting started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

1. Fronius Inverter, with Data Manager Card, or Data Manager Box

2. Windows 10, with installed Arduino EDI (my version 1.8.12) - Used by VSCode and PlatformIO

3. VSCode and PlatformIO

4. An ESP32DEV board, I used this from AliExpress: https://www.aliexpress.com/item/32849567377.html?spm=a2g0s.9042311.0.0.27424c4dcQpFO7 but others can be used.

5. USB Cable to attatch to the ESP32DEV board

6. MQTT server (I am running on a Synology NAS in docker)
   If you have a Synology NAS, I can recommend to follow [BeardedTinker](https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g) on YouTube, he makes a very intuitive explanation how to setup the whole Home Assistant environment on Synology.   

7. You will need the following in your configuration.yaml file: 

   **python_script:**

   **mqtt:**
     **broker: 192.168.1.64**
     **discovery: true**
     **discovery_prefix: homeassistant** (Default, it is the folder where you have your configuration.yaml file)

8. [](https://https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)

   

### Installing

Below a step by step that tell you how to get a development/production environment up and running, and to make things even more easy,  [BeardedTinker](https://www.youtube.com/channel/UCuqokNoK8ZFNQdXxvlE129g)  have created the two tutorials on YouTube, which gives a detailed instruction how to get it all to work. 

He has performed a tremendous task in doing this.

The video below is for the TTGO H-Grow card, but the first part shows, **how to install the INO/Arduino EDI on your Windows 10**, and you can then skip the part, that is concerning the plant card:  https://www.youtube.com/watch?v=7w6_ZkLDxko&t=231s    

I highly recommend that your see and follow this video if you do not already have the Arduino EDI on your Windows 10, then you will have success in setting the up Arduino EDI.

After installing the INO/Arduino EDI, you need to install VSCode and PlatformIO, there are good videos on youtube how to do that, just Google them.

After seeing the videos, remember to give a "Thumbs Up" to support BeardedThinker in his work.

### The Arduino Sketch

The main program here is the:

### 			main.cpp

You just use this main.cpp and set the variables needed in order to get it to run.

Few things of importance:

 1. First identify the ***// Start user defined data*** in the main.cpp

 2. Now you have to define your SSID's, you can have as many as you like, I have at the moment 4, probably going to 5 soon. You update the variable **ssidArr** with your access points, each separated by a comma. The variable **ssidArrNo** must be filled with the number of SSID's given.

 8. You then gives the Password for your SSID's (expected to all have the same).

 9. You now adjust to your time zone, by giving the numbers of hours multiplied by 3600.

 5. You need to give your credentials to your FTP server, I plan in a later release to include the server on the ESP32.

 6. The last thing to do, is to give in the information for your MQTT broker.

    

    Upload main.cpp to the ESP Board, and

    ​																					**YOU ARE DONE with first part**


## Which information do I get?

The data from the Kamstrup Unipower is translated into the following json format, and sent to your mqtt broker.

(Note that I also have Sun cells on the roof, sending power to the grid via a Fronius inverter. Se my solution for Fronius here :)

```json
{
  "FAC": {
    "FAC": "50"
  },
  "IAC": {
    "IAC": "1.47"
  },
  "IDC": {
    "IDC": "0.69"
  },
  "PAC": {
    "PAC": "264"
  },
  "TOTAL_ENERGY": {
    "TOTAL_ENERGY": "45173.00"
  },
  "UAC": {
    "UAC": "236"
  },
  "UDC": {
    "UDC": "343"
  },
  "Day_Energy": {
    "Day_Energy": "4.99"
  },
  "YEAR_ENERGY": {
    "YEAR_ENERGY": "5324.00"
  },
  "Status": {
    "Code": "0"
  },
  "Reason": {
    "Reason": ""
  },
  "UserMessage": {
    "UserMessage": ""
  },
  "timeStamp": {
    "timeStamp": "2020-09-28 17:22:43"
  },
  "MONTH_ENERGY": {
    "MONTH_ENERGY": "5328.99"
  },
  "Jan": {
    "Jan": "87.00"
  },
  "Feb": {
    "Feb": "192.00"
  },
  "Mar": {
    "Mar": "540.00"
  },
  "Apr": {
    "Apr": "808.00"
  },
  "Maj": {
    "Maj": "903.00"
  },
  "Jun": {
    "Jun": "846.00"
  },
  "Jul": {
    "Jul": "-3377.00"
  },
  "Aug": {
    "Aug": "0.00"
  },
  "Sep": {
    "Sep": "452.00"
  },
  "Okt": {
    "Okt": "301.00"
  },
  "Nov": {
    "Nov": "91.00"
  },
  "Dec": {
    "Dec": "67.00"
  },
  "lastTwelve": {
    "lastTwelve": "910.00"
  },
  "thirteen": {
    "thirteen": "5160.00"
  },
  "fourteen": {
    "fourteen": "5668.00"
  },
  "fifteen": {
    "fifteen": "5830.00"
  },
  "sixteen": {
    "sixteen": "5820.00"
  },
  "seventeen": {
    "seventeen": "5410.00"
  },
  "eightteen": {
    "eightteen": "6150.00"
  },
  "nineteen": {
    "nineteen": "5790.00"
  },
  "twenty": {
    "twenty": "5324.00"
  },
  "twentyone": {
    "twentyone": "0.00"
  },
  "twentytwo": {
    "twentytwo": "0.00"
  },
  "twentythree": {
    "twentythree": "0.00"
  },
  "twentyfour": {
    "twentyfour": "0.00"
  },
  "twentyfive": {
    "twentyfive": "0.00"
  }
}
```



## The Python Part - The Autodiscover - MAGIC

The Python script named: 

### 																																		fronius-aut.py**

you find it in the scr folder, in folder Autodiscovery.

you copy this into the folder **python_scripts** in your Home Assistant config folder, If you do not have that folder already, you need to create it. 

The Home Assistant config folder, is where you also have your configuration.yaml file.

You find and execute the Python script, in the menu "Developer Tools"/"SERVICES", where you will find it named:

### 														python_script.fronius-aut

You just press the "CALL SERVICE" button, and all the sensors from mqtt will be autodiscovered and added to your menu "Configuration"/"integrations", where you will find the MQTT integration, and here you can select the devices, and then select Solar_Power, and all sensors will be shown. You can use the "ADD TO LOVELACE" function, or you can add them manually to any LOVELACE card you want.

### Running

The Fronius ESP32, will every 20 seconds, read the Fronius Data Manager messages. These message will be translated, and the data will be sent to the MQTT server/broker, and at the same time they will be updated in Home Assistant.

This is the information I get on my Home Assistant Overview Fronius (in Danish, but I have kept the original naming, so all names are set in the Card):

![](https://github.com/pesor/Fronius-Data-Manager/blob/master/images/LoveLaceCard.JPG)

### Deployment

See instructions under **Prerequisites**

### Versioning

2.0.0 First official release
2.0.1 Stop MQTT for updating when Fronius Inverter is off line at night.

3.0.0 Changed from Arduino EDI, into VSCode and PlatformIO

### Authors

* **Per Rose** 
* BeardedTinker (contributer)

### License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details



**If you like and use my program, then** 

​       [![BMC](https://www.buymeacoffee.com/assets/img/custom_images/white_img.png)](https://www.buymeacoffee.com/pesor)

**it will be appreciated.**




