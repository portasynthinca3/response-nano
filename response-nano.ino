//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Main file

//Include the libraries
#include "defs.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>

//Hardware library objects
Adafruit_BMP085 bmp;

//Global computer state
uint32_t global_state;

void led_pulse(int level, uint32_t time){
    //Set a level on the LED pin
    //(it's active low, so invert)
    digitalWrite(PIN_LED, !level);
    //Wait 'time' milliseconds
    delay(time);
}

void led_blink_task(void* args){
    //Set the LED pin as output
    pinMode(PIN_LED, OUTPUT);
    while(1){
        //Depending on the computer's state
        //  blink a certain pattern
        switch(global_state){
            case STATE_IDLE:
                led_pulse(HIGH, 50);
                led_pulse(LOW, 1950);
                break;
            case STATE_CRASH:
                for(int i = 0; i < 4; i++){
                    led_pulse(HIGH, 250);
                    led_pulse(LOW, 250);
                }
                delay(1000);
                break;
        }
    }
}

void rn_abort(String message, String file, int line){
    //Error out
    //(worse case scenario, hopefully this will not happen during flight :<)
    Serial.println(F("---=== RESPONSE NANO CRASH REPORT ===---"));

    Serial.print(F("An unrecovable error has occured in the firmware at the following location:\n  in file "));
    Serial.print(file);
    Serial.print(F(" at line "));
    Serial.println(line);

    Serial.println(message);

    //Set the "crash" state
    global_state = STATE_CRASH;
    //Hang
    while(1);
}

void setup(){
    //Initialize the serial port
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(SERIAL_GREETINGS);

    //Reset the state
    global_state = STATE_IDLE;
    //Create an LED blinking task
    xTaskCreateUniversal(&led_blink_task, "LedBlinkTask", 512, NULL, 6, NULL, 0);

    //Initialize the hardware
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if(!bmp.begin())
        rn_abort("Barometer initialization failed", __FILE__, __LINE__);
}

void loop(){
    Serial.print("Reading alt... ");
    float alt = bmp.readAltitude();
    Serial.println(alt);
    delay(500);
}