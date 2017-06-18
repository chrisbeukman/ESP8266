/*********************************************************************
 *  AlarmMonDef.h
 *  
 *  Alarm Nonitor Definitions
 *********************************************************************/

const char* ssidDef_ = "WLN108";
const char* passwordDef_ = "@rgeNtu7e";
IPAddress staticServerIPAdrDef_(192,168,192,83);
IPAddress staticGatewayDef_(192,168,192,1);
IPAddress staticSubnetDef_(255,255,255,0);

const int PORT_NO = 80;
const int BAUD_RATE = 115200;
const String Sensor1_ON  = "almsens1/1";
const String Sensor1_OFF = "almsens1/0";

struct config_struct
{
  const char* ssid;     
  const char* password; 
  IPAddress staticServerIPAdr;
  IPAddress staticGateway;
  IPAddress staticSubnet;
};

/*
union config_union
{
  config_struct structData;
  char unionData [100];
}
*/