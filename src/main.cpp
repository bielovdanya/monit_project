#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDRESS 0x3C  
#define SDA_PIN 8  
#define SCL_PIN 9  
#define LED_PIN 15
#define A0_PIN 4   
#define BUZZER_PIN 20
float WARNING = 150;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool buzzerActivated = false;
void setup() {
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    Wire.begin(SDA_PIN, SCL_PIN); 
    Wire.setClock(100000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        while (1); 
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    ledcSetup(0, 5000, 8); 
    ledcAttachPin(BUZZER_PIN, 0); 
}

void loop() {
    static unsigned long lastUpdate = millis();
    static float filteredDustDensity = 0;
    static unsigned long lastSmokeTime = 0;

    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(280);
    int sensorValue = analogRead(A0_PIN);
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(9680);
    float voltage = sensorValue * (3.3 / 4095.0);
    if (voltage < 0.6) voltage = 0.6;  
    float dustDensity = (voltage - 0.6) * (500.0 / (3.3 - 0.6));  //масштабування значень від 0 до 500
    dustDensity = constrain(dustDensity, 0, 500);  
    if (dustDensity > filteredDustDensity) {//фільтрація значень
        filteredDustDensity = (filteredDustDensity * 0.8) + (dustDensity * 0.2);
        lastSmokeTime = millis();
    } else {
        if (millis() - lastSmokeTime > 2000) {
            filteredDustDensity *= 0.6;
        } else {
            filteredDustDensity = (filteredDustDensity * 0.7) + (dustDensity * 0.3);
        }
    }
    String airQ;
    if (filteredDustDensity <= 50) {
        airQ = "Good";
    } else if (filteredDustDensity <= 100) {
        airQ = "Moderate";
    } else if (filteredDustDensity <= 150) {
        airQ = "Unhealthy for Sensitive";
    } else if (filteredDustDensity <= 200) {
        airQ = "Unhealthy";
    } else if (filteredDustDensity <= 300) {
        airQ = "Very Unhealthy";
    } else {
        airQ = "Hazardous";
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Dust: ");
    display.print(filteredDustDensity, 1);
    display.print(" ug/m3");
    display.setCursor(0, 20);
    display.print("AQI: ");
    display.print(airQ);
    display.display();
    if (millis() - lastUpdate > 5000) {  
        if (filteredDustDensity >= WARNING) {  
            ledcWriteTone(0, 5000); 
        } else {
            ledcWriteTone(0, 0);  
        }
        lastUpdate = millis();
    }

    delay(1000);  
}
