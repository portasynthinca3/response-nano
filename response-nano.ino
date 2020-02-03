//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Main file

//Include the libraries
#include "defs.h"
#include <Wire.h>

uint32_t global_state;

void led_pulse(int level, uint32_t time){
    digitalWrite(PIN_LED, !level);
    delay(time);
}

void led_blink_task(void* args){
    pinMode(PIN_LED, OUTPUT);
    while(1){
        switch(global_state){
            case STATE_IDLE:
                led_pulse(HIGH, 100);
                led_pulse(LOW, 1900);
                break;
        }
    }
}

void setup(){
    //Initialize the serial port
    Serial.begin(SERIAL_BAUD_RATE);
    //Print a greeting message
    Serial.println(SERIAL_GREETINGS);
    //Reset the state
    global_state = STATE_IDLE;
    //Create an LED blinking task
    xTaskCreateUniversal(&led_blink_task, "LedBlinkTask", 1024, NULL, 7, NULL, 0);
}

void loop(){

}