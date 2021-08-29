
#include "globals.h"
#include "main.h"

//TEST
// ---------------------------------------------
//      Setup
// ---------------------------------------------
void setup()
{
  Serial.begin(9600); 
  radio.begin(38400,SERIAL_8N1,16,17);

  // Initialise the TFT screen
  tft.init();

  tft.fillScreen(TFT_BLACK);
  
  // Initialize the touch library

  ts.begin();
  
  // Set the rotation before we calibrate
  tft.setRotation(1);
  ts.setRotation(3);

  // call screen calibration
  //   calibrate_touch();

  // Clear the scree1n
  //tft.fillScreen(TFT_BLACK);

  // Init the status field lines:
  // fields on top of frequency display:

  field[0].initField( 25, 110,   5, 113, 40, 10);
  field[1].initField( 70, 110,  50, 113, 40, 10);
  field[2].initField(115, 110,  95, 113, 40, 10);
  field[3].initField(160, 110, 140, 113, 40, 10);
  field[4].initField(205, 110, 185, 113, 40, 10);
  field[5].initField(250, 110, 230, 113, 40, 10);
  field[6].initField(295, 110, 275, 113, 40, 10);

  // fields below the frequency display:
  field[7].initField(  25, 178,   5, 178, 40, 16);
  field[8].initField(  70, 178,  50, 178, 40, 16);
  field[9].initField( 115, 178,  95, 178, 40, 16);
  field[10].initField(160, 178, 140, 178, 40, 16);
  field[11].initField(205, 178, 185, 178, 40, 16);
  field[12].initField(250, 178, 230, 178, 40, 16);
  field[13].initField(295, 178, 275, 178, 30, 16);

  refresh_required = true;  // ensure that frequency display is initially shown

  drawKeypad();
  drawSMeter();

  init_all();

}

// --------------------------------
//         main loop
// ---------------------------------
void loop()
{
  long temp_f = f;

  // mqtt
#ifdef MQTT
  mqtt.loop();
#endif

  show_freq();

  if(f != temp_f) {
    char frequ_str[20];
    sprintf(frequ_str,"%9.3f",f/1000.0);
    //mqtt.publish("FT817/frequency", frequ_str);
    send_mqtt("frequency", frequ_str);
  }

  uint8_t b = check_menu_buttons();

  button_events(b);

  txStatus = radio.chkTX();

  if(txStatus == 0xFF) {      // RX
    // Update S-Meter
    byte sm = radio.getSMeter();
     if(sm != old_sm) {
     plotNeedle(sm, 0);

     char buf[5];
     sprintf(buf,"%d",((sm*50)/14));
     send_mqtt("smeter", buf);
     Serial.println(buf);

     old_sm = sm;
    }
    if( old_txStatus != txStatus)   // switched back from TX to RX. Restore S-Meter once.
      drawSMeter();
  }
  else {
    char timestr[10];

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_SILVER,TFT_DARKRED);
    if( old_txStatus != txStatus) {     // just starting TX
      start_tx_time = millis();
      tft.fillRect(5, 6, 150, 80, TFT_BLACK);
      tft.drawRect(60,35,30,30,TFT_SILVER);

      tft.drawRect(15, 16, 140, 65,TFT_SILVER);
      tft.fillRect(16, 17, 138, 63,TFT_DARKRED);
      tft.drawString("TX",77,26,4);
    }
    uint32_t tx_time = millis() - start_tx_time;

    sec2time(timestr,tx_time/1000);

    tft.drawString(timestr,58,53,4);
  }
  old_txStatus = txStatus;

  uint16_t tx_meter = radio.read_TX_Meter();    // analog values 0 ... 10

  if( tx_meter == 0xFFFF) 
    tx_meter = 0;           // RX
  
  show_bar((tx_meter & 0x00F0) >> 4,0);  // power meter
  show_bar((tx_meter & 0xF000) >> 12,1); // swr meter
  show_bar((tx_meter & 0x000F),2);       //mod meter
  show_bar((tx_meter & 0x0F00) >> 8,3);  // alc meter

  get_mode();     // read curent mode from FT817/818


  // the following code is called only each one second
  // to reduce frequency of EEPROM reads
  if ( (millis() - updateTime) > 1000) {

    get_steps();  // get configured steps for ssb, am and fm
    if( get_vfo() ) get_band();
    get_ipo();      // get IPO status of the actual channel
    get_att();      // get ATT status of the actual channel 
    get_nb();       // get NB status of the actual channel
    get_antenna();  // get assignment of front/rear antenna plugs to bands  
    get_pwr_sets();
    get_agc();
    get_chg();
    is_charging();
    get_nb_agc_lock();

    if(mqtt.isWifiConnected())
      field[11].drawField("ok", TFT_YELLOW, TFT_DARKBLUE); 
    else
      field[11].drawField("wait", TFT_YELLOW, TFT_DARKBLUE); 

    if(mqtt.isMqttConnected()) {
      if (!mqtt_connected) {
        field[12].drawField("ok", TFT_YELLOW, TFT_DARKBLUE); 
        settings2mqtt();
        mqtt_connected = true;
      }
    }
    else {
      field[12].drawField("wait", TFT_YELLOW, TFT_DARKBLUE);
      mqtt_connected = false;
    }
    updateTime = millis();
  }
 
}

