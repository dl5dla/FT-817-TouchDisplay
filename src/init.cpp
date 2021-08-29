// init.c

#define TFT_DARKBLUE 0x018C

#include "ft817.h"
#include "globals.h"

extern void update_menu(uint8_t);
extern bool  current_vfo;
extern byte current_ssb_step;
extern byte current_am_step;
extern byte current_fm_step;
extern long ssbSteps[];
extern long amSteps[];
extern long fmSteps[];
extern long current_steps;
extern byte current_mode;
extern byte current_band;
extern char bands[16][6];
extern bool keyStatus[NUM_OF_KEYS*MAX_MENU_PAGES];
extern byte current_pwr;
extern char pwr[4][5];
extern char agc[4][5];
extern byte current_agc;
extern byte current_chg;
extern char chargeTimes[3][4];
extern uint8_t mod_codes[9];
extern byte old_mode;
extern char modes[13][4];
extern bool locked;
extern bool charging;
extern bool refresh_required;

extern TFT_eSPI tft;
extern Status_field field[14];

extern void draw_lock();
extern void setup_mqtt();
extern void onConnectionEstablished();

extern void send_mqtt(char *, char *);
extern void send_mqtt_ant(char *name, byte);

extern FT817 radio;
extern EspMQTTClient mqtt;

bool get_vfo(void);
void get_band(void);
void get_mode(void);
void get_steps(void);
void get_ipo(void);
void get_att(void);
void get_nb(void);
void get_antenna(void);
void get_pwr_sets(void);
void get_agc(void);
void get_chg(void);
void is_charging(void);
void get_nb_agc_lock(void);
void settings2mqtt();

byte current_ant;
bool current_ipo;
bool current_att;
bool current_nb;
bool first_run = true;


void init_all() {

  for(uint8_t i=0; i<13; i++)
    field[i].drawField("wait", TFT_YELLOW, TFT_DARKBLUE);

  get_vfo();
  get_band();
  get_mode();
  get_steps();
  get_ipo();
  get_att();
 // get_nb();  // TEST
  get_antenna();
  get_pwr_sets();
// get_agc(); //TEST
  get_chg();
  is_charging();
  get_nb_agc_lock();

  setup_mqtt();

  first_run = false;
}

bool get_vfo() {

  byte res = radio.getVFO();    // EEPROM read

  if(current_vfo != res || first_run) {
    current_vfo = res;

    if (radio.eepromValidData)
    {
      if (current_vfo) {
        field[0].drawField("B", TFT_YELLOW, TFT_DARKBLUE);
        send_mqtt("vfo", "B");
      }
      else {
        field[0].drawField("A", TFT_YELLOW, TFT_DARKBLUE);
        send_mqtt("vfo", "A");
//        mqtt.publish("FT817/settings/vfo", "A");
      }

      return true;
    }
    else {
      return false;
    }

  }
  
}

