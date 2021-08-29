#include <SPI.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include "ft817.h"
#define TFT_DARKERGREY 0x5AEB


extern TFT_eSPI tft;
extern FT817 radio;

// Define meter size as 1 for tft.rotation(0) or 1.3333 for tft.rotation(1)
#define M_SIZE 0.65
#define TFT_GREY 0x5AEB
#define YOFFSET 6

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = M_SIZE*120, osy = M_SIZE*120; // Saved x & y coords

int old_analog =  -999; // Value last displayed

int old_value[6] = { -1, -1, -1, -1, -1, -1};

// #################

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{

  char buf[4];

   if(value<10)
    sprintf(buf,"S%d  ",value);
  else
    sprintf(buf,"S9+");

  tft.setTextColor(TFT_BLACK, TFT_BLACK);
  tft.drawString(buf, M_SIZE*20, M_SIZE*95+YOFFSET, 2);

  value = value*100/14;

  // Move the needle until new value reached
  do {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -142, -36); // Map value to angle

    // Calculate tip of needle coords
    float sx = cos(sdeg * 0.0174532925);  // 0.0174 = 2*PI/360
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(M_SIZE*(120 + 20 * ltx - 1), M_SIZE*(140 - 20)+YOFFSET, osx - 1, osy+YOFFSET, TFT_BLACK);
    tft.drawLine(M_SIZE*(120 + 20 * ltx), M_SIZE*(140 - 20)+YOFFSET, osx, osy+YOFFSET, TFT_BLACK);
    tft.drawLine(M_SIZE*(120 + 20 * ltx + 1), M_SIZE*(140 - 20)+YOFFSET, osx + 1, osy+YOFFSET, TFT_BLACK);

    // Re-plot text under needle
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("S", M_SIZE*120, M_SIZE*70+YOFFSET, 4);

    // Store new needle end coords for next erase
    ltx = tx;
    osx = M_SIZE*(sx * 105 + 120);
    osy = M_SIZE*(sy * 105 + 150);

    // Draw the needle in the new posiion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(M_SIZE*(120 + 20 * ltx - 1), M_SIZE*(140 - 20)+YOFFSET, osx - 1, osy+YOFFSET, TFT_RED);
    tft.drawLine(M_SIZE*(120 + 20 * ltx), M_SIZE*(140 - 20)+YOFFSET, osx, osy+YOFFSET, TFT_MAGENTA);
    tft.drawLine(M_SIZE*(120 + 20 * ltx + 1), M_SIZE*(140 - 20)+YOFFSET, osx + 1, osy+YOFFSET, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  } while (!(value == old_analog));
}   

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void drawSMeter()
{

  tft.setTextFont(1);

  tft.fillRect(5, 3+YOFFSET, M_SIZE*230, M_SIZE*119, TFT_BLACK);

  // tft.setTextColor(TFT_BLACK);  // Text colour
  tft.setTextColor(TFT_WHITE);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
   for (int i = -49; i < 50; i += 7) {
    // Long scale tick length
    int tl = 7;  //15

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 7 - 90) * 0.0174532925);
    float sy2 = sin((i + 7 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE*100 + tl) + M_SIZE*120;
    int y2 = sy2 * (M_SIZE*100 + tl) + M_SIZE*140;
    int x3 = sx2 * M_SIZE*100 + M_SIZE*120;
    int y3 = sy2 * M_SIZE*100 + M_SIZE*140;

    // Green zone limits
    if (i >= -49 && i < 14) {
      tft.fillTriangle(x0, y0+YOFFSET, x1, y1+YOFFSET, x2, y2+YOFFSET, TFT_GREEN);
      tft.fillTriangle(x1, y1+YOFFSET, x2, y2+YOFFSET, x3, y3+YOFFSET, TFT_GREEN);
    }

    // Orange zone limits
    if (i >= 14 && i < 50) {
      tft.fillTriangle(x0, y0+YOFFSET, x1, y1+YOFFSET, x2, y2+YOFFSET, TFT_ORANGE);
      tft.fillTriangle(x1, y1+YOFFSET, x2, y2+YOFFSET, x3, y3+YOFFSET, TFT_ORANGE);
    }

    // Short scale tick length
    if(i==-35 || i==-21 || i==7) tl=3;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*140;

    // Draw tick
    tft.drawLine(x0, y0+YOFFSET, x1, y1+YOFFSET, TFT_BLACK);

 // Check if labels should be drawn, with position tweaks
   if (i % 7 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl) + M_SIZE*140;

      switch (i / 7) {
        case -7: tft.drawCentreString("0", x0+6, y0-9+YOFFSET, 1); break;
        case -6: tft.drawCentreString("1", x0+5, y0 -9+YOFFSET, 1); break;
        case -4: tft.drawCentreString("3", x0+3, y0 -8+YOFFSET , 1); break;
        case -2: tft.drawCentreString("5", x0+2, y0 -9+YOFFSET, 1); break;
        case 0:  tft.drawCentreString("7", x0, y0 -10+YOFFSET, 1); break;
        case 2:  tft.drawCentreString("9", x0-1, y0 -11+YOFFSET, 1); break;
        case 4:  tft.drawCentreString("+20", x0+6, y0 -10+YOFFSET, 1); break;
        case 6:  tft.drawCentreString("+40", x0+4, y0 -5+YOFFSET , 1); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 7 - 90) * 0.0174532925);
    sy = sin((i + 7 - 90) * 0.0174532925);

    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0+YOFFSET, x1, y1+YOFFSET, TFT_BLACK);
  }

  tft.drawRect(5, 3+YOFFSET, M_SIZE*230, M_SIZE*119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
  
}

