/*
 * Author: Antonov Alexandr (Bismuth208)
 * Date:   11 November, 2017
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
*
 * -----------------------------------------------------------------------------
 * Little readme:
 *
 * This is used when need maximum pic compression,
 * and you can waste some CPU resources for it;
 * It use very simple RLE compression;
 *
 * Where are two versions:
 *  - drawPico_RLE_P;
 *  - drawPico_DIC_P;
 *
 * First use as simple as possible RLE compression;
 * Second use dictionary in addition to previos version;
 *
 *
 * Structure of every picture for drawPico_RLE_P:
 * const uint8_t somePicName[] PROGMEM = {
 *  width-1, height-1,  // one byte each
 *  picture data,       // a lot of bytes... or at least one byte ;)
 *  end marker          // 0xff mean picture data end
 * };
 *
 * Structure of every picture for drawPico_DIC_P:
 * const uint8_t somePicName[] PROGMEM = {
 *  width-1, height-1,  // one byte each
 *  offset,             // one byte offset to picture data and dictionary size
 *  dictionary,         // from zero to 92 bytes
 *  picture data,       // a lot of bytes... or at least one byte ;)
 *  end marker          // 0xff mean picture data end
 * };
 *
 * p.s -1 at begin of every pic is conversations to correspond to tft display addr size
 *
 * For more info look readme.md for picoPacker!
 * -----------------------------------------------------------------------------
 */

#include <avr/pgmspace.h>

#include "picoUnpacker.h"
#include "gfx.h"
#include "st7735.h"
#include "spi.h"
#include "help.h"

//---------------------------------------------------------------------------//
#define DATA_MARK        0x7f
#define RLE_MARK         0x80
#define MAX_DATA_LENGTH  0xcf
#define DICT_MARK        0xd0
#define PIC_DATA_END     0xff
#define ALPHA_COLOR_ID   0x0F

// unsafe but it works on avr, and stm32
#define setPicWData(a, b)   (*(uint16_t*)(&a[b]))
#ifdef __AVR__
 #define getPicWData(a, b)   pgm_read_word(&(a[(b - DICT_MARK)<<1]))
#else
 #define getPicByte(a)      (*(const uint8_t *)(a))
 #define getPicWData(a, b)  (*(const uint16_t*)(&(a[b])))
#endif /* __AVR__ */

//---------------------------------------------------------------------------//
#define MAX_UNFOLD_SIZE 128  // total RAM consumed = MAX_UNFOLD_SIZE * 4

uint8_t buf_packed[MAX_UNFOLD_SIZE<<1];   // \__ unpacked data needs
uint8_t buf_unpacked[MAX_UNFOLD_SIZE<<1]; // /   minimum 2x more RAM
//---------------------------------------------------------------------------//

uint8_t findPackedMark(uint8_t *ptr, uint16_t sizeData)
{
  do {
    if(*ptr++ >= DICT_MARK) {
      return 1;
    }
  } while(--sizeData);
  
  return 0;
}

uint8_t findPackedMark(uint8_t *ptr)
{
  do {
    if((*ptr++) == PIC_DATA_END) break;

    if(*ptr >= DICT_MARK) {
      return 1;
    }
  } while(1);
  
  return 0;
}

void printBuf_RLE(uint8_t *pData) // access to local register: less instructions
{
  uint16_t repeatColor;
  uint8_t repeatTimes, tmpByte;
  uint8_t alphaReplaceColorId = getAlphaReplaceColorId();
  
  while((tmpByte = *pData) != PIC_DATA_END) { // get color index or repeat times
    if(tmpByte & RLE_MARK) { // is it color index?
      tmpByte &= DATA_MARK; // get color index to repeat
      repeatTimes = *(++pData)+1; // zero RLE does not exist!
    }
    ++repeatTimes;
    ++pData;
    
    // get color from colorTable by color index
    repeatColor = palette_RAM[(tmpByte == ALPHA_COLOR_ID) ? alphaReplaceColorId : tmpByte];
    
    do {
#ifdef __AVR__  // really dirt trick... but... FOR THE PERFOMANCE!
      SPDR_t in = {.val = repeatColor};
      SPDR = in.msb;
      SPDR_TX_WAIT;

      SPDR = in.lsb;
      SPDR_TX_WAIT;
#else
      pushColorFast(repeatColor);
#endif
    } while(--repeatTimes);
  }
}

