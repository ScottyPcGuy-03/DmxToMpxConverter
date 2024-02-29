This project modifies the DMX to MPX Arduino converter by Dennis Engdahl: https://www.snowcrest.net/engdahl/DMX.html

The above project makes use of one of the onboard ATMega328 timer interrupts to generate the PWM for the Microplex signal.

This project extends that functionality by adding a touchscreen to allow changing the start & end DMX / Microplex channels to allow for a more flexible configuration without manually reprogramming the Arduino.
The translation settings are stored in a struct which is read from the EEPROM on startup. Settings updates require the device to be restarted to properly initialize the DMX class, timers, and I/O. This may change in a future release.

The following Arduino libraries are included:
1) Conceptinetics DMX shield (Not available from the Library manager)
2) EEPROM library
3) Adafruit GFX Library
4) Adafruit ILI9341 Library
5) XPT2046_TouchScreen_TT library
6) Four fonts (3 custom):
7) Fonts/FreeSans9pt7b.h
8) Fonts/FreeSans18pt7bModified.h
9) Fonts/lock_icon.h
10) Fonts/Monospaced_plain_72.h

IMPORTANT: To use this project with the Arduino 2.x editor, BEFORE installing the editor, clone the repo DIRECTLY in your ~\Documents\Arduino\ folder like so (note the dot at the end of the git clone to force git to unpack in the local dir):
cd %userprofile%\Documents\
mkdir Arduino
cd Arduino
git clone https://github.com/ScottyPcGuy-03/DmxToMpxConverter.git .

Now install the Arduino IDE and you should be able to load the project.
