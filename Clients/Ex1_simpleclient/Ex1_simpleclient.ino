/**
 * Corrigé de l'exercice 1, support de cours LoRa
 * Philippe Rochat
 * février 2018
 */
#include "SPI.h"
#include "Wire.h"
#include <LiquidCrystal_I2C.h>
#include <RH_RF95.h>

LiquidCrystal_I2C lcd(0x27, 16,2); // Instanciate a lcd display
RH_RF95           rf95;     // Instanciate a LoRa driver
byte              c = 1;     // Heart Beat
uint32_t          lcount = 0; // Loop counter
char              cursors[4] = {'\\', '|', '/', '-'};
uint8_t BackSlash[8]= { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
#define LOOP_IDLE 100000
uint8_t *buf; // A buffer where to write down incoming messages

// Setup and start LCD I2C screen (with backpack)
void lcd_setup() {
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, BackSlash);
  lcd.setCursor(0,0);
  lcd.print("LCD I2C OK");
}

// Setup LoRa modem
void lora_setup() {
  lcd.setCursor(0,1);
  if (!rf95.init()) {
    lcd.print("ERR: lora setup");
    delay(2000);
  }
  // Setup ISM frequency
  rf95.setFrequency(868);
  // Setup Power,dBm
  rf95.setTxPower(23);
  lcd.print("Lora OK");
}

// Heartbeat on screen (turning bar)
void heartBeat() {
  if(lcount++ > LOOP_IDLE) { // Every loop idle, play HB
    lcount = 0;
    lcd.setCursor(15,0);
    if(c > 3) {
      c = 0;
      lcd.print(char(0));
    } else {
      lcd.print(cursors[c]);
    }
    c++;
  }  
}

// Check for a LoRa message. Return the message size or 0 if none, empty or error
uint8_t checkMessage() {
  if (rf95.available()) { // We have a message
    lcd.clear();
    uint8_t size = rf95.maxMessageLength();
    memset(buf, 0, size); // Fill buffer with 0
    if (rf95.recv(buf, &size)) {
      //buf[size] = '\0'; // Not required since it's already filled with 0
      lcd.setCursor(0,0);
      lcd.printstr((char *)buf+2); 
    } else {
        lcd.print("> recv fail");
        return 0;
    }
    return size;
  } else {
    return 0;
  }
}

void setup() {
  lcd_setup();
  lora_setup();
  delay(1000);
#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
#endif
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Setup OK");
  lcd.setCursor(0,1);
  lcd.print("Starting ...");
  delay(1000);
  lcd.clear();
  //lcd.autoscroll(); 
  buf = (uint8_t *)malloc(rf95.maxMessageLength());
}

void loop() {
  checkMessage();
  // Heartbeat on screen
  heartBeat();
}
