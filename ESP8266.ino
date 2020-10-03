
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include "pass.h"

// link to get current external IP
String link = "http://api.ipify.org";

// WiFi configuration
IPAddress ip(192,168,0,129);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress dns1(8,8,8,8);
IPAddress dns2(8,8,4,4);

int httpCodeExternalIp;
int httpCodeDynamicDns;

const String washmashineName = "iwsd51252";

String pauseString = washmashineName + "_pause";
String startString = washmashineName + "_start";
String powerOnString = washmashineName + "_power_on";
String powerOffString = washmashineName + "_power_off";

unsigned long previousMillis = 0;
const long interval = 10*60*1000;           // interval 10 minutes
unsigned long currentMillis = 0;
 
String oldIp = "old";
String newIp = "new";
String inData;
char thisChar;
String inputString;         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete
char inChar;


// Start a TCP Server on port 5045
WiFiServer server(5045);
HTTPClient externalIpClient;
HTTPClient dynamicDnsClient;
WiFiClient client;


void setup() 
{
  Serial.begin(9600);
  Serial.begin(9600);

  Serial.println("Configuration...");
  inputString.reserve(200);

  WiFi.config(ip, gateway, subnet,dns1, dns2);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  Serial.println("");
  
  //Wait for connection
  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to "); Serial.println(SSID_PASSWORD);
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  Serial.print("Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
 
  // Start the TCP server
  server.begin();

  // initialize_led_status
  if( WiFi.status() == WL_CONNECTED)
  {
    GetExternalIP();
  }
}

void loop() 
{
  if (WiFi.status() == WL_CONNECTED);
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      // save the last time
      previousMillis = currentMillis;
      GetExternalIP();
    }
  } 
  
  if (!client.connected()) 
  {
    // try to connect to a new client
    client = server.available();
    if (client)
    {
      if (client.connected())
      {
        Serial.println("Connected to client");
        delay(3000);
      }
    }
  } 
  else {
    if (Serial.available()) 
    {
      // get the new byte:
      inChar = (char)Serial.read();
      if (inChar == '\n') 
      {
        stringComplete = true;
      }
    
      else
      {
        if ((' ' <= inChar) && (inChar <= '~'))
        {
          inputString += inChar;
        }
      }
    }
    
    if (stringComplete) 
    {
      //Serial.println(inputString);
      client.write(const_cast<char*>(inputString.c_str()));

      // clear the string:
      inputString = "";
      stringComplete = false;
    }
    
    // read data from the connected client
    if (client.available() > 0) 
    {
      thisChar = client.read();
      inData += thisChar;
    }
    else
    {
      if(inData != "")
      {
        Serial.print("\n");
      }
      if (inData == startString)
      {
        Serial.println("command_start");
      }
      else if (inData == powerOnString)
      {
        Serial.println("command_powerOn");
      }    
      else if (inData == powerOffString)
      {
        Serial.println("command_powerOff");
      }
      else if (inData == pauseString)
      {
        Serial.println("command_pause");
      }
      else
      {}
         
      inData = "";
    }
  }
}
    
void GetExternalIP()
{
  Serial.print("[HTTP] begin...\n");
  if (externalIpClient.begin(link))
  {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    httpCodeExternalIp = externalIpClient.GET();
    
    // httpCode will be negative on error
    if (httpCodeExternalIp > 0) 
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCodeExternalIp);
      if (httpCodeExternalIp == HTTP_CODE_OK || httpCodeExternalIp == HTTP_CODE_MOVED_PERMANENTLY)
      {
        newIp = externalIpClient.getString();
        Serial.println(newIp);
        
        if (oldIp != newIp )
          {
            UpdateDynamicDnsIp(newIp);
            Serial.println("New IP available!");
          } 
        }
        
      } 
      else 
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", externalIpClient.errorToString(httpCodeExternalIp).c_str());
      }

      externalIpClient.end();
    } 
    else 
    {
      Serial.printf("[HTTP} Unable to connect\n");
    }  
}

void UpdateDynamicDnsIp(String ip)
{
  Serial.println("start update ip");
  dynamicDnsClient.begin("http://www.duckdns.org/update?domains=" + DDNS_DOMAIN + "&token=" + DDNS_TOKEN + "&ip=" + ip + "");
  httpCodeDynamicDns = dynamicDnsClient.GET();
  Serial.println("http code url update = " + httpCodeDynamicDns);
  
  if(httpCodeDynamicDns > 0) 
  {
    oldIp = ip;
    Serial.println("success update ip to: " + ip);
  }
  
  dynamicDnsClient.end();
}


void serialEvents() 
{
  if (Serial.available()) 
  {
    // get the new byte:
    inChar = (char)Serial.read();
    // add it to the inputString:
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    Serial.println(inChar);

    if (inChar == '\n') 
    {
      stringComplete = true;
    }
    else
    {
      if ((' ' <= inChar) && (inChar <= '~'))
      {
        inputString += inChar;
      }
    }
  }
}
