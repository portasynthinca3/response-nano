//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Main file

//Include the libraries
#include "defs.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <FS.h>
#include <SPIFFS.h>

//Internal ESP32 temperature sensor readout code
#ifdef __cplusplus
  extern "C" {
#endif
  uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

//Hardware library objects
Adafruit_BMP085 bmp;

//Global computer state
uint32_t global_state;

String split_str(String data, char separator, int index){
    //Code from https://stackoverflow.com/questions/29671455/how-to-split-a-string-using-a-specific-delimiter-in-arduino#29673158
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;
    
    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }

    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int count_chars_str(String data, char c){
    int cnt = 0;
    for(int i = 0; i < data.length(); i++)
        if(data.charAt(i) == c)
            cnt++;
    return cnt;
}

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
            case STATE_FLIGHT:
                led_pulse(HIGH, 500);
                led_pulse(LOW, 500);
                break;
                break;
        }
    }
}

void shell_task(void* args){
    while(1){
        Serial.print(F("shell>"));
        //Wait for a command
        while(!Serial.available());
        String input = Serial.readStringUntil('\n');
        //Print it back
        Serial.println(input);
        //Parse the command
        String cmd = split_str(input, ' ', 0);
        if(cmd == "view"){
            //Get the filename from command
            String fn = split_str(input, ' ', 1);
            //Check if this file exists
            if(SPIFFS.exists(fn)){
                //Open it
                File f = SPIFFS.open(fn, FILE_READ);
                Serial.printf("[File size: %i]\n", f.size());
                //Read it
                f.seek(0);
                char buffer[1024];
                while(f.available()){
                    int rd = f.readBytes(buffer, 1024);
                    Serial.write((const uint8_t*)buffer, rd);
                }
                Serial.println();
                f.close();
            } else {
                //Print an error
                Serial.println("error: file does not exist");
            }
        } else if(cmd == "viewhex"){
            //Get the filename from command
            String fn = split_str(input, ' ', 1);
            //Check if this file exists
            if(SPIFFS.exists(fn)){
                //Open it
                File f = SPIFFS.open(fn, FILE_READ);
                Serial.printf("[File size: %i]\n", f.size());
                //Read it
                f.seek(0);
                char buffer[1024];
                while(f.available()){
                    int rd = f.readBytes(buffer, 1024);
                    for(int i = 0; i < rd; i++){
                        uint8_t byte = buffer[i];
                        Serial.printf("%X%X ", byte / 16, byte % 16);
                        if(i % 16 == 7)
                            Serial.print(" ");
                        if(i % 16 == 15)
                            Serial.println();
                    }
                }
                Serial.println();
                f.close();
            } else {
                //Print an error
                Serial.println("error: file does not exist");
            }
        } else if(cmd == "baro"){
            //Read the barometer
            float temp = bmp.readTemperature();
            int pressure = bmp.readPressure();
            float alt = bmp.readAltitude();
            Serial.printf("Temperature: %3.1f\t\tPressure: %i\t\tAltitude: %5.1f\n", temp, pressure, alt);
        } else if(cmd == "espsns"){
            //Read the ESP sensors
            int hall = hallRead();
            int temp = temprature_sens_read();
            float temp_celsius = ((float)temp - 32.0f) / 1.8;
            Serial.printf("Hall Sensor: %i\t\tTemperature: %3.1f\n", hall, temp);
        } else if(cmd == "state"){
            //Read the argument
            String state_str = split_str(input, ' ', 1);
            //Set the state
            global_state = state_str.toInt();
            Serial.printf("State successfully changed to %i\n", global_state);
        } else if(cmd == "rfdb"){
            rfd_begin("/rfd_test", "acc_x:float acc_y:float acc_z:float gyro_x:float gyro_y:float gyro_z:float baro_height:float pyro0:int hall:int baro_temp:int");
            Serial.println("RFD begin");
        } else if(cmd == "rfde"){
            rfd_end();
            Serial.println("RFD end");
        } else if(cmd == "rfdevt"){
            rfd_event("Event!");
            Serial.println("RFD event");
        } else {
            Serial.println("Unrecognized command");
        }
    }
}

void rn_abort(String message, String file, int line){
    //Error out
    //(worst case scenario, hopefully this will not happen during flight :<)
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
    Serial.setTimeout(3);
    Serial.println(SERIAL_GREETINGS);

    //Reset the state
    global_state = STATE_IDLE;
    //Create an LED blinking task
    xTaskCreateUniversal(&led_blink_task, "LedBlinkTask", 2048, NULL, 5, NULL, 0);

    //Initialize the hardware
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if(!bmp.begin())
        rn_abort("Barometer initialization failed", __FILE__, __LINE__);
    if(!SPIFFS.begin(true))
        rn_abort("SPIFFS initialization failed", __FILE__, __LINE__);

    SPIFFS.remove(SETTINGS_FILE);

    if(!SPIFFS.exists(SETTINGS_FILE)){
        Serial.println("No settings file, creating");
        File f = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
        f.print(SETTINGS_DEFAULT);
        f.close();
    }

    //Create a serial shell task
    xTaskCreateUniversal(&shell_task, "ShellTask", 8192, NULL, 6, NULL, 1);
}

void loop(){
}
