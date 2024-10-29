#include <hidboot.h>
#include <usbhub.h>
#include <SPI.h>
#include <WiFiS3.h>
#include <MQTTClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiSSLClient.h> 


const byte numRows = 4; // Number of rows on the keypad
const byte numCols = 4; // Number of columns on the keypad

const char* server = "phycom-scannerrrr.onrender.com";

WiFiSSLClient client; // ใช้ WiFiSSLClient สำหรับ HTTPS

char keymap[numRows][numCols] = 
{
  {'1', '2', '3', 'A'}, 
  {'4', '5', '6', 'B'}, 
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[numRows] = {10, 8, 7, 6}; 
byte colPins[numCols] = {5, 4, 3, 2};

// Define a value for NO_KEY
#define NO_KEY '\0'
LiquidCrystal_I2C lcd(0x27, 16, 2);
char keys[14];
int keyIndex = 0;
String myString = "waiting..";
int my2Str = 0;
int id;
String name;
int amount;
int price;
int mode = 1;



//--------------------------------------------------------------------------------------------------------------------------------
//กำหนด เชื่อม wifi
//================================================================================================================================
const char WIFI_SSID[] = "vttm";          // CHANGE TO YOUR WIFI SSID
const char WIFI_PASSWORD[] = "tlelovepam";  // CHANGE TO YOUR WIFI PASSWORD
//================================================================================================================================

//--------------------------------------------------------------------------------------------------------------------------------
//เชื่อม mqtt
//================================================================================================================================
const char MQTT_BROKER_ADDRESS[] = "mqtt-dashboard.com";  // CHANGE TO MQTT BROKER'S ADDRESS
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "alksfjlkasf";  // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "";                // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "";                // CHANGE IT IF REQUIRED, empty if not required

// The MQTT topics that Arduino should publish/subscribe
const char PUBLISH_TOPIC[] = "phycom/66070108";    // CHANGE IT AS YOU DESIRE
const char SUBSCRIBE_TOPIC[] = "phycom/66070108";  // CHANGE IT AS YOU DESIRE
const int PUBLISH_INTERVAL = 60 * 1000;  // 60 seconds
WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

unsigned long lastPublishTime = 0;

//================================================================================================================================
// รวม method mqtt
//================================================================================================================================

void connectToMQTT() {
  mqtt.begin(MQTT_BROKER_ADDRESS, MQTT_PORT, network);
  mqtt.onMessage(messageHandler);

  Serial.print("Arduino UNO R4 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("Arduino UNO R4 - MQTT broker Timeout!");
    return;
  }

  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("Arduino UNO R4 - Subscribed to the topic: ");
  else
    Serial.print("Arduino UNO R4 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("Arduino UNO R4 - MQTT broker Connected!");
}

//---------------------------------------------------------------------------------------------

void sendToMQTT() {
  String val_str = String(keys);
  char messageBuffer[14];
  val_str.toCharArray(messageBuffer, 14);
  mqtt.publish(PUBLISH_TOPIC, messageBuffer);
  Serial.println("Arduino UNO R4 - sent to MQTT: topic: " + String(PUBLISH_TOPIC) + " | payload: " + String(messageBuffer));
}

void messageHandler(String &topic, String &payload) {
  Serial.println("Arduino UNO R4 - received from MQTT: topic: " + topic + " | payload: " + payload);
  my2Str += 1;
  parseMQTTMessage(payload);
  lcd.clear();
}

//================================================================================================================================
// split payload
void parseMQTTMessage(String message) {
  int firstSpace = message.indexOf(' ');
  int secondSpace = message.indexOf(' ', firstSpace + 1);
  int thirdSpace = message.indexOf(' ', secondSpace + 1);
  
  id = message.substring(0, firstSpace).toInt();
  myString = message.substring(firstSpace + 1, secondSpace);
  amount = message.substring(secondSpace + 1, thirdSpace).toInt();
  price = message.substring(thirdSpace + 1).toInt(); 
}

//--------------------------------------------------------------------------------------------------------------------------------
//เชื่อม scanner และทำงาน
//================================================================================================================================
USB Usb;
USBHub Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> Keyboard(&Usb);

class KbdRptParser : public KeyboardReportParser {
  protected:
    void OnKeyDown(uint8_t mod, uint8_t key);
    void OnKeyPressed(uint8_t key);
    void output();
};

KbdRptParser KbdPrs;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();


  if (Usb.Init() == -1) {
    Serial.println("USB Host Shield initialization failed");
    while (1); // หยุดการทำงานหากมีปัญหา
  }
  
  Serial.println("USB Host Shield initialized. Ready to scan.");
  Keyboard.SetReportParser(0, &KbdPrs);

  //เชื่อม wifi และ mqtt
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Arduino UNO R4 - Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(10000);
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());


  connectToMQTT();


  for (int i = 0; i < numRows; i++) {
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], HIGH); // Initially, set all rows HIGH
    }
    for (int i = 0; i < numCols; i++) {
        pinMode(colPins[i], INPUT_PULLUP); // Enable pull-up resistors on column pins
    }

}
void loop() {
  mqtt.loop();
  lcd.setCursor(0, 0);
  lcd.print(myString);
  if (my2Str > 0) {
    lcd.setCursor(0, 1);
    lcd.print(my2Str);
    lcd.setCursor(12, 1);
    if(mode == 1 || mode == 3 ){
    lcd.print(amount - my2Str);
    }
    else if(mode == 2){
      lcd.print(amount + my2Str);
    }
  }

  char keypressed = getKey();
    if (keypressed != NO_KEY) {
        Serial.println(keypressed);
        switch (keypressed) {
            case '1':
                lcd.clear();
                myString = "wait scan";
                my2Str = 0;
                mode = 1;
                break;
            case '2':
                lcd.clear();
                myString = "increase..";
                my2Str = 0;
                mode = 2;
                break;
            case '3':
                lcd.clear();
                myString = "decrease..";
                my2Str = 0;
                mode = 3;
                break;
            case 'A':
                my2Str += 1;
                lcd.clear();
                break;
            case 'B':
                my2Str -= 1;
                lcd.clear();
                break;
            case 'D':
                if(mode == 1){
                  recordsale();
                  lcd.clear();
                  myString = "Record Success";
                  my2Str = 0;
                  delay(1000);
                  lcd.clear();
                  myString = "wait scan";
                  id = 0;
                  name = "";
                  amount = 0;
                  price = 0;
                  mode = 0;
                }
                else if(mode == 2){
                  increaseproduct();
                  lcd.clear();
                  myString = "increase Success";
                  my2Str = 0;
                  delay(1000);
                  lcd.clear();
                  myString = "increase..";
                  id = 0;
                  name = "";
                  amount = 0;
                  price = 0;
                  mode = 0;
                }
                else if(mode == 3){
                  decreaseproduct();
                  lcd.clear();
                  myString = "decrease Success";
                  my2Str = 0;
                  delay(1000);
                  lcd.clear();
                  myString = "decrease..";
                  id = 0;
                  name = "";
                  amount = 0;
                  price = 0;
                  mode = 0;
                }
                break;
            default:
                break;
        }
    }

  Usb.Task(); // ทำงานต่อเนื่องเพื่อรับข้อมูลจากอุปกรณ์ USB
}