void get_band() {

  byte res = radio.getBandVFO(current_vfo);

  if(current_band != res || first_run) {
    current_band = res;

    tft.fillRect(60,130,40,28,TFT_BLACK);
    tft.setTextColor(TFT_CYAN,TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(bands[current_band],80,135,2);
    send_mqtt("band", bands[current_band]);
    refresh_required = true;
    update_menu(0);
  }
}

void get_mode() {

  uint8_t m = radio.getMode();
  byte res;

  for(uint8_t i=0; i<9;i++)
    if(mod_codes[i]==m)
      res = i;

  if( current_mode != res || first_run ) {
    current_mode = res;
    field[1].drawField(modes[current_mode], TFT_YELLOW, TFT_DARKBLUE); 
    send_mqtt("mode", modes[current_mode]);
    update_menu(1);
  }

}

void get_steps() {

  // get configured steps for ssb, am and fm:
  long res=radio.getSteps();

  if(current_steps != res || first_run) {
    current_steps = res;

    current_ssb_step = (current_steps & 0b11000000)>>6;
    current_am_step =  (current_steps & 0b00111000)>>3;
    current_fm_step =  current_steps & 0b00000111;

    char buf[10];
    if(current_mode == 4)
      sprintf(buf,"%gk", amSteps[current_am_step]/1000.0);
    else if(current_mode == 5 || current_mode == 6 || current_mode == 8 )
      sprintf(buf,"%gk", fmSteps[current_fm_step]/1000.0);
    else 
      sprintf(buf,"%gk", ssbSteps[current_ssb_step]/1000.0);

    field[6].drawField(buf, TFT_YELLOW, TFT_DARKBLUE);
    sprintf(buf,"%d", ssbSteps[current_ssb_step]);
    send_mqtt("ssbStep", buf);
    sprintf(buf,"%d", amSteps[current_am_step]);
    send_mqtt("amStep", buf);
    sprintf(buf,"%d", fmSteps[current_fm_step]);
    send_mqtt("fmStep", buf);

    update_menu(12);
  }

}

void get_ipo() {

  bool res = radio.getIPO();

  if(current_ipo != res || first_run) {
    current_ipo = res;

    if(current_ipo) {
      keyStatus[2]=true;
      field[2].drawField("ON", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("ipo", "on");
    }
    else {
      keyStatus[2]=false;
      field[2].drawField("off", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("ipo", "off");
    }
    update_menu(2);
  }

}

void get_att() {

  bool res = radio.getATT(); // EEPROM read

  if(current_att != res || first_run) {
    current_att = res;

    if(current_att) {
      keyStatus[3]=true;
      field[3].drawField("ON", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("att", "on");
    }
    else {
      keyStatus[3]=false;
      field[3].drawField("off", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("att", "off");
    }

    update_menu(3);
  }

}

void get_nb_agc_lock() {

  byte byte57 = radio.get_Byte57();

  // get status of NoiseBlocker:
  bool res = (byte57 & 0b00100000) >> 5;

  if(current_nb != res || first_run) {
    current_nb = res;

    if(current_nb) {
      keyStatus[7]=true;
      field[4].drawField("ON", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("nb", "on");
    }
    else {
      keyStatus[7]=false;
      field[4].drawField("off", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("nb", "off");
    }
    update_menu(7);
  }

  // get status of the AGC:

  static byte old_agc = -1;
  current_agc = byte57 & 0b00000011;

  if(current_agc != old_agc) {
    old_agc = current_agc;

    field[5].drawField(agc[current_agc], TFT_YELLOW, TFT_DARKBLUE);
    send_mqtt("agc", agc[current_agc]);
    update_menu(10);
  }

  boolean res2 = !((byte57 & 0b01000000) >> 6);

  if(locked != res2 || first_run) {
    locked = res2;

    draw_lock();
    if(locked)
      send_mqtt("locked", "yes");
    else
      send_mqtt("locked", "no");
  }
}


void get_nb() {

  bool res = radio.getNB(); // EEPROM read

  if(current_nb != res || first_run) {
    current_nb = res;

    if(current_nb) {
      keyStatus[7]=true;
      field[4].drawField("ON", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("nb", "on");
    }
    else {
      keyStatus[7]=false;
      field[4].drawField("off", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("nb", "off");
    }
    update_menu(7);
  }

}

void get_antenna() {

  byte res = radio.getAnt();    // EEPROM read

  if(current_ant != res || first_run) {
    current_ant = res;

    uint8_t bit;
    if(current_band < 10) bit = 0;    // HF bands
    else bit = current_band - 9;      // bands 6m, FM, Air, 2m or UHF)

    if( bitRead(current_ant,bit) ) {
      field[8].drawField("REAR", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("antenna", "REAR");
    }
    else {
      field[8].drawField("FRONT", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("antenna", "FRONT");
    }
    send_mqtt_ant("antenna", current_ant);
  }

}

void get_pwr_sets() {

  static byte old_pwr = -1;
  byte res = radio.getPowerSets();

  if(res != old_pwr) {
    old_pwr = res;
    current_pwr = res;
    field[7].drawField(pwr[current_pwr], TFT_YELLOW, TFT_DARKBLUE);
    send_mqtt("pwr", pwr[current_pwr]);
    update_menu(8);
  }

}

void get_agc() {

  byte res = radio.getAGC();

  if(current_agc != res || first_run) {
    current_agc = res;

    field[5].drawField(agc[current_agc], TFT_YELLOW, TFT_DARKBLUE);
    send_mqtt("agc", agc[current_agc]);
    update_menu(10);
  }

}

void get_chg() {

static byte old_chg = -1;
  byte res = radio.getChargeTime();

  if(res != old_chg || first_run) {
    old_chg = res;
    current_chg = res;

    field[9].drawField(chargeTimes[current_chg], TFT_YELLOW, TFT_DARKBLUE);
    send_mqtt("chargeTime", chargeTimes[current_chg]);
  }


}

void is_charging() {

  bool res = radio.get_CHG_Status();

  if(charging != res || first_run) {
    charging = res;

    if(charging) {
      keyStatus[14]=true;
      field[10].drawField("ON", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("charging", "on");
    }
    else {
      keyStatus[14]=false;
      field[10].drawField("off", TFT_YELLOW, TFT_DARKBLUE);
      send_mqtt("charging", "off");
    }
    update_menu(14);
  }

}

void settings2mqtt() {

    if (current_vfo) send_mqtt("vfo", "B");
     else send_mqtt("vfo", "A");

    send_mqtt("band", bands[current_band]);
    send_mqtt("mode", modes[current_mode]);

    current_ssb_step = (current_steps & 0b11000000)>>6;
    current_am_step =  (current_steps & 0b00111000)>>3;
    current_fm_step =  current_steps & 0b00000111;
    char buf[10];
    sprintf(buf,"%d", ssbSteps[current_ssb_step]);
    send_mqtt("ssbStep", buf);
    sprintf(buf,"%d", amSteps[current_am_step]);
    send_mqtt("amStep", buf);
    sprintf(buf,"%d", fmSteps[current_fm_step]);
    send_mqtt("fmStep", buf);

    if(current_ipo) send_mqtt("ipo", "on");
    else send_mqtt("ipo", "off");

    if(current_att) send_mqtt("att", "on");
    else send_mqtt("att", "off");

    if(current_nb) send_mqtt("nb", "on");
    else send_mqtt("nb", "off");

    send_mqtt("agc", agc[current_agc]);

    if(locked) send_mqtt("locked", "yes");
    else send_mqtt("locked", "no");

    uint8_t bit;
    if(current_band < 10) bit = 0;    // HF bands
    else bit = current_band - 9;      // bands 6m, FM, Air, 2m or UHF)

    if( bitRead(current_ant,bit) )
      send_mqtt("antenna", "REAR");
    else
      send_mqtt("antenna", "FRONT");

    send_mqtt_ant("antenna", current_ant);

    send_mqtt("pwr", pwr[current_pwr]);
    send_mqtt("chargeTime", chargeTimes[current_chg]);

    if(charging) send_mqtt("charging", "on");
    else send_mqtt("charging", "off");

}