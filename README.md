# DMX To MPX Converter
This project modifies the DMX to MPX Arduino converter by Dennis Engdahl: https://www.snowcrest.net/engdahl/DMX.html

The above project makes use of one of the onboard ATMega328 timer interrupts to generate the PWM for the Microplex signal.

This project extends that functionality by adding a touchscreen to allow changing the start & end DMX / Microplex channels to allow for a more flexible configuration without manually reprogramming the Arduino.
The translation settings are stored in a struct which is read from the EEPROM on startup. Settings updates require the device to be restarted to properly initialize the DMX class, timers, and I/O. This may change in a future release.

# Libraries
The following Arduino libraries are included:
- Conceptinetics DMX shield (Not available from the Library manager)
- EEPROM library
- Adafruit GFX Library
- Adafruit ILI9341 Library
- XPT2046_TouchScreen_TT library
- Four fonts (3 custom):
  - Fonts/FreeSans9pt7b.h
  - Fonts/FreeSans18pt7bModified.h
  - Fonts/lock_icon.h
  - Fonts/Monospaced_plain_72.h

## Project folder structure
This repo contains everything you need to compile the project. It should be extracted directly into the Documents\Arduino folder so that the libraries are properly included.

**IMPORTANT:** To use this project with the Arduino 2.x editor, it is easiest to clone this repo DIRECTLY in your ~\Documents\Arduino\ folder BEFORE installing the editor. If you install the editor first, you will need to copy the repo files manually into `%userprofile%\Documents\Arduino`. To clone directly:

`cd %userprofile%\Documents\`  
`mkdir Arduino`  
`cd Arduino`  
`git clone https://github.com/ScottyPcGuy-03/DmxToMpxConverter.git .`

Now install the Arduino IDE and you should be able to load the project.
