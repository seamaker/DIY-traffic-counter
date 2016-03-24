/*
  This gap counter will record the clear time between vehicles and the time vehicles block the sight line.
  Each row recorded will include the length of time obscured and the length of the subsequent gap.
  Code by Bob Edmiston
  Mon Mar 21, 2016 updated for use with ESP8266
*/
/*
 *  Simple HTTP get webclient test note: esp8266 feather only works with 1.65 of the board library
 *  4Mb Flash, 81K 
PINOUTS https://learn.adafruit.com/adafruit-feather-huzzah-esp8266?view=all
GND - this is the common ground for all power and logic
BAT - this is the positive voltage to/from the JST jack for the optional Lipoly battery
USB - this is the positive voltage to/from the micro USB jack if connected
EN - this is the 3.3V regulator's enable pin. It's pulled up, so connect to ground to disable the 3.3V regulator
3V - this is the output from the 3.3V regulator, it can supply 500mA peak (try to keep your current draw under 250mA so you have plenty for the ESP8266's power requirements!)
The TX pin is the output from the module and is 3.3V logic.
The RX pin is the input into the module and is 5V compliant (there is a level shifter on this pin)
I2C SDA = GPIO #4 (default)
I2C SCL = GPIO #5 (default)
SPI SCK = GPIO #14 (default) Also note that GPIO #12/13/14 are the same as the SCK/MOSI/MISO 'SPI' pins!
SPI MOSI = GPIO #13 (default)
SPI MISO = GPIO #12 (default)
This breakout has 9 GPIO: #0, #2, #4, #5, #12, #13, #14, #15, #16 arranged at the top edge
GPIO #0, which does not have an internal pullup, and is also connected a red LED. This pin is used by the ESP8266 to determine when to boot into the bootloader. If the pin is held low during power-up it will start bootloading! That said, you can always use it as an output, and blink the red LED.
GPIO #2, is also used to detect boot-mode. It also is connected to the blue LED that is near the WiFi antenna. It has a pullup resistor connected to it, and you can use it as any output (like #0) and blink the blue LED.
GPIO #15, is also used to detect boot-mode. It has a pulldown resistor connected to it, make sure this pin isn't pulled high on startup. You can always just use it as an output
GPIO #16 can be used to wake up out of deep-sleep mode, you'll need to connect it to the RESET pin
RST - this is the reset pin for the ESP8266, pulled high by default. When pulled down to ground momentarily it will reset the ESP8266 system. This pin is 3.3V logic only
EN (CH_PD) - This is the enable pin for the ESP8266, pulled high by default. When pulled down to ground momentarily it will reset the ESP8266 system. This pin is 3.3V logic only
SDA/SCL default to pins 4 & 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL)
On ESP8266, the SD CS pin is on GPIO 15, on Atmel M0 or 32u4 it's on GPIO 10. 
 */

//#include <Wire.h>
#include "RTClib.h"
//#include <Arduino.h>
//#include <Adafruit_LEDBackpack.h>
//#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
//#include <SoftwareSerial.h>

// A simple data logger for the Arduino analog pins
#define LOG_INTERVAL  5 // mills between entries
#define CLOUDTIMEFACTOR 3600; // 60 is minutes, 300 is 5 minutes, 3600 is hours.
#define LAG 0 // slow it down so measurements are accurate // was 30
//#define ECHO_TO_SERIAL   1 // echo data to serial port
#define SIMULATION 0 // when simulating, this generates random count increments.
//#define BLUELEDPIN 2
//#define LEDERROR 0  // digital pint for red error LED
#define SENSORPIN 0 // 15,16 lead to an infinite loop
//#define VOLTAGEPIN A
#define CHIPSELECT 15 // for the data logging shield, we use digital pin 15 on the ESP8266 for the SD cs line
#define BEEP 0
#define VOLTAGEFACTOR 55


// WLAN parameters
#define WLAN_SSID       "ssid"
#define WLAN_PASS       "pwd"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Carriots parameters
#define WEBSITE  "api.carriots.com"
#define API_KEY "key"
#define DEVICE  "device"

