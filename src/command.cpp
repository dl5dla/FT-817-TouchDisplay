// command.cpp

// the funtions in this modules handle the commands
// sent via wifi and mqtt to the FT817/818

#include <ArduinoJson.h>
#include "ft817.h"

extern FT817 radio;

extern bool current_vfo;


bool process_commands(char * cmd) {

    #define MAX_ELEMENTS 3  // max 3 elements in commands
    #define MAX_CHARS    10  // max. 4 characters per element

    char part[MAX_ELEMENTS][MAX_CHARS+1];
    uint8_t count = 0;
    char* command = strtok(cmd, " ");
    while (command != NULL)
    {
        if(count >= MAX_ELEMENTS) return false;   // too many elements in command
        strncpy(part[count],command,MAX_CHARS);
        command = strtok(NULL, " ");
        count++;
   
    }

Serial.println(cmd);

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, cmd);

    if (error) {
        Serial.println("ERROR");
        return(false);
    }

    const char* band = doc["band"];
    const char* mode = doc["mode"];   
    const char* vfo = doc["vfo"];
    const char* ipo = doc["ipo"];
    const char* att = doc["att"];
    const char* nb = doc["nb"];


    if(band) {
        byte new_band = atoi(band);
        if(new_band > 14) return false;
        Serial.println("EEPROM WRITE: setBandVFO())");
        radio.setBandVFO(current_vfo, new_band);
    }

    else if(mode) {
        byte new_mode = atoi(mode);
        // filter out unvalid code values:
        if(new_mode > 12 ||
           new_mode == 5 ||
           new_mode == 7 ||
           new_mode == 9 ||
           new_mode == 11)
            return false;
        Serial.println("EEPROM WRITE: setMode())");
        radio.setMode(new_mode);
    }

    else if(vfo) {
        Serial.println("EEPROM WRITE: toggleVFO()");
        radio.toggleVFO();
    }
    else if(ipo) {
        Serial.println("EEPROM WRITE: toggleIPO()");
        radio.toggleIPO();
    }
    else if(att) {
        Serial.println("EEPROM WRITE: toggleATT()");
        radio.toggleATT();
    }
    else if(nb) {
        Serial.println("EEPROM WRITE: toggleNB()");
        radio.toggleNB();
    }

    return true;

}

