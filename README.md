# FT-817-Touch Display

'D1 mini ESP32 controller together with a 3.2 TFT display ILI9341 to control a Yaesu FT-818 or (with some small changes) the FT-817. The code is far away from optimized. Nevertheless I make it public to give others the chance to use or improve the code.

The CAT control part of the software is based on the excellent FT-817 library from Pavel Milanes (https://github.com/stdevPavelmc/ft817), which I did extend to add new functions.
Thanks to Clint Turner (KA7OEI) for his huge information about controlling the FT-817 (http://www.ka7oei.com/ft817_meow.html)

Some background information for controlling the FT-817/FT-818:
The FT-817/818 is not notifying a connected controller in case a user changes its settings, frequency etc. Instead, a CAT program is has to poll the TRX in a loop for each setting it would like to know. As each request towards the TRX take a relevant time, the time to step through the loop increases if the number of requested settings increases. To ensure, that the frequency on the TFT display is following quickly the changes on the TRX, the frequency in the loop is much more often requested than the other settings.

Warning:
Changes of settings across the CAT interface are done in most cases by writing to the EEPROM of the FT-817/818. As described by Clint Turner s. link above), this could harm the transceiver, can delete the factory settings and can lead to a defect device. But even if the write commands are sent in the right format, accidently writing to the EEPROM frequently (e.g. in a loop) can destroy the EEPROM after a certain (big) number of cycles. The minimum action you should take is to backup the "soft calibration" settings BEFORE you apply this software! Please read the details on Clint's web page! In the provided software EEprom write is disabled, but can be enabled in ft817.h (comment out "#define NO_EEPROM_WRITE").

Please note that the software development is not completed currently (and will probbably never be), because of lack of time. Thus, take it as is and improve it as you like. The next step which I would have to take is to redesign and optimize the code - it's currently not "beautiful" ;-)

Pin assignment:


