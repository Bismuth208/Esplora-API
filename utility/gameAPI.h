/*
 * Author: Antonov Alexandr (Bismuth208)
 * e-mail: bismuth20883@gmail.com
 * 
 *  THIS PROJECT IS PROVIDED FOR EDUCATION/HOBBY USE ONLY
 *  NO PROTION OF THIS WORK CAN BE USED IN COMMERIAL
 *  APPLICATION WITHOUT WRITTEN PERMISSION FROM THE AUTHOR
 *  
 *  EVERYONE IS FREE TO POST/PUBLISH THIS ARTICLE IN
 *  PRINTED OR ELECTRONIC FORM IN FREE/PAID WEBSITES/MAGAZINES/BOOKS
 *  IF PROPER CREDIT TO ORIGINAL AUTHOR IS MENTIONED WITH LINKS TO
 *  ORIGINAL ARTICLE
 */

#ifndef _GAMEAPI_H
#define _GAMEAPI_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#ifdef __cplusplus
extern "C"{
#endif

//---------------------------------------------------------------------------//
// this macros remove monstro constructions...
#define getConstCharPtr(a, b) (const uint8_t*)pgm_read_word(&(a[b]))
#define getConstWordPtr(a, b) (const uint16_t*)pgm_read_word(&(a[b]))

#define getPicByte(a)     pgm_read_byte(a)
#define getPicWord(a, b)  pgm_read_word(&(a[b]))
#define getPicSize(a, b)  wordData_t{.wData = getPicWord(a, b)}
#define getPicPtr(a, b)   getConstCharPtr(a, b)

//---------------------------------------------------------------------------//

// adrr range used: 0x00 - 0x??
#define EE_ADDR_SAVE_DATA  0x00

#define getSaveData(addr, val, type)  eeprom_read_block((void*)val, (void*)addr, sizeof(type))
#define setSaveData(addr, val, type)  eeprom_update_block((void*)val, (void*)addr, sizeof(type))

//---------------------------------------------------------------------------//

#define PALETTE_SIZE 160

#define COLOR_ID_BLACK   0x0E
#define COLOR_ID_WHITE   0x4D
#define COLOR_ID_RED     0x15
#define COLOR_ID_GREEN   0x1A
#define COLOR_ID_YELLOW  0x38
#define COLOR_ID_GRAY    0x0D

// ------------------------------------ //

typedef const uint8_t pic_t;  // Yes, they are same, but lets 
typedef const uint8_t text_t; // separeate them for easy life...
// ------------------------------------ //

typedef struct { // 2 bytes
  uint8_t x;
  uint8_t y;
} position_t;

typedef struct {  // 4 bytes
  position_t Old;
  position_t New;
} objPosition_t;   // need for collision check

typedef struct {  // 6 bytes
  objPosition_t pos;
  pic_t *pPic;
} sprite_t;

typedef struct {  // 7 bytes
  position_t P0;
  position_t P1;
  position_t P2;
  uint8_t totalSteps;
} bezier_t;

typedef struct { // 2 bytes
  uint8_t id;
  uint8_t step;
} bezierLine_t;

typedef struct {  // 1 byte
  union {
    uint8_t zBtn; // for fast reset only
    struct {
      uint8_t aBtn :1;
      uint8_t bBtn :1;
      uint8_t xBtn :1;
      uint8_t yBtn :1;
      uint8_t freeRam :4;
    };
  };
} btnStatus_t;

typedef union { // 2 bytes
  uint16_t wData;
  struct {
    uint8_t u8Data1;
    uint8_t u8Data2;
  };
} wordData_t;

//---------------------------------------------------------------------------//

extern uint16_t palette_RAM[];

//---------------------------------------------------------------------------//
void initPalette(void);
void setAlphaReplaceColorId(uint8_t colorId);
uint8_t getAlphaReplaceColorId(void);
uint16_t getPlatetteColor(uint8_t id);

void drawText(uint8_t posX, uint8_t posY, uint8_t textSize, text_t *pText);
void drawFrame(uint8_t posX, uint8_t posY, uint8_t w, uint8_t h, uint16_t clr1, uint16_t clr2);

void screenSliderEffect(uint8_t colorId);
void removePicFast(position_t *pPos, pic_t *pPic);
void drawPixelFast(position_t *pPos, uint8_t colorId);
void printDutyDebug(uint32_t duration);

bool checkSpriteCollision(sprite_t *pSprOne, sprite_t *pSprTwo);
void updateSprite(sprite_t *pSprite);
void drawSprite(sprite_t *pSprite);
void moveSprite(sprite_t *pSprite);
void removeSprite(sprite_t *pSprite);

//---------------------------------------------------------------------------//

#ifdef __cplusplus
} // extern "C"
#endif

#endif /*_GAMEAPI_H*/
