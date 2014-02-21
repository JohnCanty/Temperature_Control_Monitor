#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUDP.h>
#include <Wire.h>
//#include <SD.h>


byte mac[] = { 0xDE, 0xAD, 0xFE, 0xED, 0xDE, 0xAD };
const byte chipSelect = 4;
const int NTP_PACKET_SIZE = 48; //Pull the time stamp from the first 48 Bytes
const int DS1307 = 0x68; // Address of DS1307 see data sheets
//const char* days[] =
//{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//const char* months[] =
//{"January", "February", "March", "April", "May", "June", "July", "August","September", "October", "November", "December"};
byte packetBuffer[NTP_PACKET_SIZE]; //Buffer to hold incoming and out going
IPAddress timeServer(192, 168, 10, 2);  //Local time server Warning time in GMT
unsigned int localPort = 8888; //local port to listen to UDP
unsigned long secsSince1900;
unsigned long zoneOffset = 18000;
//unsigned long localTime;
//unsigned long localDTime;
unsigned long epoch;
byte start;
byte UtcHr;
byte EstHour;
byte EdtHour;
byte ExtMin;
byte ExtSec;
byte second = 0;
byte minute = 0;
byte hour = 0;
byte weekday = 0;
byte monthday = 0;
byte month = 0;
byte year = 0;
byte startup;
//Create a UDP instance
EthernetUDP Udp;

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH); //Disable SD Card
  delay(500); //wait half a second before the ethernet INIT
    Serial.println("Start of Ethernet");
  if(Ethernet.begin(mac) == 0) { 
    Serial.println("Obtaining Address From DHCP Server");
    while(true); //keep in loop if needed
  }
  delay(10000); //giving the ethernet module five seconds to get started
  Serial.println("The IP Address of this Microcontroller is: ");
  IPAddress myIPAddress = Ethernet.localIP();
  Serial.println(myIPAddress);
 
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialiezed:
  /*if (!SD.begin(4)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }*/
  Serial.println("card initialized.");
  Udp.begin(localPort); //Start listening
  startup = true;

}
void loop() {
  start = cycle(start);
  Serial.println(start);
  // make a string for assembling the data to log:
  String dataString = "";
  // read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2) {
      dataString += ","; 
    }
    Serial.println(dataString);
    if (startup == true){
      Serial.println("startup is true");
      //sendNTPpacket(); //NTP request packet AKA Get the damn time
      Serial.println("ntp packet sent");
      //ntp(); // Handle the damn time that came back
      delay(2000);
      readTime(); //Pull Time From RTC
      delay(500);
      //NTPUpdate(false);
      delay(200);
      //setTime(year, month, monthday, weekday, hour, minute, second);
      Serial.print(year);
      Serial.print (":");
      Serial.print(month);
      Serial.print(":");
      Serial.print(monthday);
      Serial.print(":");
      Serial.print(hour); 
      Serial.print(":"); 
      Serial.print(minute);
      Serial.print(":");
      Serial.println(second);
    startup = false; 
    }
  }
