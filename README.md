# FT-817-Touch Display

'D1 mini ESP32 controller together with a 3.2 TFT display ILI9341 to control a Yaesu FT-818 or (with some small changes) the FT-817. The code is far away from optimized. Nevertheless I make it public to give others the chance to use or improve the code.

The CAT control part of the software is based on the excellent FT-817 library from Pavel Milanes (https://github.com/stdevPavelmc/ft817), which I did extend to add new functions.
Thanks to Clint Turner (KA7OEI) for his huge information about controlling the FT-817 (http://www.ka7oei.com/ft817_meow.html)

Some background information for controlling the FT-817/FT-818:
The FT-817/818 is not notifying a connected controller in case a user changes its settings, frequency etc. Instead, a CAT program is has to poll the TRX in a loop for each setting it would like to know. As each request towards the TRX take a relevant time, the time to step through the loop increases if the number of requested settings increases. To ensure, that the frequency on the TFT display is following quickly the changes on the TRX, the frequency in the loop is much more often requested than the other settings.

<b>Warning</b><br>
Changes of settings across the CAT interface are done in most cases by writing to the EEPROM of the FT-817/818. As described by Clint Turner (s. link above), this could harm the transceiver, can delete the factory settings and can lead to a defect device. But even if the write commands are sent in the right format, accidently writing to the EEPROM frequently (e.g. in a loop) can destroy the EEPROM after a certain (big) number of cycles. The minimum action you should take is to backup the "soft calibration" settings BEFORE you apply this software! Please read the details on Clint's web page! In the provided software EEprom write is disabled, but can be enabled in ft817.h (comment out "#define NO_EEPROM_WRITE").

Please note that the software development is not completed currently (and will probably never be), because of lack of time. Thus, take it as is and improve it as you like. The next step which I would have to take is to redesign and optimize the code - it's currently not "beautiful" ;-)

<b>Compile the software for the FT-818</b><br>
I used PlatformIO instead of Arduino IDE, but the last one should work as well after manual import of the libraries.<br><br>
(1) Download the code from github as a zip file<br>
(2) Put it into the PlatformIO project folder and extract the files<br>
(3) Open it in PlatformIO ("Open Project")<br>
(4) "Build" or "Upload"<br>

Please note: Do NOT install the initial software, if you would like to connect the ESP32 via CAT interface to the FT-81<b>7</b>. I do not have a FT-817 and did not test it. The memory layout of the FT-817 is not completely the same as of the FT-818, meaning that it could lead to trouble!<br><br>
<b>Adapt the software to support the FT-817 instead of the FT-818 (UNTESTED!)</b><br>
Open the file globals.h in the src folder and search for "#define FT818". Comment this line out (deactivate it). This will change the calculation of the base address (s. ft817.cpp, around line 930). But as I said - do it only on your risk!<br>
There might be other differences, e.g. reagrding the result codes of the command "Read Transmitter Status". I you recognize that during TX the bars PWR, SWR, MOD and ALC are reversed, check main.cpp, line 130, where "show_bar()" is called.<br><br>
<b>Enable MQTT:</b><br>
Wifi and MQTT is disabled by default. It can be activtated in globals.h. Look for #define MQTT and activate this line. In this case you also have to add your Wifi secrets and the mqtt broker details in mqtt.cpp.
<br><br>
<b>Limitations</b><br>
(1) The sofware does not check, whether the TRX is connected<br>
(2) If the application does not get a Wifi connection or cannot connect to a MQTT broker, the application reboots (if MQTT is enabled in globals.h)<br>
(3) Some presented values in the area of the frequency digits are not completely deleted or not completely shown. Here the deleetion of old values needs optimization<br>
(4) I found that using the 2m antenna stick directly on the phone leads to reboots on 2m band when transmitting. This is not a software issue, but one has to take care to e.g block hf on the connections between TRX and ESP32.<br>
(5) I did not really test the S-meter presentation. May be it's not following the real signal values quickly enough. In any case the shown values are not precise. I used an example out of the original TFT_eSPI library and changed it to present the S-meter scale - should be replaced with a better one.<br><br>

<b>Operation</b><br>
Do to the polling of the settings as described before, touching the soft keys cannot not lead to immediate reactions if the application is just busy to process a request to the TRX and to wait for the reply. So it is needed to press the keys longer until the keys shortly are highlighted.

<br><br>

<b>Pin assignment</b><br>
![Screenshot](pins.png)
<br><br>
<b>Enclosure:</b><br>
I created a 3D case for the ESP32 and the display, available on thingiverse: https://www.thingiverse.com/thing:4938484
<br><br>
<b>Demonstration on Youtube</b><br>
https://www.youtube.com/watch?v=2vLFegkDQvQ



