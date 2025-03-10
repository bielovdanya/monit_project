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
#define WARNING 150
#define MIN_VOLT 0.6
#define MAX_VOLT 3.3
#define MAX_DUST 500
#define ADC 4095.0
#define BUZZER_F 5500
#define FILT_DECAY 0.6
#define FILT_RISE 0.8
#define FILT_FALL 0.7
#define FILT_NEW 0.2
#define RES_DELAY 2000
#define UPDT_INT 5000

//AQI
#define AMODE1 50
#define AMODE2 100
#define AMODE3 150
#define AMODE4 200
#define AMODE5 300

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
    ledcSetup(0, BUZZER_F, 8); 
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
    
    float voltage = sensorValue * (MAX_VOLT / ADC);
    if (voltage < MIN_VOLT) voltage = MIN_VOLT;  
    float dustDensity = (voltage - MIN_VOLT) * (MAX_DUST / (MAX_VOLT- MIN_VOLT));
    dustDensity = constrain(dustDensity, 0, MAX_DUST);  
    
    if (dustDensity > filteredDustDensity) {
        filteredDustDensity = (filteredDustDensity * FILT_RISE) + (dustDensity * FILT_NEW);
        lastSmokeTime = millis();
    } else {
        if (millis() - lastSmokeTime > RES_DELAY) {
            filteredDustDensity *= FILT_DECAY;
        } else {
            filteredDustDensity = (filteredDustDensity * FILT_FALL) + (dustDensity * (1 - FILT_FALL));
        }
    }

    String airQ;
    if (filteredDustDensity <= AMODE1) {
        airQ = "Good";
    } else if (filteredDustDensity <= AMODE2) {
        airQ = "Moderate";
    } else if (filteredDustDensity <= AMODE3) {
        airQ = "Unhealthy for Sensitive";
    } else if (filteredDustDensity <= AMODE4) {
        airQ = "Unhealthy";
    } else if (filteredDustDensity <= AMODE5) {
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

    if (millis() - lastUpdate > UPDT_INT) {  
        if (filteredDustDensity >= WARNING) {  
            ledcWriteTone(0, BUZZER_F); 
        } else {
            ledcWriteTone(0, 0);  
        }
        lastUpdate = millis();
    }

    delay(1000);  
}
