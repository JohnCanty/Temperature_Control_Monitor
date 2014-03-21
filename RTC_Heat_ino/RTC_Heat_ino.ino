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
#include <LiquidTWI.h>



//RTC Encapsulation
RTC_DS1307 RTC;
dht11 DHT11;
#define DHT11PIN 2
#define textBuffSize 9 //length of longest command string plus two spaces for CR + LF
LiquidTWI lcd(0);
byte mac[] = { 0xDE, 0xAD, 0xFE, 0xED, 0xDE, 0xAD };//MAC Address for Ethernet Shield
unsigned int localPort = 8888; //Local port to listen for UDP Packets
IPAddress timeServer(192, 168, 10, 2);//NTP Server IP 
const int NTP_PACKET_SIZE= 48; //NTP Time stamp is in the firth 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //Buffer to hold incomming and outgoing packets
EthernetUDP Udp; //UDP Instance to let us send and recieve packets
unsigned long epoch; //Unix Epoch time (NTP or RTC depending on state)
char textBuff[textBuffSize]; //someplace to put received text
int charsReceived = 0;
boolean connectFlag = 0; //we'll use a flag separate from client.connected
                         //so we can recognize when a new connection has been created
unsigned long timeOfLastActivity; //time in milliseconds of last activity
unsigned long allowedConnectTime = 300;//000; //five minutes 300 sec 5 minutes 300K miliseconds 5 minutes
EthernetServer server(23); // Telnet listens on port 23
EthernetClient client = 0; // Client needs to have global scope so it can be called
                   // from functions outside of loop, but we don't know
                   // what client is yet, so creating an empty object
//float temperatureDegF; //Ambient Temperature in degrees F
int currentTemp; //Current temp in Integer form
int temperatureSPF; //Temperature set point in Degrees F
int temperatureDBF; //Temperature dead band in Degrees F
//byte winterTime = true; // Is it winter time
byte override = false; //override button, will turn on and off heat forcibly
byte laststate;
byte currentstate;
byte DST;
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
  temperatureSPF = 72;
  temperatureDBF = 2;
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
  // look to see if a new connection is created,
  // print welcome message, set connected flag
  if (server.available() && !connectFlag) {
    connectFlag = 1;
    client = server.available();
    client.println("Temperature Server");
    client.println("? for help");
    printPrompt();
  }
  
  // check to see if text received
  if (client.connected() && client.available()) getReceivedText();
    
  // check to see if connection has timed out
  if(connectFlag) checkConnectionTimeout();
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
  int x;
  if (DST == true) x = now.hour() - 4;
  else x = now.hour() - 5;
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
  risingEdge = false;
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
void printPrompt()
{
  timeOfLastActivity = epoch; //millis();
  client.flush();
  charsReceived = 0; //count of characters received
  client.print("\n>");
}


void checkConnectionTimeout()
{
  if(epoch - timeOfLastActivity > allowedConnectTime) { //epoch was millis()
    client.println();
    client.println("Timeout disconnect.");
    client.stop();
    connectFlag = 0;
  }
}


void getReceivedText()
{
  char c;
  int charsWaiting;

  // copy waiting characters into textBuff
  //until textBuff full, CR received, or no more characters
  charsWaiting = client.available();
  do {
    c = client.read();
    textBuff[charsReceived] = c;
    charsReceived++;
    charsWaiting--;
  }
  while(charsReceived <= textBuffSize && c != 0x0d && charsWaiting > 0);
  
  //if CR found go look at received text and execute command
  if(c == 0x0d) {
    parseReceivedText();
    // after completing command, print a new prompt
    printPrompt();
  }
  
  // if textBuff full without reaching a CR, print an error message
  if(charsReceived >= textBuffSize) {
    client.println();
    printErrorMessage();
    printPrompt();
  }
  // if textBuff not full and no CR, do nothing else;  
  // go back to loop until more characters are received
  
}  


void parseReceivedText()
{
  // look at first character and decide what to do
  switch (textBuff[0]) {
    case 'b' : dbCommand();              break;
    case 't' : tempCommand();            break;
    case 'd' : dstCommand();             break;
    case 's' : estCommand();             break;
    case 'c' : checkCloseConnection();   break;
    case '?' : printHelpMessage();       break;
    case 0x0d :                          break;  //ignore a carriage return
    default: printErrorMessage();        break;
  }
 }


void dbCommand()
  // if we got here, textBuff[0] = 'b'
{
  temperatureDBF = setpointAdjust();
  client.println(temperatureDBF);
}
void dstCommand()
  // if we got here, textBuff[0] = 'd' 
{
  DST = true;
}  
void estCommand()
  // if we got here, textBuff[0] = 's' 
{
  DST = false;
}  
void tempCommand()
  // if we got here, textBuff[0] = 't'
{
  temperatureSPF = setpointAdjust();
  client.println(temperatureSPF);
}

int setpointAdjust()
{
  int tempSetting = 0;
  int textPosition = 1;  //start at textBuff[1]
  int digit;
  do {
    digit = parseDigit(textBuff[textPosition]); //look for a digit in textBuff
    if (digit >= 0 && digit <=9) {              //if digit found
      tempSetting = tempSetting * 10 + digit;     //shift previous result and add new digit
    }
    else tempSetting = -1;
    textPosition++;                             //go to the next position in textBuff
  }
  //if not at end of textBuff and not found a CR and not had an error, keep going
  while(textPosition < 3 && textBuff[textPosition] != 0x0d && tempSetting > -1);
   //if value is not followed by a CR, return an error
  if(textBuff[textPosition] != 0x0d) tempSetting = -1;  
  return tempSetting;
}

void printErrorMessage()
{
  client.println("Unrecognized command.  ? for help.");
}


void checkCloseConnection()
  // if we got here, textBuff[0] = 'c', check the next two
  // characters to make sure the command is valid
{
  if (textBuff[1] == 'l' && textBuff[2] == 0x0d)
    closeConnection();
  else
    printErrorMessage();
}


void closeConnection()
{
  client.println("\nBye.\n");
  client.stop();
  connectFlag = 0;
}


void printHelpMessage()
{
  DateTime now = RTC.now();
  client.println("\nExamples of supported commands:\n");
  client.println("  b       - DeadBand Adjust - Modify the deadband, affects High and Low.");
  client.println("  d       - DST - Turn on Daylight Savings Time.");
  client.println("  s       - EST - Turn off Daylight Savings Time.");
  client.println("  t       - Temp Adjust - Make adjustment to the tepmerature.");
  client.println("  cl      - Close Connection - End this misery");
  client.println("  ?       - Hacker Friendly Help print this help message");
  client.print("Date - ");
  client.print(now.year()); //put time in here
  client.print(now.month()); //put time in here
  client.println(now.day()); //put time in here
  client.print("Time - ");
  client.print(ESTHour()); //put time in here
  client.print(":");
  client.print(now.minute()); //put time in here
  client.print(":");
  client.println(now.second()); //put time in here
  client.print("Temperature - ");
  client.println(DegF);
  client.print("Setpoint - ");
  client.println(temperatureSPF);
  client.println("Ok, Go!");
}
int parseDigit(char c)
{
  int digit = -1;
  digit = (int) c - 0x30; // subtracting 0x30 from ASCII code gives value
  if(digit < 0 || digit > 9) digit = -1;
  return digit;
}


