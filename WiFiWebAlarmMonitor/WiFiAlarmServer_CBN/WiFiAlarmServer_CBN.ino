/*********************************************************************
 *  WebServer that monitors Alarm Client connection status and 
 *  AlarmT condition. Will activate Alarm according to the client 
 *  alarm condition.
 *  
 *  The server will set a Alarm condition depending on the request:
 *    http://server_ip/almsens#/0 will set the GPIO2 low,
 *    http://server_ip/almsens#/1 will set the GPIO2 high
 *      server_ip = IP address of the ESP8266 module.
 *      # = Sensor client number. 
 *  
 *  Info will be printed to Serial when the module is connected.
 *
 * The client module WiFiAlarmMonClient_CBN.ino should connect to 
 * this server module via a WiFi connection
 *********************************************************************/

#define __DEBUG

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "\Users\za010047\Documents\Arduino\WiFiWebAlarmMonitor_CBN\AlarmMonServer.h"

// Constants
const int SENSOR_TIMEOUT = 1500;  // 10x ms
const int CONNECTION_STATUS_PIN = 1;
const int ALARM_PIN = 2;

config_struct configData_; //Variable to store config data in EEPROM.
int connectionTimer_ = 0;
int alarmSensor1_ = -1;

// Create an instance of the Server
// specify the port to listen on as an argument
WiFiServer server(PORT_NO);


////////////////////
// Initialisation //
////////////////////
void setup() 
{
  // Initialise Serial port
  Serial.begin(BAUD_RATE);
  delay(1000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(" ");
  Serial.println("\n\r");

  // prepare GPIO's
 #ifndef __DEBUG
   // This pin can only be used when NOT Debuging with via USB
   pinMode(CONNECTION_STATUS_PIN, OUTPUT);
   digitalWrite(CONNECTION_STATUS_PIN, HIGH);
 #endif
 
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);
  
  // Connect to WiFi network
  Serial.println("<< Web Alarm Monitoring Server >>");

  // Read Configuration from EEPROM
  storeConfigData();
//  readConfigData();

  // Set Server Static IP Address
  WiFi.mode(WIFI_STA);
  WiFi.config(configData_.staticServerIPAdr, configData_.staticGateway, configData_.staticSubnet); 

  // Start Server connection
  WiFi.begin(configData_.ssid, configData_.password);
  WiFi.setAutoReconnect(true);
  printWifiStatus();
  connectToRouter();
  
  // Start the server
  server.begin();
  Serial.println("Alarm Monitor Server started.");

  // Print the IP address
  printWifiStatus();  
}

/////////////////////////
// Main Loop Execution //
/////////////////////////
void loop() 
{
  if (WiFi.status() != WL_CONNECTED) 
  {
    // Reconnect to router
    connectToRouter();
  }

  // Check if a client has connected
  WiFiClient client = server.available();
  delay(10);
  if (!client) 
  {
    connectionTimer_++;  // Update sensor timeout
    if (connectionTimer_ > SENSOR_TIMEOUT)
    {
      alarmSensor1_ = -1;  // Set sensor status to unknown
      Serial.println("\n** Sensor 1 Connection Timeout **");
      
      // Set Sensor Connection Timeout condition
      setSensorTimeout(1);
      printWifiStatus();
    }
    return;
  }
  
  // Wait until the client sends some data
  Serial.println();
  Serial.print("Client connecting.");
  while (!client.available())
  {
    Serial.print(".");
    delay(10);
  }
  
  // Read the first line of the request
  String reqMsg = client.readStringUntil('\r');
  Serial.println(reqMsg);
  client.flush();

  String responseMsg;
  responseMsg += "HTTP/1.1 200 OK\r\nContent-Type: text/html\nConnection: close\nRefresh: 10\r\n\r\n<!DOCTYPE HTML>";
  responseMsg += "<title>Web Alarm Mon</title>";
  responseMsg += "<head>\nWeb Alarm Monitor Server - Up Time: ";
  responseMsg += getElaspedTime();
  responseMsg += "<br />\n\r</head>";
  client.print(responseMsg);

  // Match the Command
  if      (reqMsg.indexOf(Sensor1_OFF) != -1) alarmSensor1_ = setAlarmSensorCondition(1, LOW);
  else if (reqMsg.indexOf(Sensor1_ON) != -1)  alarmSensor1_ = setAlarmSensorCondition(1, HIGH);

  // Match the Request
  else if (reqMsg.indexOf("") != -1)
  {
    // Provide currrent status of the Alarm Sensor
    responseMsg = prepareMessage(1, alarmSensor1_);
    responseMsg += " Connected.";
    client.print(responseMsg);
    client.stop();
    Serial.println("Sensor status request");
    return;  // Abort
  }
  client.flush();

  // Prepare the response
  responseMsg = prepareMessage(1, alarmSensor1_);
  
  // Send the response to the client
  client.print(responseMsg);
}

