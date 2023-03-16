#define ITERATIONS 32

#define height 90


#define RX  D0    // start 
#define TX  D1    // feedback


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

bool transmit = true;
bool debug = true;

int stop_number = 1;

int lastScan = 0;

/* Set these to your desired credentials. */

//const char *ssid = "SSS"; //Enter your WIFI ssid
const char *ssid = "TP-Link_D302"; //Enter your WIFI ssid
const char *password = "0.123456789"; //Enter your WIFI password

WiFiClient wifiClient;

char buf[300];

void printMacAddress(byte mac[6]) {
  // print your MAC address:
  Serial.print(" \tmac: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[5],HEX);
}
String getMacAddress(byte mac[6]) {
  String macString = "";

  for(int i = 0; i < 6; i++){
    if(mac[i] < 0x10)
      macString +="0";
    macString += String(mac[i], HEX);
    if(i!=5)
      macString+=":";
  }
  macString.toUpperCase();
  return macString;
}

void setup() {
  
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(TX,  OUTPUT);
  pinMode(RX,  INPUT);

  Serial.print("Configuring access point...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Flash the LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

  //
  digitalWrite(TX, LOW);
}

bool led = false;

void scan(){

  HTTPClient http;    // http object of clas HTTPClient

  if(debug)
    Serial.println("Scanning...");

  digitalWrite(TX, HIGH);  // busy
  
  for(int scan = 0; scan < ITERATIONS; scan++) {

    // scan and send.    
    int n = WiFi.scanNetworks();

    if(transmit == true) {

      int item=0;
      String postData = "";
        
      for(int thisNet = 0; thisNet < n; thisNet++){

        byte bssid[6];
        String mySSID;
        byte *myBSSID;
        int myRSSI, myEncryption;
    
        // send info to database
        mySSID = WiFi.SSID(thisNet);
        myRSSI = WiFi.RSSI(thisNet);
        myEncryption = WiFi.encryptionType(thisNet);
        myBSSID = WiFi.BSSID(thisNet);
        
        // a=stop_number, b=scan_number, c=ssid_number, d=ssid_name, e=signal_strength, f=encryption, g=mac
        postData += "&a"+String(item)+"="+String(stop_number)+\
                    "&b"+String(item)+"="+String(scan)+\
                    "&c"+String(item)+"="+String(thisNet)+\
                    "&d"+String(item)+"="+mySSID+\
                    "&e"+String(item)+"="+String(myRSSI)+\
                    "&f"+String(item)+"="+String(myEncryption)+\
                    "&g"+String(item)+"="+getMacAddress(myBSSID)+\
                    "&h"+String(item)+"="+String(height);
        item++;

        // send info to serial.
        if(debug) {
          Serial.print("stop_number: ");
          Serial.print(stop_number);
          Serial.print("\tscan_number: ");
          Serial.print(scan);
          Serial.print(" \tssid_number: ");
          Serial.print(thisNet);
          Serial.print(" \tssid_name: ");
          Serial.print(mySSID);
          Serial.print(" \tsignal_strength: ");
          Serial.print(myRSSI);
          Serial.print(" \tencryption: ");
          Serial.print(myEncryption);
          Serial.print(" \BSSID: ");
          printMacAddress(myBSSID);
          Serial.print(" \theight: ");
          Serial.print(height);
          Serial.println("");
        }
      }// thisNet #

      int httpCode;
      do {
        if(debug){
          Serial.println(postData);
        }
        http.begin(wifiClient, "http://www.abukharmeh.com/ips_project/dbwrite.php");      // Connect to host where MySQL databse is hosted
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");              //Specify content-type header
        
        
        httpCode = http.POST(postData);   // Send POST request to php file and store server response code in variable named httpCode

        // if connection eatablished then do this
        if (httpCode == HTTP_CODE_OK) { 
          //Serial.println("Values uploaded successfully."); 
          //Serial.println(httpCode); 
          if(debug) {
            String webpage = http.getString();    // Get html webpage output and store it in a string
            Serial.println(" " + webpage); 
          }
        }
        // if failed to connect then return and restart
        else { 
          Serial.println(httpCode); 
          Serial.println("Failed to upload values. \n"); 
        }
        http.end();
        if(debug)
          Serial.printf("[HTTP] ... code: %d\n", httpCode);
        
      } while(httpCode != 200);

    }// transmit
    
    digitalWrite(LED_BUILTIN, led);
    led = !led;

    
  }// scan #
  stop_number++;

  digitalWrite(TX, LOW); // done
}

void loop() 
{
  
  if(digitalRead(RX)==HIGH){
    delay(100); // avoid glitch
    if(digitalRead(RX)==HIGH){
      Serial.println("scanning...");
      scan();
    }
  }
  
  delay(10);

}