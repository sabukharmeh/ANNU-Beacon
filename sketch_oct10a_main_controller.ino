#define ITERATIONS 32

#define height 30


#define fast 85
#define turn 110
#define slow 0


// Black Line Follower 
#define IR1 D2    //Right sensor
#define IR2 D3    //left Sensor

#define enA D0    //Right motor
#define enB D1    //Left motor

#define TX  D4    // start other heights
#define RX1 D5    // feedback1
#define RX2 D6    // feedback2


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
void stop(){
  analogWrite (enA, 0);
  analogWrite (enB, 0);
}
void forward(){
  analogWrite (enA, fast);
  analogWrite (enB, fast);
}
void left(){
  //Tilt robot towards left by stopping the left wheel and moving the right one
  analogWrite (enA, turn);
  analogWrite (enB, slow);
}
void right(){
  //Tilt robot towards right by stopping the right wheel and moving the left one
  analogWrite (enA, slow);
  analogWrite (enB, turn);
}
void setup() {
  
  Serial.begin(115200);

  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(IR1,INPUT);
  pinMode(IR2,INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(TX,  OUTPUT);
  pinMode(RX1, INPUT);
  pinMode(RX2, INPUT);

  digitalWrite(TX, LOW);

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

}

bool led = false;

void scan(){

  HTTPClient http;    // http object of clas HTTPClient

  if(debug)
    Serial.println("Scanning...");

  digitalWrite(TX, HIGH);
  
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

    digitalWrite(TX, LOW);
    
  }// scan #
  stop_number++;
}

bool out_of_reset = true;
void loop() 
{
  
  if(digitalRead(IR1)==HIGH && digitalRead(IR2)==HIGH && (lastScan > 100 /*about 0.2 seconds*/ || out_of_reset == true)){

    stop();    
    lastScan = 0;
    Serial.println("stop for scanning...");
    scan();
    
    digitalWrite(TX, LOW);
    
    while (digitalRead(RX1) == HIGH || digitalRead(RX2) == HIGH) //wait until all other MCUs are done
      delay(10);

    out_of_reset = false;
    
    forward();
  }
  else if(digitalRead(IR1)==LOW && digitalRead(IR2)==HIGH){
    left();
  }
  else if(digitalRead(IR1)==HIGH && digitalRead(IR2)==LOW){
    right();
  }
  else {
    forward();
  }
  
  lastScan++;
  
  delay(1);
  
  if(lastScan%10==0){
    sprintf(buf, "IR1 %d, IR2 %d, lastScan %d, out_of_reset %d", digitalRead(IR1), digitalRead(IR2), lastScan, out_of_reset);
    Serial.println(buf);
    digitalWrite(LED_BUILTIN, led);
  }
}