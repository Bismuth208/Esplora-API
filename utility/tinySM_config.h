#ifndef _TINY_STATE_MACHINE_CONFIG_H
#define _TINY_STATE_MACHINE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/*
* Where to get systick function or global variable
* with current system time in milleseconds
*/
#define TSM_CONFIG_USE_ARDUINO_LIBS 0

//------------------------ Usefull LIB Flags --------------------------------//
/*
 * If set this to 0, then:
 *   - malloc;
 *   - realloc
 *   - free.
 * will be unavaliable!
 *
 * Only static memory managment!
 * Created for minimize ROM usage.
 * Use only when you know final number of tasks!
 * And tTaskStates.tTaskStatesArr must be init correctly!
 *
 * Like this:
 *  tTaskStates someName = {NULL, 0};
 *  tTaskStatesArr someNameArr[X];
 *  someName.pArr = &someNameArr;
 * where X is number of elements(Tasks);
 */
#define TSM_CONFIG_USE_DYNAMIC_MEM 0

/*
 * How much tasks if static memory managment is selected.
 * This setup is VERY IMPORTANT!!
 * Set correct number of your tasks in program
 * if flag TSM_CONFIG_USE_DYNAMIC_MEM is set to '0'
 *
 * Static allocation in global RAM.
 */
#if !TSM_CONFIG_USE_DYNAMIC_MEM
#define TSM_CONFIG_NUM_WORK_TASKS        20
#endif /* TSM_CONFIG_USE_DYNAMIC_MEM */

/*
 * If you are shure what:
 *   - you have enougth memory (bazillions bazillions bytes);
 *   - no memory leak;
 *   - you know what you're do;
 *   - need a few more programm memory;
 *   - other unnown reasons.
 * Then, set to 0
 */
#define TSM_CONFIG_USE_MEM_PANIC 0

/*
 * If you are shure what:
 *   - you wouldn't add new task in IRQ;
 *   - task array will be always filled at least by one task;
 *   - other unnown reasons.
 * Then, set to 0
 */
#define TSM_CONFIG_USE_TASKS_PANIC 0

/* 
 * If main array with tasks was not propertly inited
 * and prgramm will access to it, when UB possible!
 * If you need to prevent such behavor and stop CPU
 * from mistake set to 1
 * P.S. This is not mistake!
 * Mistake would be if we just silently return!
 */
#define TSM_CONFIG_USE_NULL_PTR_PANIC 0

/*
 * Set this one to 1 and it will periodically call
 * vTSMDefragTasksMemory() func by default.
 * And it not depend on current task array!
 */
#define TSM_CONFIG_USE_AUTO_DEFRAG 0

/* 
 * Set in seconds how often vTSMDefragTasksMemory() will call
 */
#if TSM_CONFIG_USE_AUTO_DEFRAG
  #define TSM_CONFIG_AUTO_DEFRAG_TIMEOUT 10 SEC
#endif

/*
 * Set this one to 1 and it will periodically call
 * vTSMrmDuplicateTasks() func by default.
 * And it not depend on current task array!
 */
#define TSM_CONFIG_USE_AUTO_GEMINI 0

/*
 * Set in seconds how often vTSMrmDuplicateTasks() will call
 */
#if TSM_CONFIG_USE_AUTO_GEMINI
  #define TSM_CONFIG_GEMINI_TIMEOUT 12 SEC
#endif

/*
 * If all tasks always executed and need to save
 * extra RAM and ROM then, set to 0.
 * 
 * When disabled, next functions removed:
 *  - vTSMUpdateTaskStatus();
 *  - vTSMDisableTask();
 *  - vTSMEnableTask();
 *  - vTSMDisableAllTasks();
 *  - vTSMEnableAllTasks();
 * Moreover "execute" flag no longer avaliable and not used.
 */
#define TSM_CONFIG_USE_EXECUTE_FLAG 1

/*
 * Allow to set and execute some routine when some free CPU
 * resources is avaliable.
 * Can be used as mesure of CPU load.
 */
#define TSM_CONFIG_USE_IDLE_FUNC 0

/*
 * Set this one and enable otuput errors on screen.
 * Add depedencies from gfx lib.
 *
 * NOTE: place your specific way to print error codes in vTSMPanic() func!
 */
#define TSM_CONFIG_USE_GFX_LIB 0

/*
 * Select search methode.
 * LINEAR is reque less ROM and RAM, but slow
 */
#define TSM_CONFIG_LINEAR_SEARCH

/*
 * Place here your system tick timer function or global variable
 */
#if TSM_CONFIG_USE_ARDUINO_LIBS
  #include  "Arduino.h"
  #define TSM_SYS_TICK_TIME millis() // In case of Arduino libs usage
#else
  #include "systicktimer.h"
  #define TSM_SYS_TICK_TIME uptime() // Your own implementation

  #if SYS_TIMER_CONFIG_USE_16_BIT
  #warning "Maximum period for tasks is set to 65,5 seconds!"
  #endif 
#endif

/*
 *
 */
// #define TSM_SYS_NULL_TASK   0xFF

/*
 * This one must be less than TSM_SYS_NULL_TASK
 */
// #define TSM_SYS_MAX_TASKS   0xFE

/*
 * Set this one to 1 and enable protection from timer overflow what
 * cause to stop all tasks.
 */
#define TSM_CONFIG_USE_SYS_TICK_PROTECT 0 // not implemented feature


/*
 * Enable/Disable deprecated features like:
 * vTSMInitTasksStorage();
 *
 * Try not to use deprecated features, they are may be deleted at any time.
 */
#define TSM_CONFIG_ENABLE_DEPRECATED_FEATURES 0


#endif /*_TINY_STATE_MACHINE_CONFIG_H*/