byte mac[] = { 0x18, 0xFE, 0x34, 0xD3, 0x84, 0x44 };
IPAddress ip(10,0,1,47);       // Your IP Address
IPAddress server(82,223,244,60);  // api.carriots.com IP Address
//Adafruit_7segment matrix = Adafruit_7segment();

RTC_PCF8523 rtc; // define the Real Time Clock object
DateTime now;
int lastUnix;
int currentUnix;
int posted = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

File logFile; // the logging file

unsigned long count = 0; // start the count at zero
unsigned long blockedTime;
unsigned long vacantTime;
unsigned long gapstartTime;
unsigned long gapendTime;
unsigned long blockstartTime;
unsigned long blockendTime;
int sensorValue = 0;  // variable to store the value coming from the sensor
int triggerSense = 0;
//int vsensorValue = 0;
unsigned long upTime;
unsigned long startupTime;

//volatile float sourceVolts;
String dataString = "";
String upTimeString = "";
String startupTimeString = "";
unsigned int distance;

void setup() {

  delay(2000);

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

#if ECHO_TO_SERIAL
  Serial.begin(115200);
  delay(1000);
  Serial.println("pin assignment begins");  
#endif

  //pinMode(LEDPIN, OUTPUT);   // declare the LEDPIN as an OUTPUT:
  //pinMode(LEDERROR, OUTPUT);   // declare the LEDERROR as an OUTPUT:
  pinMode(SENSORPIN, INPUT);   // declare the VOLTAGEPIN as an INPUT:
//  pinMode(BLUELEDPIN, OUTPUT);   // declare the 
  digitalWrite(SENSORPIN,HIGH);
//  digitalWrite(BLUELEDPIN,LOW);
//  pinMode(VOLTAGEPIN, INPUT);   // declare the VOLTAGEPIN as an INPUT:

 //Wire.begin(SDApin,SCLpin);
  
  //Serial.println("starting rtc");
  if (! rtc.begin()) {
    #if ECHO_TO_SERIAL
    Serial.println("Couldn't find RTC");
    #endif
    while (1);
  }
  now = rtc.now();Serial.println(now.year(), DEC);
  if (! rtc.initialized()) {
    #if ECHO_TO_SERIAL
    Serial.println("RTC is NOT running!");
    #endif
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  
  //Serial.println("year after rtc initialization");
  now = rtc.now();//Serial.println(now.year(), DEC);

  
  //pinMode(CHIPSELECT, OUTPUT); // for the data logging shield, we use digital pin 4 for the SD cs line
  //Wire.begin(SDApin,SCLpin);
  #if ECHO_TO_SERIAL
    Serial.println("opening fileio");
  #endif
      if (openFileIO() == 0) {
#if ECHO_TO_SERIAL
    Serial.println("failed to open file");
#endif  //ECHO_TO_SERIAL
      while(1);
    //exit(1); // if making a new file fails, halt.
  }
  
//matrix.begin(0x70);
//digitalWrite(LEDPIN, HIGH);
//digitalWrite(LEDERROR, LOW);
//  clearDisplay();  // Clears display, resets cursor
#if ECHO_TO_SERIAL
  Serial.println("year after file io initialization");
#endif
  now = rtc.now();//Serial.println(now.year(), DEC);
  gapstartTime = millis();

    // We start by connecting to a WiFi network
#if ECHO_TO_SERIAL
  Serial.println();
  Serial.println();
  Serial.print("MAC ");
  //WiFi.macAddress(mac); // returns the MAC address
  Serial.print(mac[0],HEX);Serial.print(" ");
  Serial.print(mac[1],HEX);Serial.print(" "); 
  Serial.print(mac[2],HEX);Serial.print(" ");
  Serial.print(mac[3],HEX);Serial.print(" ");
  Serial.print(mac[4],HEX);Serial.print(" ");
  Serial.print(mac[5],HEX);Serial.println(" ");
#endif
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #if ECHO_TO_SERIAL
    Serial.print(".");
    #endif
  }
#if ECHO_TO_SERIAL
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // reset the RTC clock to compile time, RTC clocks are a little drifty so uncomment every once in a while to update, but recomment before going disconnecting from programmer.
  now = rtc.now();
  lastUnix = now.unixtime()/CLOUDTIMEFACTOR;
  startupTime = now.unixtime()/60; // minutes 
  Serial.println("finished setup successfully");
} // setup

