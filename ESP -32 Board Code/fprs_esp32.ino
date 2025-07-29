#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// Define RX and TX pins for UART2 (Change if needed)
#define RXD2 16
#define TXD2 17

HardwareSerial serialPort(2); // UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);

uint8_t id;
uint8_t getFingerprintEnroll();

void setup() {
  Serial.begin(115200);  // Use a higher baud rate for debugging
  serialPort.begin(57600, SERIAL_8N1, RXD2, TXD2);  // Start UART2 with defined pins
  delay(100);
  Serial.println("\n\nESP32 Fingerprint Sensor Enrollment");

  // Initialize fingerprint sensor
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("\nERROR: Fingerprint sensor NOT detected!");
    Serial.println("Check wiring: TX -> RXD2, RX -> TXD2, and Power (3.3V/5V)");
    while (1) { delay(1000); } // Stop execution
  }

  Serial.println(F("Reading sensor parameters..."));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: 0x")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

uint8_t readnumber() {
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {
  Serial.println("\nReady to enroll a fingerprint!");
  Serial.println("Enter ID # (1-127) to save the fingerprint:");
  id = readnumber();
  if (id == 0) return; // ID #0 not allowed

  Serial.print("Enrolling ID #");
  Serial.println(id);
  while (!getFingerprintEnroll());
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("\nPlace finger for enrollment, ID: "); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      Serial.print(".");
      delay(500);
    } else if (p != FINGERPRINT_OK) {
      Serial.println("Error capturing fingerprint, try again.");
      return p;
    }
  }
  Serial.println("\nImage captured successfully!");

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Failed to convert image, try again.");
    return p;
  }

  Serial.println("Remove finger and place again...");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  Serial.println("Now place the same finger again.");

  while ((p = finger.getImage()) != FINGERPRINT_OK);
  Serial.println("Second image captured!");

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Failed to process second image, try again.");
    return p;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprints did not match, retry enrollment.");
    return p;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprint stored successfully!");
  } else {
    Serial.println("Failed to store fingerprint, try another ID.");
    return p;
  }

  return true;
}
