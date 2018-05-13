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

#include "gameAPI.h"
#include "picoUnpacker.h"
#include "gfx.h"
#include "st7735.h"
#include "spi.h"

#define TFT_W    160
#define TFT_H    128

//---------------------------------------------------------------------------//
/*
 * Palette based on typical old-scool 8bit graphics.
 * Consist of 80 colors (actually 75; 5 transparent colors 0x0F-0x4F).
 *
 * 8 Gray colors 0x0D-0x3D; 0x1E-0x4E
 *
 * 0x0E pure black.
 * 0x4D pure white.
 * Suggest to use 0x0F-0x4F section for transparent.
 *
 * Colors represented in RGB565 color space.
 *
 * Color adress: 0x00 - 0x4F
 */

// Size: 160 ( 80 * sizeof(uint16_t) )
const uint16_t palette_ext[] PROGMEM = {
  // 0x00-0x0F
  0x020C, 0x01F4, 0x0096, 0x4012, 0xA00B, 0xC005, 0xB820, 0x88A0,
  0x5960, 0x3220, 0x0240, 0x0226, 0x02CC, 0x8410, 0x0000, 0x0020,
  
  // 0x10-0x1F
  0x04D9, 0x03BF, 0x4ABF, 0x81BF, 0xC0B3, 0xE0E8, 0xF182, 0xD244,
  0xC300, 0x54C0, 0x0460, 0x04AB, 0x04D4, 0xC638, 0x2104, 0x0020,
  
  // 0x20-0x2F
  0x06FD, 0x059F, 0x44BD, 0x837F, 0xE23B, 0xF94D, 0xFB06, 0xF3E2,
  0xE5C3, 0x9EC1, 0x2668, 0x0E91, 0x06F9, 0xE71C, 0x31A6, 0x0020,
  
  // 0x30-0x3F
  0x07DF, 0x06FF, 0x751F, 0xAC1F, 0xEB1E, 0xFB33, 0xFCCE, 0xF589,
  0xFF04, 0xBF01, 0x2F86, 0x0F94, 0x07BB, 0xEF7D, 0x5AEB, 0x0020,
  
  // 0x40-0x4F
  0x9FFF, 0x875F, 0xA6BF, 0xDD5D, 0xFD5F, 0xFD56, 0xFE96, 0xFF34,
  0xFFB3, 0xD752, 0xA771, 0x8FB5, 0x9F9C, 0xFFFF, 0xDEFB, 0x0020,
};

uint16_t palette_RAM[PALETTE_SIZE>>1];
//---------------------------------------------------------------------------//

uint8_t alphaReplaceColorId = 0;

//---------------------------------------------------------------------------//
void drawText(uint8_t posX, uint8_t posY, uint8_t textSize, text_t *pText)
{
  tftSetTextSize(textSize);
  tftSetTextColor(COLOR_WHITE);
  tftPrintAt_P(posX, posY, (const char *)pText);
}

//---------------------------------------------------------------------------//

void drawFrame(uint8_t posX, uint8_t posY, uint8_t w, uint8_t h, uint16_t clr1, uint16_t clr2)
{
  tftFillRect(posX, posY, w, h, clr1);          // Frame 0
  tftDrawRect(posX+1, posY+1, w-2, h-2, clr2);  // Frame 1
}

// --------------------------------------------------------------- //

// hardware scrolling; blocks everything
void screenSliderEffect(uint8_t colorId)
{
  uint16_t color = palette_RAM[colorId];

  for(int16_t i = TFT_W; i >= 0; i--) {
    tftScrollSmooth(1, i-1, 2); // 2 - is delay in ms
    tftDrawFastVLine(TFT_W-i-1, 0, TFT_H, color);
  }
}

void removePicFast(position_t *pPos, pic_t *pPic)
{
  auto tmpData = getPicSize(pPic, 0);

  // +1 == convert to display addr size
  tftSetAddrWindow(pPos->x, pPos->y, pPos->x+tmpData.u8Data1, pPos->y+tmpData.u8Data2);
  uint16_t dataSize = (tmpData.u8Data1+1) * (tmpData.u8Data2+1);

  do {
#ifdef __AVR__  // really dirt trick... but... FOR THE PERFOMANCE!
    SPDR_t in = {.val = palette_RAM[alphaReplaceColorId]};
    SPDR_TX_WAIT;
    SPDR = in.msb;
    
    SPDR_TX_WAIT;
    SPDR = in.lsb;
#else
    pushColorFast(palette_RAM[alphaReplaceColorId]);
#endif
  } while(--dataSize);

#ifdef __AVR__ 
  SPDR_TX_WAIT;  // dummy wait to stable SPI
#endif
}

