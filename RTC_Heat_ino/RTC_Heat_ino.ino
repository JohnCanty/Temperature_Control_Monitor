/*
RTC Programmer NTP
Configure a DS1307 RTC using the time collected from an NTP server. Netowkring configured using DHCP

Requires
Arduino Ethernet Shield
DS1307 RTC
Pinout: +5V Vcc
GND GND
A4 SCL
A5 SQW
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <RTClib.h>
#include <dht11.h>



//RTC Encapsulation
RTC_DS1307 RTC;
dht11 DHT11;
#define DHT11PIN 2
byte mac[] = { 0xDE, 0xAD, 0xFE, 0xED, 0xDE, 0xAD };//MAC Address for Ethernet Shield
unsigned int localPort = 8888; //Local port to listen for UDP Packets
IPAddress timeServer(192, 168, 10, 2);//NTP Server IP 
const int NTP_PACKET_SIZE= 48; //NTP Time stamp is in the firth 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //Buffer to hold incomming and outgoing packets
EthernetUDP Udp; //UDP Instance to let us send and recieve packets
unsigned long epoch; //Unix Epoch time (NTP or RTC depending on state)
int eepromaddress = 0;
float temperatureDegF; //Ambient Temperature in degrees F
int currentTemp; //Current temp in Integer form
int temperatureSPF; //Temperature set point in Degrees F
int temperatureDBF; //Temperature dead band in Degrees F
//byte winterTime = true; // Is it winter time
byte override = false; //override button, will turn on and off heat forcibly
byte laststate;
byte currentstate;
float humidity;
float temperature;
float DegF;
float kelvin;
float DewC;
float DewF;

void setup() {
  Serial.begin(115200); //Start the serial interface

  Wire.begin(); //Start Wire
  
  RTC.begin(); //Start the RTC

  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH); //Disable SD Card
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  delay(1000);
  
  Serial.println("Starting Ethernet"); //configure Ethernet
    if(Ethernet.begin(mac) == 0) {
      Serial.println("Obtaining Address From DHCP Server");
      while(true); //keep in loop if needed
            }
    Serial.println("Ethernet Configured");
    delay(1000);
    Serial.println("The IP Address of this Microcontroller is: ");
    IPAddress myIPAddress = Ethernet.localIP();
    Serial.println(myIPAddress);
    Udp.begin(localPort);
     //get the NTP timestamp
    epoch = getNTP();
    Serial.println(epoch);
    //Set the RTC on startup
    RTC.adjust(epoch);
    Serial.println("RTC Adjusted");
  //check to see if the RTC is running, display fault message
  if( !RTC.isrunning() ) {
    
    //display what we did!
    Serial.println("RTC Fault, check wiring.");
    
  } else {
    //Show unix epoch
    Serial.print("Unix Epoch: ");
    Serial.println(epoch);    
    //show UTC
    Serial.print("UTC: ");
    showUTC(epoch);
  }
}

void loop() {
  int chk = DHT11.read(DHT11PIN); //Fill chk with any error codes from the dht read (library Function)
  delay(500);  
  switch (chk) { //Do some things like print to the serial monitor if there is an error.
    case DHTLIB_OK: // Atta Boy!
                break;
    case DHTLIB_ERROR_CHECKSUM:
                Serial.println("Checksum error"); //The checksum is bad, let the user know
                break;
    case DHTLIB_ERROR_TIMEOUT:
                Serial.println("Time out error"); //Is this thing connected?
                break;
    default:
                Serial.println("Unknown error"); //Yeah something broke, don't ask me what it was though
                break;
    }
    
  //repeatedly show the current date and time, taken from the RTC
  DateTime now = RTC.now();
  DHTRead();
  currentTemp = DegF;
  temperatureSPF = 77;
  temperatureDBF = 2;
  Serial.println(now.unixtime());
  Serial.print(" - ");
  Serial.print(currentTemp);
  Serial.print(" - ");
  //Serial.print(now.year(), DEC);
  //Serial.print('/');
  //Serial.print(now.month(), DEC);
  //Serial.print('/');
  //Serial.print(now.day(), DEC);
  //Serial.print(" - ");
  //showUTC(now.unixtime());
  //Serial.print(ESTHour());
  //Serial.print(":");
  //Serial.print(now.minute());
  //Serial.print(":");
  //Serial.println(now.second());
  //RTC.adjust(epoch);
  pinMode(A0, OUTPUT);
  if (Winter(override) == true){
    Serial.println("wintertime");
  digitalWrite(A0, TempControl(currentTemp, temperatureSPF, temperatureDBF));
  currentstate = digitalRead(A0);
  if (currentstate != laststate){
    if (laststate == HIGH) {
     //Put logic in here to datalog a heat off state 
    }
    if (laststate == LOW) {
     //Put logic in here to datalog a heat on state 
    }
    laststate = currentstate;
  }
  }
  
}

void showUTC(unsigned long epoch) {
  Serial.print((epoch % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if ( ((epoch % 3600) / 60) < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ( (epoch % 60) < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.println(epoch %60); // print the second
}

unsigned long getNTP() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if ( Udp.parsePacket() ) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    
    return epoch;
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0; // Stratum, or type of clock
  packetBuffer[2] = 6; // Polling Interval
  packetBuffer[3] = 0xEC; // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}
int ESTHour (){
  DateTime now = RTC.now();
  int x = now.hour() - 5;
  return x;
}
/* This is the heart of the heating system, There is nothing here to handle cooling, at the time of
conception heat was the only option available to me, Expect more functionality in the future.
*/
byte TempControl(int x, int y, int z){
// x is ambient temperature
// y is setpoint
// z is dead band
int dbhi = y + z; //create integer dead band for the heat Ceiling
int dblo = y - z; //create integer dead band for the heat Floor
byte enableSP = false; //build an enable byte
byte fallingEdge = false; //rising edge
byte fire = false; //build a turn on byte
byte lastState; //last state of the fire command
byte risingEdge; //falling edge
byte enableDB; //Dead band enable
if (x < y){ //allow the enable byte to go high
 enableSP = true;
} else { //once the temperature rises to the high dead band reset everything (turn off heat)
  enableSP = false;
  fire = false;
  fallingEdge = false;
}
if (x < dblo && lastState == false){ //we dropped below the dead band and are not rising in temp
  fallingEdge = true;
}
if (x > dblo && lastState == true){ //we rise above the low deadband and we are giving it juice
  risingEdge = true;
}
if (fallingEdge == true || risingEdge == true){
  enableDB = true;
}
if (enableDB == true && enableSP == true){
  fire = true;
}
lastState = fire;
return fire;
}
/* The automatic function of winter time can be overridden. On warm winter days a touch of this
button will disable all heating function and on cold summer days this button will enable the heat
The override feature will be self resetting most likely with time. 
*/
byte Winter(byte override){
  DateTime now = RTC.now();
  byte monthenable = false;
  if (now.month() >= 11 || now.month() <= 5 ) {
    monthenable = true;
  } else {
    monthenable = false;
  }
  if (!override && monthenable == true) return true;
  if (override == true && monthenable == false) return true;
  else {
    return false;
  }
}