// -------------------------------------------
// Provide a menu to select one of the bands
// -------------------------------------------
uint8_t band_select()
{
  #define  BASEX 24
  #define  BASEY 8
  #define  WIDTH 238
  #define  HEIGHT 230
  #define  BUT_W 48
  #define  BUT_H 48
  #define  BUT_SPACING_X 3
  #define  BUT_SPACING_Y 3

  TFT_eSPI_Button band_button[16];

  tft.setTextFont(2);
  tft.setTextColor(TFT_SILVER,TFT_SILVER);

  tft.fillRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_DARKGREEN);
  tft.drawRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_SILVER);
  tft.drawRect(BASEX+1, BASEY-1, WIDTH-2, HEIGHT-2, TFT_SILVER);

  for(uint8_t row=0;row<4;row++) {
    for(uint8_t col=0;col<4;col++)
    {
      uint16_t color;
      if( current_band == (row*4+col) )
        color = TFT_RED;
      else
        color = TFT_DARKSILVER;

      band_button[row*4+col].initButton(&tft, 
        BASEX + 42 + col * (BUT_W + BUT_SPACING_X),
        BASEY + 38 + row * (BUT_H + BUT_SPACING_Y), 
        BUT_W, BUT_H, TFT_BLACK, 
        color, TFT_WHITE, bands[row*4+col], KEY_TEXTSIZE);
      band_button[row*4+col].drawButton();

    }
  }

  // check which button was pressed:
  while(1) {
    uint8_t pressed = check_touch();

	  for (uint8_t i=0; i<16; i++) {
  	  if (pressed && band_button[i].contains(x, y)) {
        tft.fillRect(0,0,320,240,TFT_BLACK);
        refresh_required = true;  // refresh frequency display
        show_freq();
        drawKeypad();
        drawSMeter();
		    return (i); // i contains the code for the band as used by the FT817/818
      }
	  }
  }

}

// -------------------------------------------
// Provide a menu to select one of the modes
// -------------------------------------------
uint8_t mode_select()
{
  #define  BASEX 24
  #define  BASEY 8  
  #define  WIDTH 182
  #define  HEIGHT 178
  #define  BUT_W 48
  #define  BUT_H 48
  #define  BUT_SPACING_X 3
  #define  BUT_SPACING_Y 3

  TFT_eSPI_Button mode_button[16];

  tft.setTextFont(2);
  tft.setTextColor(TFT_SILVER,TFT_SILVER);

  tft.fillRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_DARKGREEN);
  tft.drawRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_SILVER);
  tft.drawRect(BASEX+1, BASEY-1, WIDTH-2, HEIGHT-2, TFT_SILVER);

  for(uint8_t row=0;row<3;row++) {
    for(uint8_t col=0;col<3;col++)
    {
      uint16_t color;
      if( current_mode == (row*3+col) )
        color = TFT_RED;
      else
        color = TFT_DARKSILVER;

      mode_button[row*3+col].initButton(&tft, 
        BASEX + 42 + col * (BUT_W + BUT_SPACING_X),
        BASEY + 38 + row * (BUT_H + BUT_SPACING_Y), 
        BUT_W, BUT_H, TFT_BLACK, 
        color, TFT_WHITE, modes[row*3+col], KEY_TEXTSIZE);

      mode_button[row*3+col].drawButton();

    }
  }

  // check which button was pressed:
  while(1) {
    uint8_t pressed = check_touch();

	  for (uint8_t i=0; i<9; i++) {
  	  if (pressed && mode_button[i].contains(x, y)) {
        tft.fillRect(0,0,320,240,TFT_BLACK);
        refresh_required = true;  // refresh frequency display
        show_freq();
        drawKeypad();
        drawSMeter();
		    return (i); // i contains the code for the mode as used by the FT817/818
      }
	  }
  }

}

