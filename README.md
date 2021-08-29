# FT-817-TouchDisplay

Using a 'D1 mini ESP32 controller together with a 3.2 TFT display ILI9341 to control a Yaesu FT-818 or (with some small changes) the FT-817. The code is far away from optimized. Nevertheless I make it public to give others the chance to use or improve the code.

The CAT control part of the software is based on the excellent FT-817 library from Pavel Milanes (https://github.com/stdevPavelmc/ft817), which I did extend to add new functions.
Thanks to Clint Turner (KA7OEI) for his huge information about controlling the FT-817 (http://www.ka7oei.com/ft817_meow.html)

Basics for controlling the FT-817/FT-818:
The TRX is not notifying a connected controller in case a user changes settings, frequency etc. by itself. Instead, a CAT program is has to poll the TRX in a loop for each setting it would like to know. As each request towards the TRX take a relevant time, the time to step through the loop increases if the number of requested settings increases. To ensure, that the frequency on the TFT display is following quickly the changes on the TRX, the frequency in the loop is frequency is much more often requested as the other settings.