//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius) {
        return 1.8 * celsius + 32;
}

//Celsius to Kelvin conversion
double Kelvin(double celsius) {
        return celsius + 273.15;
}

// delta max = 0.6544 wrt dewPoint()
// 5x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity) {
        double a = 17.271;
        double b = 237.7;
        double temp = (a * celsius) / (b + celsius) + log(humidity/100);
        double Td = (b * temp) / (a - temp);
        return Td;
}
double dewPointFastF(double celsius, double humidity) {
        double a = 17.271;
        double b = 237.7;
        double temp = (a * celsius) / (b + celsius) + log(humidity/100);
        double Tf = (b * temp) / (a - temp);
        return 1.8 * Tf + 32;
}
void DHTRead() {
  Serial.print("Humidity (%): ");
  humidity = DHT11.humidity;
  Serial.println(humidity, 2);
  
  
  Serial.print("Temperature (oC): ");
  temperature = DHT11.temperature;
  Serial.println(temperature, 2);
  
  
  Serial.print("Temperature (oF): ");
  DegF = (Fahrenheit(DHT11.temperature));
  Serial.println(DegF, 2);

  Serial.print("Temperature (K): ");
  kelvin = (Kelvin(DHT11.temperature));
  Serial.println(kelvin , 2);

  Serial.print("Dew Point (oC): ");
  DewC = (dewPointFast(DHT11.temperature, DHT11.humidity));
  Serial.println(DewC , 2);

  Serial.print("Dew Point (oF): ");
  DewF = (dewPointFastF(DHT11.temperature, DHT11.humidity));
  Serial.println(DewF , 2);
return;
}  

