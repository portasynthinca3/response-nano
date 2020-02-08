//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//Settings manager

//Include the libraries
#include "defs.h"
#include <FS.h>
#include <SPIFFS.h>

String settings_read_value(String key){
    //Open the settings file
    File f = SPIFFS.open(SETTINGS_FILE, FILE_READ);
    f.setTimeout(100);
    //Find the key we need
    if(!f.find(key.c_str(), key.length())){
        f.close();
        return "<not found>";
    }
    //Seek one byte
    f.seek(f.position() + 1);
    //Read the value
    String val = f.readStringUntil('\n');
    //Close the file and return
    f.close();
    return val;
}

void settings_write_value(String key, String value){
    //Open the settings file
    File f = SPIFFS.open(SETTINGS_FILE, FILE_READ);
    f.setTimeout(100);
    //Read the entire file
    String settings = f.readString();
    f.close();
    //Try to overwrite the previous value if it exists
    for(uint32_t i = 0; i < count_chars_str(settings, '\n') + 1; i++){
        //Get the entry and its key
        String entry = split_str(settings, '\n', i);
        String e_key = split_str(entry, '=', 0);
        //Compare keys
        if(e_key == key){
            //Find the position at which it starts in the settings
            int idx = settings.indexOf(e_key);
            //Get the remainder of the settings list
            String settings_after = settings.substring(idx + entry.length());
            //Get the trailer of the settings list
            String settings_before = settings.substring(0, idx - 1);
            //Construct a new string
            settings = settings_before + key + "=" + value + settings_after;
            //Close the file, re-open it for write
            f.close();
            f = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
            //Write new settings, close the file
            f.write((const uint8_t*)settings.c_str(), settings.length());
            f.close();
            //Return
            return;
        }
    }
    //If we didn't return, there are no existing entries with this key;
    //  create a new entry
    settings += "\n" + key + "=" + value;
    //Close the file, re-open it for write
    f.close();
    f = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
    //Write new settings, close the file
    f.write((const uint8_t*)settings.c_str(), settings.length());
    f.close();
}