// ######################## TOUCH ########################

void getPoint()
{

  TS_Point p = ts.getPoint(); 
  x = map(p.x, TS_MINX, TS_MAXX, 10, 310);
  y = map(p.y, TS_MINY, TS_MAXY, 10, 230);
}

// -------------------------------------------
// Checks if display is touched and returns
// the duration the screen is touched
// -------------------------------------------
uint8_t check_touch()
{	

	ulong starttime = millis();

	while(ts.touched())
  {
   	if(millis()-starttime > 300) {		  // long press (>300ms)
      getPoint();
      return 2;		
    }
  }

  if(millis()-starttime > 10 ) {	// short press(>10ms)
      getPoint();
		  return 1;
  }

	// no press
	return 0;

}


// ----------------------------
void draw_lock() {

  uint32_t color;
  int32_t x = 30;
  int32_t y = 150;

  tft.fillRect(x-20,y-19,45,31,TFT_BLACK);

  if(locked) {
    color = TFT_RED;
    tft.fillRect(x,y-2,3,5,TFT_BLACK);
  }
  else {
    color = TFT_DARKGREEN;
    tft.fillRect(x,y-2,3,5,TFT_BLACK);
  }

  tft.drawCircle(x+1,y-10,5,color);
  tft.drawCircle(x+1,y-10,6,color);
  tft.fillRect(x-8,y-7,19,12,color);
      
}

// ----------------------------
void drawKeypad()
{
 
  // Draw the keys
  for (uint8_t col = 0; col < NUM_OF_KEYS; col++) {
    uint8_t b = col + (menu_page-1)*NUM_OF_KEYS;
    update_menu(b);
  }

  tft.drawRect(4,4,312,88,TFT_SILVER);
  tft.drawRect(4,92,312,194,TFT_SILVER);
  tft.drawRect(4,194,312,42,TFT_SILVER);

  tft.fillRect(5,93,310,33,TFT_DARKBLUE);
  tft.setTextColor(TFT_WHITE, TFT_DARKBLUE);
  tft.drawCentreString("VFO",25,94,2);
  tft.drawCentreString("MODE",70,94,2);
  tft.drawCentreString("IPO",115,94,2);
  tft.drawCentreString("ATT",160,94,2);
  tft.drawCentreString("NB",205,94,2);
  tft.drawCentreString("AGC",250,94,2);
  tft.drawCentreString("STEPS",295,94,2);

  tft.fillRect(5,164,310,30,TFT_DARKBLUE);
  tft.setTextColor(TFT_WHITE, TFT_DARKBLUE);
  tft.drawCentreString("PWR",25,164,2);
  tft.drawCentreString("ANT",70,164,2);
  tft.drawCentreString("CHG",115,164,2);
  tft.drawCentreString("CHG",160,164,2);
  tft.drawCentreString("Wifi",205,164,2);
  tft.drawCentreString("MQTT",250,164,2);
  tft.drawCentreString("",295,164,2);

  for(uint8_t i=0;i<14;i++)
    field[i].refresh();

}

// --------------------------------------------------------
//  Check, which part of the frequency digits are touched
// --------------------------------------------------------
uint8_t  digits2edit(long old)
{ 

  if( check_touch() )     // touched?
  {

    if( y > 128 && y < 160 ) {
      if(x > 103 && x < 169)  // left
      {
       return(3);
      }
      else if( x > 169 && x < 236 )  // middle
      {
        return(2);
      }
      else if( x > 236 )  // right
      {
      return(1);
      }
    }
  }
  return(old);    // nothing pressed

}

