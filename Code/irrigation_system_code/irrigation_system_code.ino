#include <DFRobot_SIM808.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define PHONE_NUMBER          "05319110077"

#define PIN_BUTTON            2
#define PIN_MOTOR             8
#define PIN_TX                10
#define PIN_RX                11
#define PIN_MOISTURE          A0
#define PIN_WATER             A1

#define PIN_WATER_LED_HIGH    6
#define PIN_WATER_LED_LOW     9
#define PIN_RUN_LED           7

#define MESSAGE_LENGTH        160
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];

bool isRun = true;
int pushButton = 1;
int buttonState = 0;
int lastbuttonState = 1;
int waitValue = 0;
bool isLedPress = false;
bool sendWaterLevelSms = false;

LiquidCrystal_I2C dis(0x27, 16, 2);
SoftwareSerial softSerial(PIN_TX, PIN_RX);
DFRobot_SIM808 sim808(&softSerial);           //Connect RX, TX, PWR,

void setup() {
    Wire.begin();

    softSerial.begin(9600);
    Serial.begin(9600);

    //******** Initialize SIM808 module *************
    while (!sim808.init()) {
        delay(1000);
        Serial.print("SIM808 INIT ERROR\r\n");
    }
    Serial.println("SIM808 INIT SUCCESS");

    pinMode(PIN_MOTOR, OUTPUT);
    digitalWrite(PIN_MOTOR, HIGH);

    pinMode(PIN_WATER_LED_HIGH, OUTPUT);
    digitalWrite(PIN_WATER_LED_HIGH, LOW);

    pinMode(PIN_WATER_LED_LOW, OUTPUT);
    digitalWrite(PIN_WATER_LED_LOW, LOW);
    
    pinMode(PIN_RUN_LED, OUTPUT);
    digitalWrite(PIN_RUN_LED, LOW);

    pinMode(PIN_BUTTON, INPUT);

    // attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), Interrupt_Function, FALLING);
    
    dis.init();
    dis.backlight();
    dis.clear();

    dis.setCursor(0, 0);
    dis.print("IRRIGATION");
    dis.setCursor(0, 1);
    dis.print("SYSTEM IS ON ");
    for (int a = 12; a <= 14; a++) {
        dis.setCursor(a, 1);
        dis.print(".");
        delay(500);
    }
    dis.clear();

    //******** define phone number and text **********
    sim808.sendSMS(PHONE_NUMBER, "SYSTEM OPENED.");
}

