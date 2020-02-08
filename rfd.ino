//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Response Flight Data format

//Include the libraries
#include "defs.h"
#include <FS.h>
#include <SPIFFS.h>

//The current file
File rfd_file;
//The current field scheme
rfd_field_t* rfd_scheme;
//The current field count
uint32_t rfd_f_count;
//The write task handle
TaskHandle_t wrt_task_handle;
//The last time logging task wrote something
uint64_t wrt_task_last_wrt;
//Logging task delay in ms
uint64_t wrt_task_dly;

void wrt_task(void* args){
    wrt_task_last_wrt = millis();
    while(1){
        //If time has come
        if(millis() - wrt_task_last_wrt >= wrt_task_dly){
            //Write an update marker
            rfd_file.write((uint8_t)0);
            //Scan through the fields
            for(int i = 0; i < rfd_f_count; i++){
                //Write the value
                rfd_file.write((const uint8_t*)&(rfd_scheme[i].value), 4);
            }
            //Update the time
            wrt_task_last_wrt = millis();
        }
        //Delay for freeRTOS' watchdog not to trigger
        delay(wrt_task_dly / 2);
    }
}

void rfd_begin(String filename, String scheme){
    //Open the file
    rfd_file = SPIFFS.open(filename, FILE_WRITE);
    //Parse the scheme from the string
    rfd_f_count = count_chars_str(scheme, ' ') + 1;
    rfd_scheme = (rfd_field_t*)malloc(rfd_f_count * sizeof(rfd_field_t));
    for(int i = 0; i < rfd_f_count; i++){
        //Get one field descriptor
        String desc = split_str(scheme, ' ', i);
        String name = split_str(desc, ':', 0);
        String type = split_str(desc, ':', 1);
        //Decode its type
        int type_int = 0;
        if(type == "int")
            type_int = RFD_TYPE_INT;
        if(type == "float")
            type_int = RFD_TYPE_FLOAT;
        //Assign the field
        rfd_scheme[i].type = type_int;
        memcpy(rfd_scheme[i].name, name.c_str(), 32);
        memset(&rfd_scheme[i].name[name.length()], 0, 32 - name.length());
    }
    //Write the header to the file
    rfd_file.print("RFD0");
    //Write the field scheme to the file
    rfd_file.write((uint8_t)rfd_f_count);
    for(int i = 0; i < rfd_f_count; i++){
        rfd_file.write((const uint8_t*)rfd_scheme[i].name, 32);
        rfd_file.write((uint8_t)rfd_scheme[i].type);
    }

    //Read the update frequency
    wrt_task_dly = 1000 / settings_read_value("rfd_data_freq").toInt();

    //Create the write task
    xTaskCreateUniversal(&wrt_task, "RFDWriteTask", 8192, NULL, 5, &wrt_task_handle, 0);
}

void rfd_set_field(String field, uint32_t val){
    //Scan the field list
    for(int i = 0; i < rfd_f_count; i++){
        //Check if names are equal
        if(memcmp(rfd_scheme[i].name, field.c_str(), field.length()) == 0){
            //Set the value
            rfd_scheme[i].value = val;
            return;
        }
    }
}

void rfd_event(String str){
    //Write an event marker
    rfd_file.write((uint8_t)1);
    //Write the event itself
    rfd_file.write((const uint8_t*)str.c_str(), str.length());
    rfd_file.write((uint8_t)0);
}

void rfd_end(){
    //Stop the task
    vTaskDelete(wrt_task_handle);
    //Close the file
    rfd_file.close();
}