////////////////////////////////////////////
// Prepare status message
String prepareMessage(int sensorNo, int sensorStatus)
{
  String msg;
  msg = "<html>\r\nSensor ";
  msg += sensorNo;

  if (sensorStatus != -1)
  {
    msg += " Alarm ";
    msg += (sensorStatus)?"ON":"OFF";
  }
  else
  {
    msg += " - NOT ";
  }
  msg += "</html>\n";
  return msg;
}

////////////////////////////////////////////
// Set Sensor connection timeout condition
void setSensorTimeout(int sensorNo)
{
  connectionTimer_ = 0;  // Reset Timeout
  digitalWrite(ALARM_PIN, HIGH);
  delay(200);
  digitalWrite(ALARM_PIN, LOW);  
//  digitalWrite(CONNECTION_STATUS_PIN, HIGH);
}

////////////////////////////////////////////
// Reset Sensor connection timeout condition
void resetSensorTimeout(int sensorNo)
{
  connectionTimer_ = 0;  // Reset Timeout
  digitalWrite(ALARM_PIN, LOW);  
//  digitalWrite(CONNECTION_STATUS_PIN, LOW);  
}

////////////////////////////////////////////
// Set Status of Alarm Sensor
int setAlarmSensorCondition(int sensorNo, int alarmStatus)
{
  // Set GPIO for Sensor Alarm request
  digitalWrite(ALARM_PIN, alarmStatus);
  
  // Indicated that Sensor is connected
  resetSensorTimeout(sensorNo);
  return alarmStatus;
}

////////////////////////////////////////////
// Print WiFi info on serial
void printWifiStatus() 
{
  Serial.println();
  Serial.print("Alarm Server Time: "); Serial.println(getElaspedTime());
  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: "); Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: "); Serial.println(ip);
  Serial.println(configData_.staticGateway);

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
    setSensorTimeout(1); // Set Sensor Connection Timeout condition by default
    delay(1000);
  }
  Serial.println();
  Serial.println("WiFi Router connected.");
}

///////////////////////////////////////////////
// Read Configuration setup data from EEPROM
void readConfigData()
{
  int eeAddress = 0;       //EEPROM address to start reading from

  Serial.print("Read from EEPROM: ");

  //Get configuration data from the EEPROM at position 'eeAddress'
  EEPROM.get(eeAddress, configData_);
}

///////////////////////////////////////////////
// Store Configuration setup data on EEPROM
void storeConfigData()
{
  int eeAddress = 0;       //EEPROM address to start writing to

  configData_.ssid = ssidDef_;
  configData_.password = passwordDef_;
  configData_.staticServerIPAdr = staticServerIPAdrDef_;
  configData_.staticGateway = staticGatewayDef_;
  configData_.staticSubnet  = staticSubnetDef_;

//  EEPROM.update(eeAddress, configData_[0]);
  delay(50);
}