// -------------------------------------------
// Show the current frequency on the display
// -------------------------------------------
void show_freq()
{
  char buffer[20];

  tft.setTextColor(TFT_CYAN,TFT_BLACK);

  edit_frequ = digits2edit(edit_frequ); 

  if(edit_frequ > 0) {     // Edit mode
    f = radio.getFreqMode()*10;
    long backup_frequ=f;    // backup current frequence
    long fneu=0;
    long faux = f;
    long fold=0;
  
    TFT_eSPI_Button exit_button[2];
    tft.setTextFont(2);
    tft.fillRect(0,190,320,50,TFT_BLACK);
    exit_button[0].initButton(&tft, 235, 210, 100, 40, TFT_SILVER, 
                              TFT_DARKSILVER, TFT_WHITE, "CANCEL", KEY_TEXTSIZE);
    exit_button[0].drawButton();
    exit_button[1].initButton(&tft, 85, 210, 100, 40, TFT_SILVER, 
                              TFT_DARKSILVER, TFT_WHITE, "OK", KEY_TEXTSIZE);          
    exit_button[1].drawButton();

    uint32_t dialspeed = millis();

    tft.setFreeFont(&FreeSans18pt7b);

    do {

      // Check whether "OK" or "Cancel" was pressed: 
      uint8_t pressed = check_touch();

      if (pressed && exit_button[0].contains(x, y)) {     // Cancel button
        exit_button[0].press(true);  // tell the button it is pressed
        radio.setFreq(backup_frequ/10);  // restore old value
        tft.fillScreen(TFT_BLACK);
        drawKeypad();
        drawSMeter();
        exit_button[0].press(false);  // tell the button it is NOT pressed
        edit_frequ = 0;
        refresh_required = true;
        return;
      }
      else if (pressed && exit_button[1].contains(x, y)) {  // Ok button
        exit_button[1].press(true);  // tell the button it is pressed
        radio.setFreq(faux/10);
        tft.fillScreen(TFT_BLACK);
        drawKeypad();
        drawSMeter();
        exit_button[1].press(false);  // tell the button it is NOT pressed
        edit_frequ = 0;
        refresh_required = true;
        return;
      }

      fneu = radio.getFreqMode()*10;

      uint8_t old_digits = edit_frequ;
      edit_frequ = digits2edit(edit_frequ); 

      if(fneu != fold || edit_frequ != old_digits) {
        long tick;

        old_digits = edit_frequ;

        if (fneu-fold > 50)  tick=1;
        else if (fneu-fold < -50)  tick=-1;
        else tick=0;
   
        // speed up if knob is turned faster:
        if(tick != 0)
          Serial.println( (millis() - dialspeed) );
        if ( (millis() - dialspeed) < 100 ) {
          tick  = 10 * tick;
          dialspeed = millis();
        }

        if(tick != 0) {
          dialspeed = millis();
        }
  
        fold=fneu;

        if( edit_frequ == 2) 
          tick = tick * 1000;
        else if( edit_frequ == 3)
          tick = tick * 1000000;

        faux = faux + tick;

        // Check if new frequency vale is within valid range:
        if( faux < 100000)                          faux=470000000;
        else if (faux>56000000  && faux<67000000)   faux=76000000;
        else if (faux<76000000  && faux>67000000)   faux=56000000;
        else if (faux>154000000 && faux<280000000)  faux=420000000;
        else if(faux<420000000  && faux>280000000)  faux=154000000;
        else if (faux>470000000)                    faux=100000;
      
        tick=0;

        sprintf(buffer,"%ld",faux);

        int len = strlen(buffer);
        int x=270; int y=128;
        //  Delete old digits:
        tft.fillRect(100,130,210,28,TFT_BLACK);
        int i=1;
        int part=1;

        while(len>0) {
          if(i>3 && len>0) {
            x = x - tft.drawChar('.', x+18,y+28,GFXFF);
            part++;
            i=1;
          }
          else {
            if(part==edit_frequ)
              tft.setTextColor(TFT_RED,TFT_BLACK);    
            x = x - tft.drawChar(buffer[len-1], x+8,y+28,GFXFF);
            tft.setTextColor(TFT_CYAN,TFT_BLACK);    
            len--;
            i++;
          }
        }
      } // end of 'if' loop
    } while (edit_frequ > 0);

  }
  else {
    tft.setFreeFont(&FreeSans18pt7b);
    f = radio.getFreqMode()*10;

    if( (f != old_freq) || refresh_required) {
      refresh_required = false;
      updateTime = millis();

      sprintf(buffer,"%ld",f);

      int len = strlen(buffer);
      int x=270; int y=128;
      //  Delete old digits:
      tft.fillRect(100,130,210,28,TFT_BLACK);
      int i=1;
      int part=1;

      while(len>0) {
        if(i>3 && len>0) {
          x = x - tft.drawChar('.', x+18,y+28,GFXFF);
          part++;
          i=1;
        }
        else {
          if(part==edit_frequ)
            tft.setTextColor(TFT_RED,TFT_BLACK);

        if( f < hambands[current_band][0] || f > hambands[current_band][1] )
          tft.setTextColor(TFT_RED,TFT_BLACK);

          x = x - tft.drawChar(buffer[len-1], x+8,y+28,GFXFF);

          tft.setTextColor(TFT_CYAN,TFT_BLACK);    
          len--;
          i++;
        }
      }
      old_freq = f;
    }
  }
}