// Convert double to string
String doubleToString(float input,int decimalPlaces){
  if(decimalPlaces!=0){
    String string = String((int)(input*pow(10,decimalPlaces)));
      if(abs(input)<1){
        if(input>0)
          string = "0"+string;
        else if(input<0)
          string = string.substring(0,1)+"0"+string.substring(1);
      }
      return string.substring(0,string.length()-decimalPlaces)+"."+string.substring(string.length()-decimalPlaces);
    }
  else {
    return String((int)input);
  }
} // Convert double to string

void loop() {  // begin the relatime loop at zero
  delay (LOG_INTERVAL);  // Debouncing delay. Wait this long of clear time before taking another measurement.
  now = rtc.now(); // record the time and date now so it can be written to the file when the row is written.
  currentUnix = now.unixtime()/CLOUDTIMEFACTOR; // 60 minute, 300 five minute, 3600 hour
  upTime = (now.unixtime()/60) - startupTime; // A measure of uptime in seconds. Number of seconds since we powered on the unit.
  sensorValue = digitalRead(SENSORPIN); // see if any obstructions are present
  triggerSense = sensorValue;  // save the observed value for writing the the file because sensorValue is reused to check for vacancy
  
  if ((currentUnix > lastUnix)&&(!posted)) { // post data to cloud begin
      lastUnix = currentUnix;

      // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(WEBSITE, httpPort)) {
      #if ECHO_TO_SERIAL
      Serial.println("connection failed");
      #endif
      return;
    }
    // Convert data to String
    String upTimesString = doubleToString(upTime,0);
    String dailyCountString = doubleToString(count,0);
   
    // Prepare JSON for Carriots & get length
    int length = 0;
    String data = "{\"protocol\":\"v2\",\"device\":\""+String(DEVICE)+"\",\"at\":\"now\",\"data\":{\"Uptime\":"+String(upTime)+",\"TDCount\":"+String(count)+",\"Volts\":"+String(doubleToString(lastUnix,0))+"}}";
    length = data.length();
    #if ECHO_TO_SERIAL
    Serial.print("Data length");
    Serial.println(length);
    Serial.println();
    
      // Print request for debug purposes
    Serial.println("POST /streams HTTP/1.1");
    Serial.println("Host: api.carriots.com");
    Serial.println("Accept: application/json");
    Serial.println("User-Agent: Arduino-Carriots");
    Serial.println("Content-Type: application/json");
    Serial.println("carriots.apikey: " + String(API_KEY));
    Serial.println("Content-Length: " + String(length));
    Serial.print("Connection: close");
    Serial.println();
    Serial.println(data);
    #endif
    if (client.connected()) {
      #if ECHO_TO_SERIAL
      Serial.println("Connected!");
      #endif
      client.println("POST /streams HTTP/1.1");
      client.println("Host: api.carriots.com");
      client.println("Accept: application/json");
      client.println("User-Agent: Arduino-Carriots");
      client.println("Content-Type: application/json");
      client.println("carriots.apikey: " + String(API_KEY));
      client.print("Content-Length: ");
      int thisLength = data.length();
      client.println(thisLength);
      client.println("Connection: close");
      client.println();
      client.println(data);

      //https://dweet.io/dweet/for/impacthubseafrontdoor?hello=upTime&foo=count;
      
      count = 0;
      posted = 1;
      
    } else {
      #if ECHO_TO_SERIAL
      Serial.println(F("Connection failed"));    
      #endif
      return;
    }
    count = 0; // count turns into a pumpkin at midnight if over 3600 seconds since last upload
    posted = 0;
  } // post data to cloud end
  if (SIMULATION) {
    count += random(9);
  }
  if (!sensorValue) { // if there is voltage, then something is present in the beam so this is the end of the gap and beginning of the block.  //was +40 ms
    #if ECHO_TO_SERIAL
      Serial.println("starting to log a recording");
    #endif  
      delay(LOG_INTERVAL);
    gapendTime = millis(); // timestamp the end of the gap in traffic because something blocked the beam
    blockstartTime = millis(); // timestamp the start of the blockage of the beam.  Same as gap end timestamp.
    count++;  // increment the number of beam blockages counted since power on.
    
    //digitalWrite(LEDPIN, LOW);   // turn the Green LED off because the beam is blocked.
    //digitalWrite(LEDERROR, HIGH); // turn on the Red LED because the beam is blocked.
    
#if BEEP  // for counters that have the buzzer implemented, buzz when there is obstruction.  Typically used for debugging because the buzzer is annoying in the field.
    tone(6, 4000); // play a note on pin 6 until the beam is cleared
#endif
//   clearDisplay();  // Clears display, resets cursor
   
   while (!sensorValue) { // wait until the beam is cleared.  When sense voltage drops below BASELINE, the view is clear.
      sensorValue = digitalRead(SENSORPIN);  // read the sensor to see if it's still blocked.
      //writeLED((millis()-gapendTime)/1000); // display the length of the blockage in integer seconds 
      blockedTime = (millis()-gapendTime);
      //digitalWrite(BLUELEDPIN,HIGH);
//      clearDisplay();  // Clears display, resets cursor
//      delay(80);
//      writeLED(sensorValue);
//      delay(300);
#if ECHO_TO_SERIAL
    Serial.print(blockedTime);   Serial.print(" ");Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);Serial.print(" ");Serial.print(lastUnix);
    Serial.println(" blockedms");
    #endif
    delay(LOG_INTERVAL);
   }
   //digitalWrite(BLUELEDPIN,LOW);
   #if ECHO_TO_SERIAL
   Serial.println("thank you");
   #endif

    blockendTime = millis();
    vacantTime = gapendTime - gapstartTime; // The gap in traffic in milliseconds is the time from when the beam last cleared until it got blocked again.