// เมื่อกดปุ่มคีย์บอร์ดหรือสแกนบาร์โค้ด
void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  uint8_t asciiChar = OemToAscii(mod, key);
  if (asciiChar) {
    OnKeyPressed(asciiChar);
  }
}

// แสดงผลเมื่อกดปุ่มสำเร็จ
void KbdRptParser::OnKeyPressed(uint8_t key) {
  if (keyIndex < sizeof(keys) - 1) {
    keys[keyIndex] = key;
    keyIndex++;
    keys[keyIndex] = '\0'; // เพิ่ม null terminator เพื่อบ่งชี้ว่าจบ string
  } 
  if (keyIndex == 13) {
    output();
  }
}

//method สำหรับ ส่งข้อมูลออกจาก arduino
void KbdRptParser::output() {
  Serial.println(keys);
  keyIndex = 0;
   String productCode = String("/product_code/") + String(keys);
if (client.connect(server, 443)) { // ใช้ port 443 สำหรับ HTTPS
        Serial.println("Connected to server");
        
        // สร้าง GET request
        client.println("GET " + String(productCode) + " HTTP/1.1");
        client.println("Host: " + String(server));
        client.println("Connection: close");
        client.println(); // หมายถึงการสิ้นสุดคำขอ
    } else {
        Serial.println("Connection to server failed");
    }

    // อ่านการตอบกลับจากเซิร์ฟเวอร์
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
    }

    // ปิดการเชื่อมต่อ
    client.stop();

}


char getKey() {
    // Scan the rows and detect pressed keys
    for (int row = 0; row < numRows; row++) {
        // Set the current row LOW and the rest HIGH
        digitalWrite(rowPins[row], LOW);
        for (int col = 0; col < numCols; col++) {
            if (digitalRead(colPins[col]) == LOW) {
                // A key is pressed, return the corresponding key from the keymap
                while (digitalRead(colPins[col]) == LOW); // Wait for the key to be released
                digitalWrite(rowPins[row], HIGH); // Reset the row to HIGH
                return keymap[row][col];
            }
        }
        digitalWrite(rowPins[row], HIGH); // Set the row back to HIGH
    }
    return NO_KEY;
}

void recordsale() {
   String productCode = String("/recordsales/product_code/") + String(keys) + String(my2Str);
if (client.connect(server, 443)) { // ใช้ port 443 สำหรับ HTTPS
        Serial.println("Connected to server");
        
        // สร้าง GET request
        client.println("GET " + String(productCode) + " HTTP/1.1");
        client.println("Host: " + String(server));
        client.println("Connection: close");
        client.println(); // หมายถึงการสิ้นสุดคำขอ
    } else {
        Serial.println("Connection to server failed");
    }

    // อ่านการตอบกลับจากเซิร์ฟเวอร์
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
    }

    // ปิดการเชื่อมต่อ
    client.stop();
}

void increaseproduct() {
   String productCode = String("/increaseproduct/") + String(id) + String(my2Str);
if (client.connect(server, 443)) { // ใช้ port 443 สำหรับ HTTPS
        Serial.println("Connected to server");
        
        // สร้าง GET request
        client.println("GET " + String(productCode) + " HTTP/1.1");
        client.println("Host: " + String(server));
        client.println("Connection: close");
        client.println(); // หมายถึงการสิ้นสุดคำขอ
    } else {
        Serial.println("Connection to server failed");
    }

    // อ่านการตอบกลับจากเซิร์ฟเวอร์
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
    }

    // ปิดการเชื่อมต่อ
    client.stop();
}

void decreaseproduct() {
   String productCode = String("/decreaseproduct/") + String(id) + String(my2Str);
if (client.connect(server, 443)) { // ใช้ port 443 สำหรับ HTTPS
        Serial.println("Connected to server");
        
        // สร้าง GET request
        client.println("GET " + String(productCode) + " HTTP/1.1");
        client.println("Host: " + String(server));
        client.println("Connection: close");
        client.println(); // หมายถึงการสิ้นสุดคำขอ
    } else {
        Serial.println("Connection to server failed");
    }

    // อ่านการตอบกลับจากเซิร์ฟเวอร์
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            Serial.println(line);
        }
    }

    // ปิดการเชื่อมต่อ
    client.stop();
}
