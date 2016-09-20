
//-------- Required Libraries --------//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <TimeLib.h> // TimeTracking
#include <WiFiUdp.h> // UDP packet handling for NTP request

//-------- ntp setup --------//
//NTP Servers:
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const char* ntpServerName = "time.nist.gov";

const int timeZone = -6;  // Eastern central Time (USA)

WiFiUDP Udp;
unsigned int localPort = 2390;  // local port to listen for UDP packets

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
bool NTP;
//---------- timestamp functions-------//
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println(F("Transmit NTP Request"));
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println(F("Receive NTP Response"));
      NTP = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println(F("No NTP Response :-("));
  return 0; // return 0 if unable to get the time
}

void udpConnect() {
  Serial.println(F("Starting UDP"));
  Udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(Udp.localPort());
  Serial.println(F("waiting for sync"));
  setSyncProvider(getNtpTime);
}

time_t prevDisplay = 0; // when the digital clock was displayed


void checkTime () {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
    }
  }
}




//--------  anager function. Configure the wifi connection if not connect put in mode AP--------//
void wifimanager() {

  WiFiManager wifiManager;
  Serial.println(F("empezando"));
  if (!  wifiManager.autoConnect("Espmanager")) {
    Serial.println(F("error no conecto"));

    if (!wifiManager.startConfigPortal("Espmanager")) {
      Serial.println(F("failed to connect and hit timeout"));

      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    Serial.print(F("."));
  }
  //if you get here you have connected to the WiFi

  Serial.println(F("connected...yeey :)"));

}
//-------- setup  --------//

void setup() {
  Serial.begin(115200);
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    wifimanager();
    delay(1000);
  }

  while (NTP == false) {
    udpConnect ();
    delay(500);
  }
}



void loop() {
Serial.println(hour());            // the hour now  (0-23)
Serial.println(minute());          // the minute now (0-59)          
Serial.println(second());          // the second now (0-59) 
Serial.println(day());             // the day now (1-31)
Serial.println(weekday());         // day of the week, Sunday is day 0 
Serial.println(month());           // the month now (1-12)
Serial.println(year()); 
delay(10000);                      // 10 seconds delay
}

