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

void rfd_begin(String filename, String scheme){
    //Open the file
    rfd_file = SPIFFS.open(filename, FILE_WRITE);
    //Parse the scheme from the string
    rfd_f_count = count_chars_str(scheme, ' ') + 1;
    rfd_scheme = (rfd_field_t*)malloc(rfd_f_count * sizeof(rfd_field_t));
    for(int i = 0; i < rfd_f_count; i++){
        String desc = split_str(scheme, ' ', i);
        String name = split_str(desc, ':', 0);
        String type = split_str(desc, ':', 1);
        int type_int = 0;
        if(type == "int")
            type_int = RFD_TYPE_INT;
        if(type == "float")
            type_int = RFD_TYPE_FLOAT;
        if(type == "str")
            type_int = RFD_TYPE_STRING;
        rfd_scheme[i].type = type_int;
        memcpy(rfd_scheme[i].name, name.c_str(), 32);
        memset(&rfd_scheme[i].name[name.length()], 0, 32 - name.length());
    }
    //Write the header to the file
    rfd_file.print("RFD0");
    //Write the field scheme to the file
    rfd_file.write((uint8_t)rfd_f_count);
    rfd_file.write((const uint8_t*)rfd_scheme, rfd_f_count * sizeof(rfd_field_t));
}

void rfd_event(String str){
    //Write an event
    rfd_file.write((uint8_t)1);
    rfd_file.write((const uint8_t*)str.c_str(), str.length() + 1);
    rfd_file.write((uint8_t)0);
}

void rfd_end(){
    //Close the file
    rfd_file.close();
}