void show_bar(uint8_t value, byte type) {

  tft.setTextColor(TFT_CYAN,TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextDatum(TL_DATUM);

  if(value > 10) value=10;

  switch(type) {
    case 0:
    tft.drawString("PWR",172,12,2);
    
    for( uint8_t i=0;i<10;i++) 
        tft.fillRect(206+i*10,14,8,10,TFT_DARKERGREY);
    if(!value) return;

    for( uint8_t i=1;i<=value;i++) {
      uint16_t color;
      if(i<4) color=TFT_GREEN;
      else if(i<6) color=TFT_YELLOW;
      else if(i<8) color=TFT_YELLOW;
      else color=TFT_RED;
     tft.fillRect(196+i*10,14,8,10,color);
    }
    break;

    case 1:
    tft.drawString("SWR",172,32,2);
    for( uint8_t i=0;i<10;i++) 
      tft.fillRect(206+i*10,34,8,10,TFT_DARKERGREY);

    if(!value) return;

    for( uint8_t i=1;i<=value;i++) {
      uint16_t color;
      if(i<4) color=TFT_GREEN;
      else if(i<6) color=TFT_YELLOW;
      else if(i<8) color=TFT_YELLOW;
      else color=TFT_RED;
     tft.fillRect(196+i*10,34,8,10,color);
    }
    break;

    case 2:
    tft.drawString("ALC",172,52,2);
    for( uint8_t i=0;i<10;i++) 
      tft.fillRect(206+i*10,54,8,10,TFT_DARKERGREY);

    if(!value) return;

    for( uint8_t i=1;i<=value;i++) {
      uint16_t color;
      if(i<4) color=TFT_GREEN;
      else if(i<6) color=TFT_YELLOW;
      else if(i<8) color=TFT_YELLOW;
      else color=TFT_RED;
      tft.fillRect(196+i*10,54,8,10,color);
    }
    break;

    case 3:
    tft.drawString("MOD",172,72,2);
    for( uint8_t i=0;i<10;i++) 
      tft.fillRect(206+i*10,74,8,10,TFT_DARKERGREY);

    if(!value) return;

    for( uint8_t i=1;i<=value;i++) {
      uint16_t color;
      if(i<4) color=TFT_GREEN;
      else if(i<6) color=TFT_YELLOW;
      else if(i<8) color=TFT_YELLOW;
      else color=TFT_RED;
      tft.fillRect(196+i*10,74,8,10,color);
    }
    break;
  }

}