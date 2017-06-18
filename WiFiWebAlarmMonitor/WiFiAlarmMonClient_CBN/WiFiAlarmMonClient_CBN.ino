/*********************************************************************
 *  WiFi Client that monitors Alarm Server connection status and 
 *  Alarm pin condition.
 *  
 *  The server will set a Alarm condition depending on the request of
 *  the client:
 *    http://server_ip/almsens#/0 will set the GPIO2 low,
 *    http://server_ip/almsens#/1 will set the GPIO2 high
 *      server_ip = IP address of the ESP8266 module.
 *      # = Sensor client number. 
 *  
 *  Info will be printed to Serial when the module is connected.
 *
 * This module connects to the WiFiAlarmServer_CBN.ino server 
 * module via a WiFi connection
 ********************************************************************/

#define __DEGUG

#include <ESP8266WiFi.h>
#include "\Users\za010047\Documents\Arduino\WiFiWebAlarmMonitor_CBN\AlarmMonClient.h"

const int SENSOR_MON_TIMEOUT = 100;  // ms
const int ALARM_PIN = 2;

config_struct configData_; //Variable to store config data in EEPROM.

////////////////////
// Initialisation //
////////////////////
void setup() 
{
  Serial.begin(BAUD_RATE);
  delay(1000);
 
  // prepare GPIO
  pinMode(ALARM_PIN, INPUT);
  Serial.println();
  Serial.println("\n\r");
  Serial.println("<< Start WiFi Client Sensor Connection >>");

  // Read Configuration from EEPROM
  storeConfigData();
//  readConfigData();
  
//  WiFi.mode(WIFI_STA);
//  WiFi.config(configData_.staticClientIPAdr, configData_.staticGateway, configData_.staticSubnet); 
//  WiFi.begin(configData_.ssid, configData_.password);
//  WiFi.setAutoReconnect(true);

  // We start by connecting to a WiFi network
  printWifiStatus();
  connectToRouter();
  printWifiStatus();
}

/////////////////////////
// Main Loop Execution //
/////////////////////////
int alarmStatusCurrent = LOW;

void loop() 
{
  // Monitor Alarm IO input
  for (int i=0; i < 70; ++i)
  {
    delay(SENSOR_MON_TIMEOUT);
    int alarmStatus = digitalRead(ALARM_PIN);
    if (alarmStatus != alarmStatusCurrent)
    {
      alarmStatus = digitalRead(ALARM_PIN);
      delay(100);
      if (alarmStatus != alarmStatusCurrent)
      {
        // Report Alarm Condition
        alarmStatusCurrent = alarmStatus;
        break;
      }
    }
  }

  Serial.println();
  Serial.print("Connecting to Server ");
  Serial.println(configData_.staticServerIPAdr);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(configData_.staticServerIPAdr, PORT_NO)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URL for the request
  String urlCommand;
  if (alarmStatusCurrent == HIGH)
  {
    urlCommand = Sensor1_ON;
  }
  else
  {
    urlCommand = Sensor1_OFF;
  }
  Serial.print("Sending URL Command: ");
  Serial.println(urlCommand);
  
  // This will send the request to the server
  client.print(String("GET ") + urlCommand + " HTTP/1.1\r\n" +
               "Host: " + configData_.staticServerIPAdr + "\r\n" + 
               "Connection: close\r\n\r\n");
               
  unsigned long timeout = millis();
  while (client.available() == 0) 
  {
    if (millis() - timeout > 5000) 
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;  // Abort
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available())
  {
    String line = client.readStringUntil('\r');
  }
  Serial.println("closing connection.");
}

////////////////////////////////////////////
// Print WiFi info on serial
void printWifiStatus() 
{
  Serial.println();
  Serial.print("Alarm Cient Time: "); Serial.println(getElaspedTime());
  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: "); Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: "); Serial.println(ip);

  // print your WiFi shield's MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): "); Serial.print(rssi); Serial.println(" dBm");
}

////////////////////////////////////////////
// Perform Router connection
void connectToRouter()
{
  Serial.print("Connecting to "); Serial.print(configData_.ssid); Serial.print(":"); 
  long progressTimer = 0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    if (progressTimer-- <= 0)
    {
      // Show progress
      progressTimer = 30;
      Serial.println();
      Serial.print(getElaspedTime()); Serial.print(" ");
    }
    else
    {
      Serial.print(".");
    }
    delay(1000);
  }
  Serial.println();
  Serial.println("WiFi Router connected.");
}

////////////////////////////////////////////
// Get time elasped since power-up
String getElaspedTime()
{
  unsigned long time;
  float hour;
  float min;
  float sec;
  char  elaspedTime[20];
  
  time = millis();
  hour = (float)(time / 3600000.0);
  min  = (float)((hour - (unsigned int)(hour)) * 60.0);
  sec  = (float)((min - (unsigned int)(min)) * 60.0);
  
  sprintf(elaspedTime, "%d:%d:%d", (unsigned int)(hour), (unsigned int)(min), (unsigned int)(sec));
  return elaspedTime;
}

///////////////////////////////////////////////
// Store Configuration setup data on EEPROM
void storeConfigData()
{
  int eeAddress = 0;       //EEPROM address to start writing to

  configData_.ssid = ssidDef_;
  configData_.password = passwordDef_;
  configData_.staticServerIPAdr = staticServerIPAdrDef_;
  configData_.staticClientIPAdr = staticClientIPAdrDef_;
  configData_.staticGateway = staticGatewayDef_;
  configData_.staticSubnet  = staticSubnetDef_;

//  EEPROM.update(eeAddress, configData_[0]);
  delay(50);
}