void antenna_setup() 
{
  #define  BASEX 5
  #define  BASEY 5
  #define  WIDTH 310
  #define  HEIGHT 230

  bool changed = false;

  TFT_eSPI_Button ant_button[8];

tft.setFreeFont(&FreeSans9pt7b);

  tft.setTextColor(TFT_SILVER,TFT_SILVER);

  tft.fillRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_BLACK);
  tft.drawRect(BASEX, BASEY, WIDTH, HEIGHT, TFT_BLACK);
  tft.drawRect(BASEX+1, BASEY-1, WIDTH-2, HEIGHT-2, TFT_BLACK);

  tft.drawString("HF", BASEX+50, BASEY+12, GFXFF);
  tft.drawString("6m", BASEX+50, BASEY+47, GFXFF);
  tft.drawString("Radio", BASEX+50, BASEY+82, GFXFF);
  tft.drawString("Air", BASEX+50, BASEY+117, GFXFF);
  tft.drawString("2m", BASEX+50, BASEY+152, GFXFF);
  tft.drawString("70cm", BASEX+50, BASEY+187,GFXFF);

  tft.setTextFont(2);
  byte ant = radio.getAnt();    // EEPROM read

  if (radio.eepromValidData)
  {
    for(int i=0;i<6;i++) {
      ant_button[i].setLabelDatum(0,4,MC_DATUM);

      if( bitRead(ant,i) ) {
        ant_button[i].initButton(&tft, BASEX+130,BASEY+22+i*35, 50, 28, TFT_BLACK, 
                              TFT_DARKGREEN, TFT_WHITE, "REAR", KEY_TEXTSIZE);
      } else {
        ant_button[i].initButton(&tft, BASEX+130,BASEY+22+i*35, 50, 28, TFT_BLACK, 
                              TFT_DARKSILVER, TFT_WHITE, "FRONT", KEY_TEXTSIZE);
      }
      ant_button[i].drawButton();
    }
  }

  ant_button[6].initButton(&tft, 245, 200, 100, 40, TFT_SILVER, 
                              TFT_DARKSILVER, TFT_WHITE, "CANCEL", KEY_TEXTSIZE);          
  ant_button[6].drawButton();
  ant_button[7].initButton(&tft, 245, 120, 100, 40, TFT_SILVER, 
                              TFT_DARKSILVER, TFT_WHITE, "OK", KEY_TEXTSIZE);          
  ant_button[7].drawButton();

  while(1) {
    uint8_t pressed = check_touch();

    for (uint8_t i=0; i<6; i++) {
      if ( pressed && ant_button[i].contains(x, y) ) {
        changed = true;
        if( bitRead(ant,i) ) {
          ant = bitClear(ant,i);
          ant_button[i].initButton(&tft, BASEX+130,BASEY+22+i*35, 50, 28, TFT_BLACK, 
                              TFT_DARKSILVER, TFT_WHITE, "FRONT", KEY_TEXTSIZE);
        } else {
          ant = bitSet(ant,i);
          ant_button[i].initButton(&tft, BASEX+130,BASEY+22+i*35, 50, 28, TFT_BLACK, 
                              TFT_DARKGREEN, TFT_WHITE, "REAR", KEY_TEXTSIZE);
        }
        ant_button[i].drawButton(); 
        delay(500); // debounce
      }
    }

    if (pressed && ant_button[6].contains(x, y)) {      // CANCEL button
      ant_button[6].press(true);  // tell the button it is pressed
      tft.fillScreen(TFT_BLACK);
      refresh_required = true;  // refresh frequency display
      show_freq();
      drawKeypad();
      drawSMeter();
      return;
    }

    else if (pressed && ant_button[7].contains(x, y)) {   // OK button
      ant_button[7].press(true);  // tell the button it is pressed

      // only write antenna config to EEPROM, if something was changed;
      if(changed)
        radio.setAnt(ant);

      tft.fillScreen(TFT_BLACK);
      refresh_required = true;  // refresh frequency display
      show_freq();
      drawKeypad();
      drawSMeter();
      return;
    } else {
      ant_button[6].press(false);  // tell the button it is NOT pressed
      ant_button[7].press(false); 
    }
  }

}