#if ECHO_TO_SERIAL    
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);Serial.print(' ');
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
#endif
    // assemble the comma delimited string for output to serial port and to file io
    dataString = "";  // clear out the data string since we last used it.
    dataString += now.year();dataString += ('/');
    dataString += now.month();dataString += ('/');
    dataString += now.day();
    dataString += " "; 
    dataString += now.hour();  dataString += (':');
    dataString += now.minute(); dataString += (':');
    dataString += now.second(); dataString += (',');
    dataString += now.year(); dataString += (',');
    dataString += now.month(); dataString += (',');
    dataString += now.day(); dataString += (',');
    dataString += now.hour(); dataString += (',');
    dataString += now.minute(); dataString += (',');
    dataString += now.second(); dataString += (',');
    dataString += daysOfTheWeek[now.dayOfTheWeek()];dataString += (',');
    dataString += String(upTime); dataString += (',');
//    vsensorValue = analogRead(VOLTAGEPIN);
//    sourceVolts = float (vsensorValue / VOLTAGEFACTOR);
//    dataString += String(sourceVolts); dataString += (',');
    dataString += String(vacantTime); dataString += (',');
    dataString += String(blockendTime - blockstartTime); dataString += (',');
    dataString += ('1'); dataString += (',');
    dataString += String(count);

    logFile.println(dataString); // write data string to file
    logFile.flush(); // The following line will 'save' the file to the SD card after every
#if ECHO_TO_SERIAL
    Serial.println(dataString); // print to the serial port too:
#endif  //ECHO_TO_SERIAL

    //writeLED(count); // display the total count so far to the display.  remains until the next break
    //digitalWrite(LEDPIN, HIGH); //green on
    //digitalWrite(LEDERROR, LOW); // red off
    gapstartTime = millis();
//    clearDisplay();  // Clears display, resets cursor
  } else {
    //clearDisplay();  // Clears display, resets cursor
    vacantTime = (millis() - gapstartTime)/1000;
    //vsensorValue = analogRead(VOLTAGEPIN);
    //sourceVolts = float (vsensorValue / VOLTAGEFACTOR);
    //writeLED(sourceVolts);
    //writeLED(vacantTime);
    //writeLED(count);
    delay(LOG_INTERVAL);
  }

} // loop

