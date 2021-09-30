/************************************************************************************************************************
 * TITULO: HMI firmware
 * AUTOR: Jhimmy Astoraque Durán
 * DESCRIPCION: Firmware para microcontrolador que permite controlar desde una app (hmi) un lcd, relay, motor dc con pwm, dht11, leds, potenciometro.
 *              mediante un puerto SoftwareSerial y bluetooth.
 * CANAL YOUTUBE: https://www.youtube.com/c/jadsatv
 * Version: 1.0.2
 * © 2021
 * **********************************************************************************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <Wire.h>


// Arduino Pin Variables
const uint8_t TX2 = 2;
const uint8_t RX2 = 3;                  // software serial for BT
const uint8_t IN3 = 5;  // motorB pin1  //IN3
const uint8_t IN4 = 4;  // motorB pin2  //IN4
const uint8_t ENB = 6;  // PWM pinVVVV  //ENB
const uint8_t IN2 = 7;  // motorA pin2  //IN2
const uint8_t IN1 = 8;  // motorA pin1  //IN1
const uint8_t ENA = 9;  // PWM          //ENA
const uint8_t blueLed = 10;             //D03
const uint8_t greenLed = 11;            //D04
const uint8_t yellowLed = 12;           //D02
const uint8_t redLed = 13;              //D01  -> DIGITAL OUT 1
const uint8_t dhtPin = 15; // A1        //DI1  -> DIGIAL INPUT 1
const uint8_t relay1Pin = 16;  //A2     //DO5  -> DIGITAL OUT 5
const uint8_t relay2Pin = 17;  //A3     //DO6
const uint8_t potPin = A0;              //AI1    
const uint8_t analogPin2 = A6;          //AI2    
const uint8_t analogPin3 = A7;          //AI3    
// A4 and A5 reserved for I2C

// BT
SoftwareSerial bt(RX2, TX2); // rx tx on arduino

// Oled i2c
const uint8_t ancho = 128; //px
const uint8_t alto = 64;
Adafruit_SH1106G oled = Adafruit_SH1106G(ancho, alto, &Wire);
String oledMsgRow1 = "";
String oledMsgRow2 = "";
String clockRow = "";
String minutes = "";
String hours = "";

// dht
const int DHTTYPE = DHT11;
uint32_t dhtStartMillis;
uint32_t dhtCurrentMillis;
uint32_t dhtPeriod = 2250UL; // UL forces unsigned long constant
DHT dht(dhtPin, DHTTYPE);

// analog reading var handlers
uint32_t analogReadingsInitMillis;
uint32_t analogReadingsCurrentMillis;
uint32_t analogReadingsInterval = 500UL; // UL forces unsigned long constant

// Potentiomenter
uint16_t potentiomenterLecture = 0;
uint16_t potCurrentLecture;

// Any analog sensor in A6 (pcb AI2)
uint16_t analogSensor2Lecture = 0;
uint16_t analogSensor2CurrentLecture;

// pwm motor
uint16_t dutyCycle;
uint8_t pwmValue;

// Serial communication  works with physical COM or BT
String inputCommand = "";
bool commandReceived = false;


void setup()
{
    // enabling serial and bluetooth Com
    //Serial.begin(9600);
    bt.begin(9600);

    // pin config for outputs
    pinMode(redLed, OUTPUT);
    pinMode(yellowLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(relay1Pin, OUTPUT);
    pinMode(relay2Pin, OUTPUT);
    pinMode(IN1, OUTPUT);   // motor 1 control
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(IN3, OUTPUT);   // motor 2 control
    pinMode(IN4, OUTPUT);
    pinMode(ENB, OUTPUT);

    // relay default off
//    digitalWrite(relay1Pin, HIGH); // cause it's low trigger
//    digitalWrite(relay2Pin, HIGH);

    // initialize Oled
    oled.begin(0x3C, true); // address 0x3C 
    oled.display();
    delay(1500);
    oled.clearDisplay();
    oled.display();
    oled.setTextSize(1);
    oled.setTextColor(SH110X_WHITE);

    // init dht
    dht.begin();
    
    // init timers
    dhtStartMillis = millis();
    analogReadingsInitMillis = millis();

    //Serial.println("Setup Finished");
} // eof setup


void loop()
{
    /****************** TX PART ********************/
    // DHT
    dhtCurrentMillis = millis();  //get the current time
    if (dhtCurrentMillis - dhtStartMillis >= dhtPeriod)  //test whether the period has elapsed
    {
        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        int h = dht.readHumidity(); // int truncate decimals cause it is a dht11       
        int t = dht.readTemperature(); // Read temperature as Celsius
        
        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t)) {
            //Serial.println("Failed to read from DHT sensor!");
            return;
        }

        // Send data
        bt.print("#H");        
        bt.println(h);
        bt.print("#T");
        bt.println(t);
        dhtStartMillis = dhtCurrentMillis;  // restart timer

        // debug DHT
