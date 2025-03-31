#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <main.h>

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