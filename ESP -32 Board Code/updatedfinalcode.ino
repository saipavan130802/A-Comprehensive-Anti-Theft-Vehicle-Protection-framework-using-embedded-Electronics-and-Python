#include <TinyGPS++.h>
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// GPS Module Pins (UART1)
#define RXD1 33  // GPS RX (ESP32 TX1)
#define TXD1 32  // GPS TX (ESP32 RX1)

// Fingerprint Sensor Pins (UART2)
#define RXD2 16
#define TXD2 17

// GSM Module Pins (UART3)
#define RXD3 26  // GSM RX (ESP32 TX3)
#define TXD3 27  // GSM TX (ESP32 RX3)

// Other Sensors and Actuators
#define VIBRATION_SENSOR_PIN 34  
#define BUZZER_PIN 25  
#define ENG_PIN 18
#define SHOCK_PIN 19

// Serial Communication
HardwareSerial gpsSerial(1);  // UART1 for GPS
TinyGPSPlus gps;
HardwareSerial serialPort(2); // UART2 for Fingerprint
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);
HardwareSerial gsmSerial(3);  // UART3 for GSM

// LCD screen (I2C address is typically 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 16 columns and 2 rows

// Global Variables for GPS Data
String lat_str, lng_str;

// Function Prototypes
void checkVibration();
void SendMessage();
uint8_t getFingerprintID();
static void smartDelay(unsigned long ms);
void checkGSMCommands();

void setup() {
  Serial.begin(115200);  // USB Serial for Debugging

  // Initialize GPS
  gpsSerial.begin(9600, SERIAL_8N1, RXD1, TXD1);

  // Initialize Fingerprint Sensor
  serialPort.begin(57600, SERIAL_8N1, RXD2, TXD2);
  finger.begin(57600);

  // Initialize GSM Module
  gsmSerial.begin(9600, SERIAL_8N1, RXD3, TXD3);

  // Set up Sensor and Control Pins
  pinMode(VIBRATION_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENG_PIN, OUTPUT);
  pinMode(SHOCK_PIN, OUTPUT);

  digitalWrite(ENG_PIN, LOW);
  digitalWrite(SHOCK_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Initializing");
  delay(2000);

  Serial.println("ESP32 GPS, Vibration, Fingerprint & GSM System Initialized");

  // Check if fingerprint sensor is detected
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Detected!");
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR: Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("Sensor NOT Detected!");
    while (1) { delay(1000); }  // Stop execution
  }
}

void loop() {
  smartDelay(1000);  // Process GPS data
  checkVibration();  // Check vibration sensor
  getFingerprintID(); // Check fingerprint authentication
  checkGSMCommands(); // Check for incoming GSM messages
}

// Function to Process GPS Data
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
  } while (millis() - start < ms);
}

// Function to Check Vibration and Send Location
void checkVibration() {
  int vibrationStatus = digitalRead(VIBRATION_SENSOR_PIN);

  if (vibrationStatus == HIGH) { 
    Serial.println("⚠️ WARNING: Vibration detected! ⚠️");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);

    digitalWrite(SHOCK_PIN, LOW);
    delay(5000);
    digitalWrite(SHOCK_PIN, HIGH);
    SendMessage();
  }
}

// Function to Send GPS Location as Alert
void SendMessage() {
  double lat_val = gps.location.lat();
  double lng_val = gps.location.lng();
  bool loc_valid = gps.location.isValid();

  if (!loc_valid) {
    Serial.println("Location not available");
    return;
  }

  lat_str = String(lat_val, 6);
  lng_str = String(lng_val, 6);

  Serial.println("Sending Alert...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending Alert...");
  lcd.setCursor(0, 1);
  lcd.print("Location: ");
  lcd.print(lat_str);
  lcd.print(", ");
  lcd.print(lng_str);

  gpsSerial.println("AT+CMGF=1");  // Set SMS mode to text
  delay(1000);
  gpsSerial.println("AT+CMGS=\"+919347054969\"");  // Replace with actual mobile number
  delay(1000);
  gpsSerial.println("⚠️ ALERT: Vibration detected!\nLocation: https://www.google.com/maps/?q=" + lat_str + "," + lng_str);
  delay(1000);
  gpsSerial.write(26);  // End SMS (CTRL+Z)
  Serial.println("Message sent");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Message Sent");
  lcd.setCursor(0, 1);
  lcd.print("Alert Sent!");
  delay(2000);
}

// Function to Get Fingerprint Authentication
uint8_t getFingerprintID() {
  Serial.println("\nPlace your finger on the sensor...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place your finger");
  lcd.setCursor(0, 1);
  lcd.print("on the sensor...");

  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    Serial.println("No valid fingerprint detected.");
    return p;
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Failed to convert fingerprint image.");
    return p;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Fingerprint ID: ");
    Serial.println(finger.fingerID);
    Serial.print("Confidence: ");
    Serial.println(finger.confidence);

    digitalWrite(ENG_PIN, HIGH);
    digitalWrite(SHOCK_PIN, HIGH);
    delay(5000);
    digitalWrite(ENG_PIN, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint OK");
    lcd.setCursor(0, 1);
    lcd.print("Engine & Shock ON");
    delay(2000);
  } else {
    Serial.println("Fingerprint NOT recognized!");
    Serial.println("UNAUTHORIZED Access Detected!");
    digitalWrite(ENG_PIN, LOW);
    digitalWrite(SHOCK_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(5000);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(SHOCK_PIN, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unauthorized Access");
    lcd.setCursor(0, 1);
    lcd.print("Buzzer Activated!");
    delay(2000);
  }
  return p;
}

// Function to Check for GSM Commands
void checkGSMCommands() {
  if (gsmSerial.available()) {
    String receivedText = gsmSerial.readString();
    receivedText.trim();

    Serial.print("Received SMS: ");
    Serial.println(receivedText);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Received SMS:");
    lcd.setCursor(0, 1);
    lcd.print(receivedText);

    if (receivedText.indexOf("ENGINE ON") != -1) {
      Serial.println("🔵 Turning Engine ON");
      digitalWrite(ENG_PIN, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("Engine ON");
    } else if (receivedText.indexOf("ENGINE OFF") != -1) {
      Serial.println("🔴 Turning Engine OFF");
      digitalWrite(ENG_PIN, LOW);
      lcd.setCursor(0, 1);
      lcd.print("Engine OFF");
    }
  }
}
