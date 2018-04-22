# Esplora API
***
Developed only for *Arduino Esplora*!


#### Describtion:
API created for *Arduino Esplora* board.
Created to reduce overhead of standart *Arduino library* and get to hardware as close
as possible to archive maximum perfomance at low cost.
Also include *Game API* to write simple games.

#### Installation:
Place *esploraAPI* folder to *Arduino's* libs library:
- OSX: ~/Documents/Arduino/libraries/
- WIN: My Documents\Arduino\libraries\

#### Esplora API support:
- RGB LED manipulation;
- GFX for st7735 tft display;
- poll Buttons;
- read Light sensor;
- read Temp;
- read Slider;
- read Analoge joystick;
- read Accelerometr;
- read Microphone;
- set F_CPU frequency;
- get system uptime (system tick timer based on timer0);
- a few memory manipulation functions insted heavy <stdio.h> one;
- *~~generate annoying sounds via buzzer~~*;

#### Game API supports:
- sprites: draw, collisions;
- fixed palette of 80 colors (76 actually);
- picoPaker API to uncompress pictures;
- simple task manager for pseudo multitasking;
- fast and low cost random function;
- sound engine grabbed from Gamebuino Library (http://gamebuino.com) (Huge thx to them!);
- tiny font only in ASCII range 32-90 to make retro effect and reduce consumed ROM;
- a few GFX help over Esplora API;

#### Missed features:
- SD card support;
- *Arduino library* support;
- pwm write;
- low case letters in font(only upper case) as side effect of tiny font;

#### Hardware:
- *Arduino Esplora* board;
- MCU: *ATmega32u4*;
- ROM: 28,672 bytes;
- RAM: 2,560 bytes;
- TFT LCD: *ST7735* 160x128.

> ## :exclamation: ATTENTION! :exclamation:
>  * This API is still unstable and in develop! :beetle:
>  * Any changes are possible at any time!  
