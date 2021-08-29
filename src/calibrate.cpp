#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

extern TFT_eSPI tft; 
extern XPT2046_Touchscreen ts;



void calibrate_touch() 
{
  long x1=0,y1=0,count=0;

  tft.fillScreen(TFT_BLACK);
  tft.drawRect(10,10,300,220,TFT_RED);

  tft.drawCircle(10,10,5,TFT_RED);


  while(1) 
  {
	  ulong starttime = millis();
    while(ts.touched())
    {
      	if(millis()-starttime > 300)
        {
          TS_Point p = ts.getPoint(); 
          count++;
          x1=x1+p.x;
          y1=y1+p.y;

          //Serial.print("x=  ");Serial.print(p.x);Serial.print(", x_avg=  ");Serial.print(x1/count);
          //Serial.print(", y=  ");Serial.print(p.y);Serial.print(", y_avg=  ");Serial.println(y1/count);
          break;
        }
    }


  }

  return;
}

/*
if(ts.touched() ) {
  TS_Point p = ts.getPoint(); 
 
  x = map(p.x, TS_MAXX, TS_MINX, 10, 300);
  y = map(p.y, TS_MAXY, TS_MINY, 10, 220);

  tft.drawCircle(x,y,5,TFT_CYAN);

}
*/
/*
Serial.print("X MIN:  ");Serial.print(10);Serial.print("  ");Serial.println(p.x);
Serial.print("Y MIN:  ");Serial.print(10);Serial.print("  ");Serial.println(p.y);
Serial.print("X MAX:  ");Serial.print(310);Serial.print("  ");Serial.println(p.x);
Serial.print("Y MAX:  ");Serial.print(230);Serial.print("  ");Serial.println(p.y);
*/