//---------------------------------------------------
//  check which button was pressed on the main page
//---------------------------------------------------
uint8_t check_menu_buttons()
{
  uint8_t pressed = check_touch();

 	uint8_t b = 0;

 	if ( pressed && (x > 14) && (x < 50) && (y > 129) && (y < 167) )
    return(100);	//locked pressed

	for (uint8_t col = 0; col < NUM_OF_KEYS; col++) {
	  b = col + (menu_page-1)*NUM_OF_KEYS;

	  if (pressed && key[b].contains(x, y))
    {
//      delay(50);  // debounce
		  return (b); // menu button pressed
    }
	}
  return -1;   // no button pressed

}

//------------------------------------
void button_events(uint8_t b) {

  tft.setFreeFont(LABEL1_FONT);

  if(b>=0 && b < NUM_OF_KEYS*MAX_MENU_PAGES) {
    key[b].initButton(&tft, KEY_X + (b-NUM_OF_KEYS*((menu_page-1))) * (KEY_W + KEY_SPACING_X),
        KEY_Y, // x, y, w, h, outline, fill, text
        KEY_W, KEY_H, TFT_WHITE, TFT_SILVER, TFT_WHITE,
        keyLabel[b], KEY_TEXTSIZE);
        key[b].drawButton();
  }

  if(b==5 + NUM_OF_KEYS*((menu_page-1)))
  {
    menu_page++;
    if(menu_page > MAX_MENU_PAGES)
      menu_page=1;
    delay(500);
    drawKeypad();
  }
  
  else if(b == 0)   // Band key
  {
    uint8_t band;
    band = band_select();
    if( band != current_band) {  // write only to EEPROM if band changed
      Serial.println("EEPROM WRITE: setBandVFO())");
      radio.setBandVFO(current_vfo, band);
    }
  }

  else if(b == 1)   // mode key
  {
    uint8_t mode;
    mode = mode_select();

    if( mode != current_mode && mode != 5) {  // write only to EEPROM if mode changed
                                              // and mode is not 5 (WFM)
      Serial.println("EEPROM WRITE: setMode())");
      radio.setMode(mod_codes[mode]);
    }
  }

  else if(b == 2)   // IPO key
  {
    Serial.println("EEPROM WRITE: toggleIPO()");
    radio.toggleIPO();
    bool ipo = radio.getIPO();
    if(ipo)
      keyStatus[2]=true;
    else
      keyStatus[2]=false;
  }
    
  else if(b == 3)   // ATT key
  {
    Serial.println("EEPROM WRITE: toggleATT()");
    radio.toggleATT();
    bool att = radio.getATT();

    if(att)
      keyStatus[3]=true;
    else
      keyStatus[3]=false;
  }

  else if(b == 4) {  // TUN key

    delay(500);
    update_menu(4);

    byte old_pwr;
    byte old_mode;

    if( keyStatus[4] ) {

      if(old_mode != 12) 
        radio.setMode(old_mode);   // restore previous mode
      if(old_pwr != TUNE_PWR) 
        radio.setPWR(old_mode);    // restore previous power value

      radio.PTT(false);
      keyStatus[4]=false;
      update_menu(4);
    }
    else {
      old_pwr = current_pwr;
      old_mode = current_mode;

      if(old_mode != 12) 
        radio.setMode(12);   // set mode to PKT, if not already selected
       if(old_pwr != TUNE_PWR) 
        radio.setPWR(TUNE_PWR);   // set power to tuning power, if not already selected     

      byte res = radio.PTT(true);

      if( res == 0xF0 ) {           // could not enable TX
        if(old_mode != 12) 
          radio.setMode(mod_codes[old_mode]);   // restore previous mode
        if(old_pwr != TUNE_PWR) 
          radio.setPWR(old_pwr);    // restore previous power value
      }
      else {                         // TX switched on
        keyStatus[4]=true;
        update_menu(4);
      }
    }

  }

  else if(b == 6)   // VFO key
  {
    radio.toggleVFO();
    get_vfo();

    update_menu(6);
  }

  else if(b == 7)   // NB key
  {
    Serial.println("EEPROM WRITE: toggleNB()");
    radio.toggleNB();
    bool nb = radio.getNB();
    if(nb)
      keyStatus[7]=true;
    else
      keyStatus[7]=false;
  }

  else if(b == 8) {  // TX Power key
      Serial.println("EEPROM WRITE: setPWR()");
    current_pwr--;
    if(current_pwr < 0) current_pwr=3;
    radio.setPWR(current_pwr);

  }

  else if(b == 9)   // antenna key
    antenna_setup(); 

  else if(b == 10) { // AGC key
    Serial.println("EEPROM WRITE: setAGC()");
    current_agc++;
    if(current_agc > 3) current_agc=0;
    radio.setAGC(current_agc);
  }

  else if(b == 12) {  // STP key

    if(current_mode == 4) {   // AM mode
      current_am_step++;
      if(current_am_step > 5) current_am_step = 0;
    }
    else if(current_mode == 5 || current_mode == 6 || current_mode == 8 ) {   // FM mode
      current_fm_step++;
      if(current_fm_step > 7) current_fm_step = 0;
    }
    else {
      current_ssb_step++;     // SSB mode
      if(current_ssb_step > 2) current_ssb_step = 0;
    }
    radio.setSteps(current_ssb_step << 6 | current_am_step << 3 | current_fm_step);

  }

  else if(b == 13) {  // CHG key
    Serial.println("EEPROM WRITE: setChargeTime()");
    current_chg++;
    if(current_chg > 2) current_chg=0;
    radio.setChargeTime(current_chg);
    update_menu(13);
  }

  else if(b == 14) {  // Charging enable key
    Serial.println("EEPROM WRITE: enableCharging()");

    //charging = radio.get_CHG_Status();

    if(charging) {
      radio.enableCharging(false);
      keyStatus[14]=false;
    }
    else {
      radio.enableCharging(true);
      keyStatus[14]=true;
    }

    update_menu(14);
  }

  else if(b == 15) { // dummy key '4'
    delay(500);
    keyStatus[15]=false;
    update_menu(15);
  }

  else if(b == 16) {  // dummy key '5'
    delay(500);
    keyStatus[16]=false;
    update_menu(16);
  }

  else if(b==100)
  {
    tft.drawRect(10,131,45,31,TFT_SILVER);
    tft.drawRect(11,132,43,29,TFT_SILVER);

    if(locked)
      radio.lock(false);
    else
      radio.lock(true);
  }

}


// ------------------------------
//    update menu button
// ------------------------------
void update_menu(uint8_t b) {

  uint8_t col = b % NUM_OF_KEYS;

// if button is currently not shown in menu, skip. Otherwise changes would
// apply to the button of another menu page at same column
  if( (int) (b/NUM_OF_KEYS + 1)  != menu_page) return;

  tft.setFreeFont(LABEL1_FONT);

  if(keyStatus[b])
    key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
      KEY_Y, KEY_W, KEY_H, TFT_WHITE, keyColor_active[b], TFT_WHITE,
      keyLabel[b], KEY_TEXTSIZE);
  else
    key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
      KEY_Y, KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
      keyLabel[b], KEY_TEXTSIZE);

  key[b].drawButton();

}


void sec2time(char *buf, uint32_t seconds)
{

  ldiv_t res;
  int secs, minutes, hours;

  res = ldiv( seconds, 60 );
  secs = res.rem;


/*
  res = ldiv( res.quot, 60 );
  minutes = res.rem;
  hours = res.quot;
*/
  sprintf(buf, "%02d:%02d", res.quot, secs);

  return;

}