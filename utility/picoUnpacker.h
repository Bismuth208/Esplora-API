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

#ifndef _PICOUNPACKER_H
#define _PICOUNPACKER_H

#include <stdint.h>
#include "gameAPI.h"

#ifdef __cplusplus
extern "C"{
#endif

void drawPico_DIC_P(uint8_t x, uint8_t y, pic_t *pPic);
void drawPico_RLE_P(uint8_t x, uint8_t y, pic_t *pPic);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  /*_PICOUNPACKER_H*/