//call writestring use string 'datalog.txt' as the filename and dataString as the input.
 // writestring("datalog1.txt" , dataString); 
 
}
/* This is nothing more than a cyclic counter, counts to 254 then resets
back to zero. This function pulls in a byte and ejects (returns) a byte.*/
byte cycle(byte Cycle){
   if ( Cycle < 254 && Cycle >= 0) { 
    Cycle ++;
    delay(10);
  }
  else {
    Cycle = 0;
}
return Cycle;
}
byte decToBcd(byte val) {
  return ((val/10*16) + (val%10));
}
byte bcdToDec(byte val) {
  return ((val/16*10) + (val%16));
}
/* This function takes 2 inputs as strings. The first is the filename to be opened
the second is the string to write into that file. SD.open is fed with a character
array, therefore the string of 'X' needs to be converted the +1 is the EOL, 
required by ANSI standard.*/
/*void writestring(String X, String Y){
  char Filename [X.length()+1]; //create character array variable named Filename make it the length of variable X +1 for EOL
  X.toCharArray(Filename, sizeof(Filename)); //Convert 'X' to character array and dispense into Filename as the size of Filename.
  File dataFile = SD.open(Filename , FILE_WRITE); //Open the File and prepare to Write.
  String dataString = Y; //This is the Refuse going into the file
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close(); // be sure to close it when done.
    // print to the serial port too:
    Serial.println(dataString);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
}*/
void ntp() {
  if ( Udp.parsePacket() ) {
    delay(500);
    Udp.read(packetBuffer,NTP_PACKET_SIZE); //Dump what was read into the buffah
    unsigned long hi = word(packetBuffer[40], packetBuffer[41]);
    unsigned long low = word(packetBuffer[42], packetBuffer[43]);
       unsigned long secsSince1900 = hi << 16 | low;  
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    Serial.println(epoch);                               


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    UtcHr = ((epoch  % 86400L) / 3600);
    Serial.print(UtcHr); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    ExtMin = ((epoch  % 3600) / 60);
    if ( ExtMin < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
      Serial.print(ExtMin); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    ExtSec = (epoch % 60);
    if ( ExtSec < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(ExtSec); // print the second
  

 Serial.print("EST Local time is ");       // UTC is the time at Greenwich Meridian (GMT)
    unsigned long localTime = (epoch - zoneOffset);
    EstHour = ((localTime  % 86400L) / 3600); 
    Serial.print(EstHour); // print the hour (86400 equals secs per day 3600 seconds in an hour)
    Serial.print(':');     
    if ( ExtMin < 10 ) Serial.print('0');       // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print(ExtMin); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    
    if ( ExtSec < 10 ) Serial.print('0');  // In the first 10 seconds of each minute, we'll want a leading '0' 
    Serial.println(ExtSec); // print the second
  

  Serial.print("EDT Local time is ");       // UTC is the time at Greenwich Meridian (GMT)
    unsigned long localDTime = (epoch - zoneOffset + 3600);
    EdtHour = ((localDTime  % 86400L) / 3600);
    Serial.print(EdtHour); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    if ( ExtMin < 10 ) Serial.print('0');    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print(ExtMin); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    if ( ExtSec < 10 ) Serial.print('0');   // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.println(ExtSec); // print the second
  }
return;
}
unsigned long sendNTPpacket() {
  Serial.println("got to ntp packet function");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);   // set all bytes in the buffer to 0 
  /* Initialize values needed to Harvest NTP request */
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Serial.println("packet buffer initialized");
  /* all NTP fields have been given values, now    *
   * you can send a packet requesting a timestamp: */		   
  Udp.beginPacket(timeServer, 123); //NTP requests are to port 123
  Serial.println("UDP begin");
  Udp.write(packetBuffer,NTP_PACKET_SIZE); //Drop it all in the packetBuffer
  Serial.println("packet sent");
  Udp.endPacket(); //End this circus
  Serial.println("circus end packet");
  delay(500);
}
 void setTime(byte T, byte U, byte V, byte W, byte X, byte Y, byte Z) {
  Serial.print("Please enter the current year, 00-99. - ");
  year = T;
  Serial.println(year);
  Serial.print("Please enter the current month, 1-12. - ");
  month = U;
  //Serial.println(months[month-1]);
  //Serial.print("Please enter the current day of the month, 1-31. - ");
  monthday = V;
  Serial.println(monthday);
  Serial.println("Please enter the current day of the week, 1-7.");
  Serial.print("1 Sun | 2 Mon | 3 Tues | 4 Weds | 5 Thu | 6 Fri | 7 Sat - ");
  weekday = W;
  //Serial.println(days[weekday-1]);
  //Serial.print("Please enter the current hour in 24hr format, 0-23. - ");
  hour = X;
  Serial.println(hour);
  Serial.print("Please enter the current minute, 0-59. - ");
  minute = Y;
  Serial.println(minute);
  second = Z;
  Serial.println("The data has been entered.");

  // The following codes transmits the data to the RTC
  Wire.beginTransmission(DS1307);
  Wire.write(byte(0));
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekday));
  Wire.write(decToBcd(monthday));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(byte(0));
  Wire.endTransmission();
  // Ends transmission of data
}
void readTime() {
  Wire.beginTransmission(DS1307);
  Wire.write(byte(0));
  Wire.endTransmission();
  Wire.requestFrom(DS1307, 7);
  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read());
  weekday = bcdToDec(Wire.read());
  monthday = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());
}
void NTPUpdate (byte X) {
 second = ExtSec;
 minute = ExtMin;
  if (X == true) {
   hour = EdtHour;
 } else {
   hour = EstHour;
 }
}