uint8_t *unpackBuf_DIC(const uint8_t *pDict, uint16_t dataSize)
{
  uint16_t bufPackedPos, bufUnpackedPos;
  uint8_t dictMarker = findPackedMark(&buf_packed[0], dataSize);
  uint8_t *pUnpackedData = nullptr;
  
  if(dictMarker) {
    bufPackedPos =0;
    bufUnpackedPos =0;
    pUnpackedData = &buf_unpacked[0];
    
    while(dictMarker) {
      if(buf_packed[bufPackedPos] >= DICT_MARK) {
        setPicWData(buf_unpacked, bufUnpackedPos) = getPicWData(pDict, buf_packed[bufPackedPos]);
        ++bufUnpackedPos;
      } else {
        buf_unpacked[bufUnpackedPos] = buf_packed[bufPackedPos];
      }
      ++bufUnpackedPos;
      ++bufPackedPos;
      
      if(bufPackedPos >= dataSize) {
        dictMarker = findPackedMark(&buf_unpacked[0], bufUnpackedPos);
        dataSize = bufUnpackedPos;
        if(dictMarker) {
          memcpy_F(&buf_packed[0], &buf_unpacked[0], bufUnpackedPos); // maybe swap pointers?
          bufUnpackedPos =0;
          bufPackedPos =0;
        }
      }
    }
  } else {
    pUnpackedData = &buf_packed[0];
  }

  pUnpackedData[dataSize] = PIC_DATA_END; // unpacked chunk end
  return pUnpackedData;
}

// extended RLE, based on dictionary, a bit slower but better compression
void drawPico_DIC_P(uint8_t x, uint8_t y, pic_t *pPic)
{
  auto tmpData = getPicSize(pPic, 0);
  tftSetAddrWindow(x, y, x+tmpData.u8Data1, y+tmpData.u8Data2);
  
  uint8_t tmpByte;
  uint16_t unfoldPos;
  auto pDict = &pPic[3];         // save dictionary pointer
  pPic += getPicByte(&pPic[2]);  // make offset to picture data
  
  for(;;) { // endless cycle here is bad architecture... but it's works!
    unfoldPos =0;
    
    do {
      if((tmpByte = getPicByte(++pPic)) < PIC_DATA_END) {
        if(tmpByte < DICT_MARK) {
          buf_packed[unfoldPos] = tmpByte;
        } else {
          setPicWData(buf_packed, unfoldPos) = getPicWData(pDict, tmpByte);
          ++unfoldPos;
        }
        ++unfoldPos;
      } else {
        break;
      }
    } while((unfoldPos < MAX_UNFOLD_SIZE-1)
            && ((tmpByte > DATA_MARK) || (tmpByte > MAX_DATA_LENGTH)));
    
    if(unfoldPos) {
      //buf_packed[unfoldPos + 1] = PIC_DATA_END; // chunk end
      printBuf_RLE(unpackBuf_DIC(pDict, unfoldPos)); // V2V3 decoder
    } else {
      return;
    }
  }
}

// =============================================================== //
// old version, faster, but less comression
void drawPico_RLE_P(uint8_t x, uint8_t y, pic_t *pPic)
{
  uint16_t repeatColor;
  uint8_t tmpInd, repeatTimes;
  uint8_t alphaReplaceColorId = getAlphaReplaceColorId();

  auto tmpData = getPicSize(pPic, 0);
  tftSetAddrWindow(x, y, x+tmpData.u8Data1, y+tmpData.u8Data2);
  
  ++pPic; // make offset to picture data
  
  while((tmpInd = getPicByte(++pPic)) != PIC_DATA_END) { // get color index or repeat times
    if(tmpInd & RLE_MARK) { // is it color index?
      tmpInd &= DATA_MARK; // get color index to repeat
      repeatTimes = getPicByte(++pPic)+1; // zero RLE does not exist!
    }
    ++repeatTimes;
    
    // get color from colorTable by color index
    repeatColor = palette_RAM[(tmpInd == ALPHA_COLOR_ID) ? alphaReplaceColorId : tmpInd];
    
    do {
#ifdef __AVR__  // really dirt trick... but... FOR THE PERFOMANCE!
      SPDR_t in = {.val = repeatColor};
      SPDR = in.msb;
      SPDR_TX_WAIT;

      SPDR = in.lsb;
      SPDR_TX_WAIT;
#else
      pushColorFast(repeatColor);
#endif
    } while(--repeatTimes);
  }
}