int openFileIO() {
  // initialize the SD card
#if ECHO_TO_SERIAL
  Serial.print("Waking up the SD card...");
#endif  //ECHO_TO_SERIAL

  if (!SD.begin(CHIPSELECT)) {
#if ECHO_TO_SERIAL
    Serial.println("Card failed, or not present");
#endif  //ECHO_TO_SERIAL
    // don't do anything more:
    return 0;
  }

#if ECHO_TO_SERIAL
  Serial.println("card initialized.");
#endif  //ECHO_TO_SERIAL
  // create a new file
  char filename[] = "LOGS0000.CSV";

  for (uint8_t i = 0; i < 100; i++) {
#if ECHO_TO_SERIAL
    Serial.println(filename);
#endif  //ECHO_TO_SERIAL
    filename[4] = i / 1000 + '0';
    filename[5] = i / 100 + '0';
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logFile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
      delay(80);
    }
  }

  if (!logFile) {
#if ECHO_TO_SERIAL
    //Serial.println(filename);
#endif  //ECHO_TO_SERIAL
    error("shit, no file");
  }
  dataString = "MM/DD/YYYY HH:MM:SS,YYYY,MM,DD,HH,MM,SS,UPTIMEs,VOLTSv,GAPms,BLOCKEDms,I,N";
#if ECHO_TO_SERIAL
  Serial.print("Logging to: ");
  Serial.println(filename);
  Serial.println(dataString);
#endif  //ECHO_TO_SERIAL
  logFile.println(dataString); // write data string to file
  return 1;
}


//void writeLED(unsigned long amount) { //called only when the beam has been broken
//  // try to print a number thats too long
//  matrix.println(amount);
//  matrix.writeDisplay();
//}

void error(char errorstring[]) // when the file writing fails.
{
#if ECHO_TO_SERIAL
  Serial.print("error: ");
  Serial.println(errorstring);
#endif  //ECHO_TO_SERIAL 
  // red LED indicates error
  //digitalWrite(LEDERROR, HIGH);
  logFile.println(errorstring);
  while (1); // halt
}


// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
//void clearDisplay()
//{
//  matrix.println(0);
//  matrix.writeDisplay();
//}

////  Wiring connections
//  Sparkfun RedBoard $24.95 Amazon Prime ASIN: B00BFGZZJO
//  Adafruit Assembled Data Logging shield for Arduino $19.95 + $1.95 stackable headers
//  Sanyo eneloop 2000 mAh typical, 1900 mAh minimum, 1500 cycle, 8 pack AA, Ni-MH Pre-Charged Rechargeable Batteries $20.19 Model: SEC-HR3U8BPN
//  Adafruit Diffused Green 3mm LED (25 pack) - $4.95  ID: 779
//  Adafruit 9V battery clip with 5.5mm/2.1mm plug -  ID: 80  could be better with a 90 degree barrel plug for clearance $3
//  Adafruit Diffused Red 3mm LED (25 pack) -$4.95 ID: 777
//  Adafruit 0.56" 4-Digit 7-Segment Display w/I2C Backpack - White ID:1002 $12,95 plus shipping
//  Optional: Adafruit In-line power switch for 2.1mm barrel jack - ID: 1125  Watch for faulty female connector, had to replace. $3
//  220 ohm  resistor $0.99
//  Radio Shack battery holder $2.99 RadioShack® 8 “AA” Battery Holder Model: 270-407  | Catalog #: 270-407 $3
//
//  IR Sensor Sharp GP2Y0A710K Distance Sensor (100-550cm) - DFRobot.com, got on Amazon for $34.96 out the door.
//  Black x 2 -> Ground
//  Red x 2 -> +5V
//  Blue -> A0 3.3v max so fine with Seeeduino Stalker logic
//
//  Red Error LED
//  D8 -> Anode ->  1K Resistor -> Ground
//
//  Green Count LED
//  D9 -> Anode ->  1K Resistor -> Ground
//
//  Adafruit 0.56" 4-Digit 7-Segment Display w/I2C Backpack (optional)
//  Wiring to the matrix is really easy
//  Connect CLK SCL to the I2C clock - on Arduino UNO thats Analog #5, on the Leonardo it's Digital #3, on the Mega it's digital #21
//  Connect DAT SDA to the I2C data - on Arduino UNO thats Analog #4, on the Leonardo it's Digital #2, on the Mega it's digital #20
//  Connect GND to common ground
//  Connect VCC+ to power - 5V is best but 3V also seems to work for 3V microcontrollers.  3v is dimmer which is better