void loop() {   

    buttonState = digitalRead(PIN_BUTTON);
      
    if (buttonState != lastbuttonState) {
        //Serial.println("IF");
        if (buttonState == 1) {
            if (pushButton == 0) {
                pushButton = 1;
            }
            else {
                pushButton = 0;
                isLedPress = false;
                digitalWrite(PIN_RUN_LED, LOW);
                //goto stopstep;
            }
        }
        lastbuttonState = buttonState;
    }
    else  {
      if (pushButton == 0 && !isLedPress) {
        digitalWrite(PIN_MOTOR, HIGH);
        digitalWrite(PIN_WATER_LED_HIGH, LOW);
        digitalWrite(PIN_WATER_LED_LOW, LOW);
        dis.clear();
        dis.setCursor(0, 0);
        dis.print("PRESS BUTTON...");
        isLedPress = true;
      }     
    }

    if (pushButton == 1) {
        messageIndex = sim808.isSMSunread();
        if (messageIndex > 0) {
            sim808.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);
            //*********** In order not to full SIM Memory, is better to delete it **********  
            sim808.deleteSMS(messageIndex);
            //Serial.print("From number: ");
            //Serial.println(phone);
            //Serial.print("Datetime: ");
            //Serial.println(datetime);
            Serial.print("Recieved Message: ");
            Serial.println(message);
        }

        String msgStr(message);

        if (msgStr.indexOf("#ON") != -1) {
            isRun = true;
            dis.clear();
            digitalWrite(PIN_RUN_LED, HIGH);
            sim808.sendSMS(PHONE_NUMBER, "IRRIGATION SYSTEM STARTED.");
            dis.setCursor(0, 0);
            dis.print("SYSTEM IS ON...");
        }
        else if (msgStr.indexOf("#OFF") != -1) {
            isRun = false;
            dis.clear();
            sim808.sendSMS(PHONE_NUMBER, "IRRIGATION SYSTEM STOPPED.");
            digitalWrite(PIN_MOTOR, HIGH);
            digitalWrite(PIN_RUN_LED, LOW);
            dis.setCursor(0, 0);
            dis.print("SYSTEM IS OFF...");
        }

        if (isRun) {
            digitalWrite(PIN_RUN_LED, HIGH);
            
            int moistureValue = analogRead(PIN_MOISTURE);
            //Serial.println(moistureValue);

            int waterValue = analogRead(PIN_WATER);
            Serial.println(waterValue);

            int waterPercentage = map(waterValue, 60, 700, 0, 100);
            Serial.println(waterPercentage);
            
            if(waterValue <= 100) {
              digitalWrite(PIN_WATER_LED_LOW, HIGH);
              digitalWrite(PIN_WATER_LED_HIGH, LOW);

              if(!sendWaterLevelSms) {
                if (waterPercentage < 0) { waterPercentage = 0; }
                String waterInfoSMS = "WATER LEVEL DECREASED. ";
                String infoSMS = waterInfoSMS + "\r\nWATER: %" + waterPercentage;
                //Serial.println(infoSMS);
                sim808.sendSMS(PHONE_NUMBER, infoSMS.c_str());
                sendWaterLevelSms = true;
              }              
            } else {
              digitalWrite(PIN_WATER_LED_HIGH, HIGH);
              digitalWrite(PIN_WATER_LED_LOW, LOW);
              sendWaterLevelSms = false;
            }
            
           if (msgStr.indexOf("#MOTORWAIT") != -1) {
                //Serial.println("WAIT");
                String waitStr = msgStr;
                String lastChar = waitStr.substring(waitStr.length() - 1);
                //waitStr.replace("#MOTORWAIT","");
                //Serial.println(lastChar);
                waitValue = lastChar.toInt();
                //Serial.println(waitValue);
                waitStr == "";
           }
           
           if (msgStr.indexOf("#INFO") != -1) {
                String stringOne = "MOISTURE: ";
                String infoSMS = stringOne + moistureValue + "\r\nWATER: " + waterValue;
                sim808.sendSMS(PHONE_NUMBER, infoSMS.c_str());
            }

            if (moistureValue < 599) {
                digitalWrite(PIN_MOTOR, HIGH);
                dis.setCursor(0, 0);
                dis.print("MOTOR IS OFF");
                dis.setCursor(0, 1);
                dis.print("MOISTURE : HIGH");

                if (msgStr.indexOf("#MOTORON") != -1) {
                    digitalWrite(PIN_MOTOR, LOW);
                    dis.setCursor(0, 0);
                    dis.print("MOTOR IS ON ");
                    //sim808.sendSMS(PHONE_NUMBER, "IRRIGATION STARTED.");
                    dis.setCursor(0, 1);
                    dis.print("MOISTURE : LOW ");
                    goto stopstep;
                }
            }
            else if (moistureValue > 600 && moistureValue < 900) {
                digitalWrite(PIN_MOTOR, HIGH);
                dis.setCursor(0, 0);
                dis.print("MOTOR IS OFF");
                dis.setCursor(0, 1);
                dis.print("MOISTURE : MID ");

                if (msgStr.indexOf("#MOTORON") != -1) {
                    digitalWrite(PIN_MOTOR, LOW);
                    dis.setCursor(0, 0);
                    dis.print("MOTOR IS ON ");
                    //sim808.sendSMS(PHONE_NUMBER, "IRRIGATION STARTED...");
                    dis.setCursor(0, 1);
                    dis.print("MOISTURE : LOW ");
                    goto stopstep;
                }
            }
            else if (moistureValue > 900) {
                digitalWrite(PIN_MOTOR, LOW);
                dis.setCursor(0, 0);
                dis.print("MOTOR IS ON ");
                //sim808.sendSMS(PHONE_NUMBER, "IRRIGATION STARTED...");
                dis.setCursor(0, 1);
                dis.print("MOISTURE : LOW ");

                if (waitValue != 0) {
                    digitalWrite(PIN_MOTOR, HIGH);
                    dis.clear();
                    dis.setCursor(0, 0);
                    dis.print("MOTOR WAIT...");
                    for (int i = 1; i <= waitValue; i++) {
                        dis.setCursor(0, 1);
                        dis.print(i);
                        delay(i*1000);
                    }
                    //delay(waitValue*1000);
                    waitValue = 0;
                    goto stopstep;
                    //digitalWrite(PIN_MOTOR, LOW);
                }
                
                if (msgStr.indexOf("#MOTOROFF") != -1) {
                    digitalWrite(PIN_MOTOR, HIGH);
                    dis.clear();
                    dis.setCursor(0, 0);
                    dis.print("MOTOR IS OFF");
                    dis.setCursor(0, 1);
                    dis.print("MOISTURE : HIGH");
                    goto stopstep;
                }
            }
        }
        
         

stopstep:
        memset(message, 0, sizeof message);  // message array clear
        msgStr == ""; 
        
        delay(4000);
    }   // IF PUSHBUTTON
    else {        
        //dis.clear();
        //dis.setCursor(0, 0);
        //dis.print("PRESS BUTTON...");
    }

    delay(20);
//stopstep:

//    delay(4000);
}

//Interrupt Function
void Interrupt_Function()
{
    Serial.println("Interrupt_Function");
    buttonState = digitalRead(PIN_BUTTON);   
    //Serial.println(buttonState); 
}