void drawPixelFast(position_t *pPos, uint8_t colorId)
{
  tftSetAddrPixel(pPos->x, pPos->y);
  pushColorFast(palette_RAM[colorId]);
}

void printDutyDebug(uint32_t duration)
{
  char buf[8];

  tftSetTextSize(1);
  tftFillRect(0, 0, 36, 7, palette_RAM[alphaReplaceColorId]);
  tftPrintAt(0, 0, itoa(duration, buf, 10));
}

// --------------------------------------------------------------- //

bool checkSpriteCollision(sprite_t *pSprOne, sprite_t *pSprTwo)
{
  auto tmpDataOne = getPicSize(pSprOne->pPic, 0);
  auto tmpDataTwo = getPicSize(pSprTwo->pPic, 0);

  /* ----------- Check X position ----------- */
  uint8_t objOnePosEndX = (pSprOne->pos.Old.x + tmpDataOne.u8Data1);

  if(objOnePosEndX >= pSprTwo->pos.Old.x) {
    uint8_t objTwoPosEndX = (pSprTwo->pos.Old.x + tmpDataTwo.u8Data1);
    if(pSprOne->pos.Old.x >= objTwoPosEndX) {
      return false; // nope, different X positions
    }
    // ok, objects on same X lines; Go next...
  } else {
    return false; // nope, absolutelly different X positions
  }

  /* ---------------------------------------- */
  /* ----------- Check Y position ----------- */
  uint8_t objOnePosEndY = (pSprOne->pos.Old.y + tmpDataOne.u8Data2);
  
  if(objOnePosEndY>= pSprTwo->pos.Old.y) {
    uint8_t objTwoPosEndY = (pSprTwo->pos.Old.y + tmpDataTwo.u8Data2);
    if(pSprOne->pos.Old.y <= objTwoPosEndY) {
      // ok, objects on same Y lines; Go next...
      // yep, if we are here
      // then, part of one object collide wthith another object
      return true;
    } else {
      return false; // nope, different Y positions
    }
  } else {
    return false; // nope, absolutelly different Y positions
  }
}

void updateSprite(sprite_t *pSprite)
{
  moveSprite(pSprite);
  drawSprite(pSprite);
}

void moveSprite(sprite_t *pSprite)
{
  //is position changed?
  if((pSprite->pos.Old.x != pSprite->pos.New.x) || (pSprite->pos.Old.y != pSprite->pos.New.y)) {
    removeSprite(pSprite); // clear previos position
    pSprite->pos.Old = pSprite->pos.New; // store new position
  }
}

void drawSprite(sprite_t *pSprite)
{
  drawPico_DIC_P(pSprite->pos.Old.x, pSprite->pos.Old.y, pSprite->pPic);
}

void removeSprite(sprite_t *pSprite)
{
  removePicFast(&pSprite->pos.Old, pSprite->pPic);
}

//---------------------------------------------------------------------------//
void getSaveData(uint8_t blockNum, void *blockPtr, size_t blockSize)
{
  uint16_t blockEEAddr = blockNum * blockSize;

  if(blockEEAddr < (EE_MAX_ADDR - blockSize)) {
    eeprom_read_block(blockPtr, (void*)blockEEAddr, blockSize);
  }
}

void setSaveData(uint8_t blockNum, void *blockPtr, size_t blockSize)
{
  uint16_t blockEEAddr = blockNum * blockSize;

  if(blockEEAddr < (EE_MAX_ADDR - blockSize)) {
    eeprom_update_block(blockPtr, (void*)blockEEAddr, blockSize);
  }
}

//---------------------------------------------------------------------------//
void initPalette(void)
{
  // place palette in RAM for faster access
  memcpy_P(&palette_RAM[0], palette_ext, PALETTE_SIZE);
}

void setAlphaReplaceColorId(uint8_t colorId)
{
  alphaReplaceColorId = colorId;
}

uint8_t getAlphaReplaceColorId(void)
{
  return alphaReplaceColorId;
}

uint16_t getPlatetteColor(uint8_t id)
{
  return palette_RAM[id];
}

//---------------------------------------------------------------------------//
