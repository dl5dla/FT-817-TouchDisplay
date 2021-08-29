//  main.h

// IMPORTANT: if you are using a FT-817 instead of a FT-818, comment out the line
// "#define FT818" in ft817.h. The Yaesu FT-818 uses a slightly different address map
// for the EEPROM. The memory of the VFOb on the FT-818 starts at address 416 (dec) while at the FT817 
// the address on the FT-817 uses address 390 (dec).

#include "FS.h"
#include "ft817.h"


// Declarations of the functions. This is needed to avoid that platformio
// complains about unknwon functions during complilation if they are not
// defined in main.c in th right order.

uint8_t check_menu_buttons();
uint8_t check_touch();
void drawKeypad();
void show_freq();
void button_events(uint8_t);
void draw_lock(int32_t,int32_t);
void update_menu(uint8_t);
void sec2time(char *, uint32_t);

extern void calibrate_touch();
extern void plotNeedle(int, byte);
extern void drawSMeter();
extern void show_bar(uint8_t, byte);
extern void init_all(void);
extern bool get_vfo(void);
extern void get_band(void);
extern void get_mode(void);
extern void get_steps(void);
extern void get_ipo(void);
extern void get_att(void);
extern void get_nb(void);
extern void get_antenna(void);
extern void get_pwr_sets(void);
extern void get_agc(void);
extern void get_chg(void);
extern void is_charging(void);
extern void get_nb_agc_lock(void);
extern void send_mqtt(char *, char *);
extern void  settings2mqtt(void);


// Modes of the FT817/818
char modes[13][4] =
  {"LSB", "USB", "CW", "CWR", "AM", "WFM", "FM", "DIG", "PKT"};//

uint8_t mod_codes[9] = {0, 1, 2, 3, 4, 6, 8, 10, 12 };

// Mapping from band codes to band strings
char bands[16][6] =
  {"160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "6m", "BCR", "Air", "2m", "UHF", "PHT"}; 

// Mapping table from frequency step number to frequency in Hz
long ssbSteps[3] = { 1000,2500,5000};
long amSteps[6] = { 2500,5000,9000,10000,12500,25000 };
long fmSteps[8] = {5000,6250,10000,12500,15000,20000,25000,50000 };

long current_steps;

uint32_t hambands[16][2] = {
  { 1810000, 2000000},        // 160m
  { 3500000, 3800000},        // 80m
  { 5351500, 5366500},        // 60m
  { 7000000, 7200000},        // 40m
  { 10100000, 10150000},       // 30m
  { 14000000, 14350000},      // 20m
  { 18068000, 18168000},      // 17m
  { 21000000, 21450000},      // 15m
  { 24890000, 24990000},      // 12m
  { 28000000, 29700000},      // 10m
  { 50080000, 51000000},      //  6m
  { 0, 0},                    // BCR
  { 0, 0},                    // Air
  { 144000000, 146000000},    // 2m
  { 430000000, 440000000},    // UHF
  { 0, 0}                     // PHT
  };

char chargeTimes[3][4] = { "6h", "8h", "10h" };

// Create keys for the keypad
char keyLabel[NUM_OF_KEYS*MAX_MENU_PAGES][5] = {
  "BND","MOD","IPO","ATT","TUN",">>",
  "VFO","NB","PWR","ANT","AGC",">>",
  "STP","chg","CHG","","",">>"
 };

// TODO: differentiate between FT818 and FT817
char pwr[4][5] = {"6W","5W","2.5W","1W"};

// Mapping the AGC codes to the strings to be displayed
char agc[4][5] = {"auto","fast", "slow", "off"};

// The following two arrays define how each individual menu button should look like
// in idle or active stage

uint16_t keyColor[NUM_OF_KEYS*MAX_MENU_PAGES] = {
  TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER,
  TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER,
  TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER, TFT_DARKSILVER
};

uint16_t keyColor_active[NUM_OF_KEYS*MAX_MENU_PAGES] = {
  TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKSILVER,
  TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKSILVER,
  TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKRED, TFT_DARKSILVER
};

// Array keeping the status of the menu buttons
bool keyStatus[NUM_OF_KEYS*MAX_MENU_PAGES];

uint8_t menu_page=1;   // part of the menu currently shown on display

// Define the menu button class
TFT_eSPI_Button key[NUM_OF_KEYS*MAX_MENU_PAGES];

FT817 radio; // define "radio" so that we may pass CAT commands

extern EspMQTTClient mqtt;

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN); // invoke touch library

// the updateTime is used for a timer which control how often time consuming 
// EEPROM read commands (get commands) are sent to the FT817/818
uint32_t updateTime = 0;

long f;
long old_freq=0;
int old_sm=0;
byte old_mode=0;
bool old_vfo=false;
uint8_t edit_frequ = 0;

byte current_ssb_step;
byte current_am_step;
byte current_fm_step;

byte current_agc;
byte current_pwr;
byte current_chg;
bool charging = false;      // true if device is currently charging the battery

byte current_mode=0;      // that all the get-calls are done once in the setup()
                          // already.
bool locked = false;      // if FT817 is locked
byte current_band = 0;
bool  current_vfo = false;
bool refresh_required = false;
long x,y;                 // current pointer, is set at each touch event
byte txStatus = 0;        // holds response from radio.chkTX(). If 0 the TRX is in receive
                          // mode, otherwise contains status of SPLIT, SWR, PTT
byte old_txStatus = 0;
uint32_t start_tx_time;

bool mqtt_connected = false;

Status_field field[14];