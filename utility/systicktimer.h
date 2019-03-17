#ifndef _SYSTICKTIMER_H
#define _SYSTICKTIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

#define SYS_TIMER_CONFIG_USE_16_BIT 1


#if SYS_TIMER_CONFIG_USE_16_BIT
uint16_t uptime(void);
#else 
uint32_t uptime(void);
uint32_t micros(void);
#endif

void _delayMS(uint16_t timetoloop);

#if !SYS_TIMER_CONFIG_USE_16_BIT
void _delayMicroseconds(uint16_t us);
#endif

void initSysTickTimer(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