//        Serial.print("#H");        
//        Serial.println(h);
//        Serial.print("#T");
//        Serial.println(t);
    }
    // ANALOG READINGS FOR AI1 AI2 (Potentiomenter, Any other sensor)
    analogReadingsCurrentMillis = millis();

    if (analogReadingsCurrentMillis - analogReadingsInitMillis >= analogReadingsInterval){
        // read the adc
        potCurrentLecture = analogRead(potPin);
        analogSensor2CurrentLecture = analogRead(analogPin2);

        // Send data only if lecture changes
        if ((potCurrentLecture > potentiomenterLecture + 1 || potCurrentLecture < potentiomenterLecture - 1)){
            bt.print("#A");
            potentiomenterLecture = potCurrentLecture;
            bt.println(potentiomenterLecture);
        }

        if ((analogSensor2CurrentLecture > analogSensor2Lecture + 1 || analogSensor2CurrentLecture < analogSensor2Lecture - 1)){
            bt.print("#B");
            analogSensor2Lecture = analogSensor2CurrentLecture;
            bt.println(analogSensor2Lecture);
        }
        analogReadingsInitMillis = analogReadingsCurrentMillis;
    }

    /****************** TX PART ********************/

    // RX PART
    if (commandReceived){
        inputCommand.trim();
        //Debug
    //    Serial.print("Comando recibido desde c#: ");
    //    Serial.println(inputCommand);
                
        // process the cmd
        if (inputCommand.equals("$Ron")){
            digitalWrite(redLed, HIGH);
        }
        else if (inputCommand.equals("$Roff")){
            digitalWrite(redLed, LOW);
        }
        else if (inputCommand.equals("$Yon")){
            digitalWrite(yellowLed, HIGH);
        }
        else if (inputCommand.equals("$Yoff")){
            digitalWrite(yellowLed, LOW);
        }
        else if (inputCommand.equals("$Bon")){
            digitalWrite(blueLed, HIGH);
        }
        else if (inputCommand.equals("$Boff")){
            digitalWrite(blueLed, LOW);
        }
        else if (inputCommand.equals("$Gon")){
            digitalWrite(greenLed, HIGH);
        }
        else if (inputCommand.equals("$Goff")){
            digitalWrite(greenLed, LOW);
        }
        else if( inputCommand.indexOf("$L") != -1){
            oledMsgRow1 = inputCommand.substring(2, 10); // filter the msg part
            oledMsgRow2 = inputCommand.substring(10);            
            updateOledScreen();
        }
        else if (inputCommand.indexOf("$T") != -1){
            // clock data from hmi
            clockRow = inputCommand.substring(2);
            updateOledScreen();
        }
        else if (inputCommand.equals("$Eon")){ // for relay
            digitalWrite(relay1Pin, HIGH);
        }
        else if (inputCommand.equals("$Eoff")){ // for relay
            digitalWrite(relay1Pin, LOW);
        }
        else if (inputCommand.equals("$Fon")){ // for relay 2 that is not  LOW LEVEL TRIGGER
            digitalWrite(relay2Pin, HIGH);
        }
        else if (inputCommand.equals(("$Foff"))){
            digitalWrite(relay2Pin, LOW);
        }
        else if (inputCommand.indexOf("$P") != -1){    // Motor 1
            dutyCycle = inputCommand.substring(2).toInt();
            dutyCycle = constrain(dutyCycle, 0, 100);
            pwmValue = map(dutyCycle, 0, 100, 0 , 255);
            if (pwmValue == 0){
                digitalWrite(IN1, LOW);
                digitalWrite(IN2, LOW);
                analogWrite(ENA, pwmValue);
            }
            else{
                digitalWrite(IN1, HIGH);
                digitalWrite(IN2, LOW);
                analogWrite(ENA, pwmValue);
            }
        }
        else if (inputCommand.indexOf("$S") != -1){
          showScreenSaver();
        }
        
        // reset command
        inputCommand = "";
        commandReceived = false;      
    } // end if

    // Check for incoming data commands from hmi
    checkBtCmds(); 

} // eof loop


void checkBtCmds(){
    while (bt.available() > 0){
        char c = (char)bt.read();
        if (c == '\n'){
            commandReceived = true;
        }
        else{
            inputCommand += c;
        }
    }
}


void updateOledScreen(){
    oled.fillRect(0, 0, 128, 64, SH110X_BLACK);
    // clock
    oled.setCursor(30, 5);
    oled.setTextSize(2);
    oled.print(clockRow);

    // row messages
    oled.setCursor(0, 30);
    oled.println(oledMsgRow1);
    oled.print(oledMsgRow2);
    oled.display();
}

void showScreenSaver(){
  // a trick
  oled.begin(0x3C, true); // address 0x3C
  oled.display();
}
