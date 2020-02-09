//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Main file

//Include the libraries
#include "defs.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_MPU6050.h>
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
Adafruit_MPU6050 mpu;

//Global computer state
uint32_t global_state;

//Calibrated ground level ASL
float ground_level = 0;
//Mean value of last 10 baro measurements
float baro_mean = 0;
int baro_mean_cnt = 0;
//The last mean value of last 10 baro measurements
float baro_mean_last = 0;

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
                break;
            case STATE_FLIGHT:
                led_pulse(HIGH, 500);
                led_pulse(LOW, 500);
                break;
        }
    }
}

void sensor_task(void* args){
    //Read launch detection acceleration threshold
    float launch_thres = settings_read_value("launch_thres").toFloat();

    //Calibrate ground level
    ground_level = bmp.readAltitude();

    while(1){
        float temp_flt; //a temporary variable to store float values

        //Read IMU data
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        //Write IMU data
        rfd_set_field("acc_x", *(uint32_t*)&(temp_flt = a.acceleration.x));
        rfd_set_field("acc_y", *(uint32_t*)&(temp_flt = a.acceleration.y));
        rfd_set_field("acc_z", *(uint32_t*)&(temp_flt = a.acceleration.z));
        rfd_set_field("gyro_x", *(uint32_t*)&(temp_flt = g.gyro.x * 57.29578)); //This magic number here
        rfd_set_field("gyro_y", *(uint32_t*)&(temp_flt = g.gyro.y * 57.29578)); //  converts radians to degrees
        rfd_set_field("gyro_z", *(uint32_t*)&(temp_flt = g.gyro.z * 57.29578));
        rfd_set_field("imu_temp", *(uint32_t*)&(temp_flt = temp.temperature));
        //Calculate acceleration magnitude
        float acc_m = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
        if(acc_m > launch_thres && (global_state == STATE_IDLE)){
            Serial.printf("Acceleration of %3.1f m/s2 detected, threshold %3.1f m/s2\n", acc_m, launch_thres);
            flight_begin();
        }

        //Write hall sensor value
        rfd_set_field("hall", hallRead());

        //Write barometer data
        rfd_set_field("baro_height", *(uint32_t*)&(baro_mean += (temp_flt = bmp.readAltitude() - ground_level)));
        baro_mean_cnt++;
        //Actually get the mean value
        if(baro_mean_cnt >= 10){
            baro_mean_cnt = 0;
            baro_mean /= 10.0f;
            //Check if the mean altitude has decreased by at least a meter
            if(baro_mean_last - baro_mean >= 1.0f){
                //Deploy pyro
                rfd_set_field("pyro0", 1);
                digitalWrite(PIN_PYRO, HIGH);
            }
            //Set the last baro alt
            baro_mean_last = baro_mean;
            //Reset the mean value
            baro_mean = 0;
        }
    }
}

void flight_begin(){
    //Don't start a flight if we're not ready
    if(global_state != STATE_IDLE)
        return;
    //Try to find a filename that's not taken
    int file_no = 0;
    for(int i = 1; i < 10000; i++){
        if(!SPIFFS.exists("/flight_" + String(i))){
            Serial.println("Starting flight #" + String(i));
            file_no = i;
            break;
        }
    }
    //Begin RFD file
    rfd_begin("/flight_" + String(file_no), settings_read_value("rfd_format"));
    //Set pyro to 0
    rfd_set_field("pyro0", 0);
    //Set the state to "flight"
    global_state = STATE_FLIGHT;
}

void flight_end(){
    Serial.println("Ending flight");
    //End RFD file
    rfd_end();
    //Set the state to "idle"
    global_state = STATE_IDLE;
}

void shell_task(void* args){
    while(1){
        //Wait for a command
        while(!Serial.available());
        String input = Serial.readStringUntil('\n');
        //Print it back
        Serial.print(F("shell>"));
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
        } else if(cmd == "rm"){
            //Get the filename from command
            String fn = split_str(input, ' ', 1);
            //Check if this file exists
            if(SPIFFS.exists(fn)){
                //Remove it
                SPIFFS.remove(fn);
            } else {
                //Print an error
                Serial.println("error: file does not exist");
            }
        } else if(cmd == "baro"){
            //Read the barometer
            float alt = bmp.readAltitude();
            Serial.printf("Altitude: %5.1f\n", alt);
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
        } else if(cmd == "fb"){
            flight_begin();
        } else if(cmd == "fe"){
            flight_end();
        } else if(cmd == "fv"){
            //Get the filename from command
            String fn = "/flight_" + split_str(input, ' ', 1);
            //Check if this file exists
            if(SPIFFS.exists(fn)){
                rfd_print(fn);
            } else {
                //Print an error
                Serial.println("error: file does not exist");
            }
        } else if(cmd == "fsd"){
            int ub = SPIFFS.usedBytes();
            int tb = SPIFFS.totalBytes();
            Serial.printf("SPIFFS: %i kB used out of %i kB (%3.1f%%)\n", ub / 1024, tb / 1024, ((float)ub * 100.0f) / (float)tb);
        } else if(cmd == "fsfmt"){
            SPIFFS.format();
            Serial.println("Formatting done");
        } else {
            Serial.println("Unrecognized command");
        }
        //Delay a little bit
        delay(200);
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
    pinMode(PIN_PYRO, OUTPUT);
    digitalWrite(PIN_PYRO, LOW);
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if(!bmp.begin())
        rn_abort("Barometer initialization failed", __FILE__, __LINE__);
    if(!mpu.begin())
        rn_abort("IMU initialization failed", __FILE__, __LINE__);
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    if(!SPIFFS.begin(true))
        rn_abort("SPIFFS initialization failed", __FILE__, __LINE__);

    if(!SPIFFS.exists(SETTINGS_FILE)){
        Serial.println("No settings file, creating");
        File f = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
        f.print(SETTINGS_DEFAULT);
        f.close();
    }

    //Create a sensor readout task
    xTaskCreateUniversal(&sensor_task, "SensorTask", 8192, NULL, 6, NULL, 1);
    //Create a serial shell task
    xTaskCreateUniversal(&shell_task, "ShellTask", 8192, NULL, 6, NULL, 1);
}

void loop(){}