#ifndef _TINY_STATE_MACHINE_H
#define _TINY_STATE_MACHINE_H

// Include file from you project with local setting for TinySM
#include "tinySM_config.h"

#define MAX_TASKS   0xFE
#define NULL_TASK   0xFF
#define SEC         *1000

//#define SHRINK_FACTOR 1.24733f

//------------------------ Usefull LIB Flags --------------------------------//
#if TSM_CONFIG_USE_GFX_LIB
// in rgb 565 color space
  #define GFX_COLOR_BLUE   0x001F  //   0,   0, 255
  #define GFX_COLOR_WHITE  0xFFFF  // 255, 255, 255

// screen size in pixels
  #define GFX_SCR_W        160
  #define GFX_SCR_H        128
#endif /* TSM_CONFIG_USE_GFX_LIB */
//---------------------------------------------------------------------------//

//----------- ERROR CODES --------------//
//  NAME:                  ERR CODE:    WHY IT`S HAPPEN:
#define NO_TASKS_FAIL           0x00    // tasks list is empty!
#define OVER_LIMIT_FAIL         0x01    // more than: 0xFE
#define OVER_RANGE_FAIL         0x02    // more than: maxTasks
#define ALLOC_FAIL              0x03    // not enougth RAM
#define NULL_PTR_ACCESS         0x0F    // PAA was not inited!

//                                      WHERE IT`S HAPPEN:
#define CHECK_TASKS_FAIL        0x00    // vTSMRunTasks()
#define ADD_FAIL                0x10    // vTSMAddTask()
#define ADD_TO_ARR_FAIL         0x20    // vTSMAddTaskToArr()
#define DEFRAG_FAIL             0x30    // vTSMDefragTasksMemory()
#define FIND_TASK_FAIL          0x40    // ucTSMSearchTask()
//--------------------------------------//

//---------- Bool definitions --------------//
#ifndef bool
 #define bool uint8_t
#endif

#ifndef true
 #define true 1
#endif

#ifndef false
 #define false 0
#endif

//------------------------------------------//
#if SYS_TIMER_CONFIG_USE_16_BIT
  #define TSM_TIME_TYPE uint16_t
#else
  #define TSM_TIME_TYPE uint32_t
#endif

//------------------------------------------//

#define T(a) a##Task
#define TASK_N(a)     const taskParams_t T(a)
#define TASK(a,b)     TASK_N(a) PROGMEM = {.xFunc=a, .usTimeOut=b}
#define TASK_P(a)     (taskParams_t*)&T(a)
#define TASK_ARR_N(a) const tasksArr_t a##TasksArr[]
#define TASK_ARR(a)   TASK_ARR_N(a) PROGMEM
#define TASK_END      NULL

//------------------------------------------//

#ifdef __cplusplus
extern "C"{
#endif

//--------------------------- Little Help -----------------------------------//
typedef void (*pFunc_t)(void);

#if TSM_CONFIG_USE_GFX_LIB
typedef void (*fPrint_t)(uint8_t*);
typedef void (*fSetCursor_t)(uint16_t, uint16_t);
typedef void (*fDrawFastVLine_t)(uint16_t, uint16_t, uint16_t, uint16_t);
typedef void (*fFillRect_t)(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
#endif
//---------------------------------------------------------------------------//
    
//------------------------ Tasks Structures ---------------------------------//
#pragma pack(push, 1)
typedef struct {
#ifdef __AVR__
  union  {
    pFunc_t xFunc;    // on avr it get 2 bytes
    struct {
      uint8_t hi;
      uint8_t low;
    };
  };
#else
  pFunc_t xFunc;     // on 32-bit arch it get 4 bytes
#endif /* __AVR__ */
  
  uint16_t usTimeOut;   // 65,5 seconds will be enougth? (65535 millis / 1000)
} taskParams_t;
  
typedef struct {                  // 9 bytes RAM(*)
  TSM_TIME_TYPE ulLastCallTime;   // when was last func call
  taskParams_t xTask;
#if TSM_CONFIG_USE_EXECUTE_FLAG
  uint8_t ucExecute;      		    // status flag; need exec or not
#endif
  /* // at this moment not need
  struct {
  	uint8_t ucExecute 	:1;		// status flag; need exec or not
  	uint8_t ucFreeRam 	:7;		// not implemented features
    //uint8_t ucPriority 	:2;		// priority of xTask 0-3
  };
   */
  // whole size: avr = 7 bytes, arm = 9 bytes
  // or avr = 5 bytes if SYS_TIMER_CONFIG_USE_16_BIT is set
  // or avr = 4 bytes if TSM_CONFIG_USE_EXECUTE_FLAG is not set + above flag is set
} taskFunc_t;

typedef struct {            // (2 bytes (*) + tasksCount * taskFunc_t) + 1 bytes RAM
  taskFunc_t *pxTasksArr;   // pointer to array whith tasks
  uint8_t ucTasksCount;     // Note: 0xFF reserved as NULL !
  // whole size: avr = 3 bytes, arm = 5 bytes.
} tasksContainer_t;
#pragma pack(pop)
  
typedef const taskParams_t * const tasksArr_t;
  
// (*) On AVR arch only.
//---------------------------------------------------------------------------//
  
//------------------------ Function Prototypes ------------------------------//
void vTSMAddTaskToArr(tasksContainer_t *pxTasksStorage, pFunc_t xTask, uint16_t usTimeToCheckTask, bool ucExec);
void vTSMAddTask(pFunc_t xTask, uint16_t usTimeToCheckTask, bool ucExec);
void vTSMAddTask_P(const taskParams_t *pxTaskP);
void vTSMAddTasksArray_P(tasksArr_t *pxArr);
void vTSMReplaceTask(pFunc_t xOldTask, pFunc_t xNewTask, uint16_t usTimeToCheckTask, bool ucExec);
void vTSMUpdateTaskTimeCheck(pFunc_t xTask, uint16_t usTimeToCheckTask);
void vTSMDeleteTask(pFunc_t xTask);
void vTSMDeleteAllTasks(void);

#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMUpdateTaskStatus(pFunc_t xOldTask, bool ucExec);
void vTSMDisableTask(pFunc_t xTask);
void vTSMEnableTask(pFunc_t xTask);
void vTSMDisableAllTasks(void);
void vTSMEnableAllTasks(void);
#endif

__attribute__ ((noreturn)) void vTSMRunTasks(void);

#if TSM_CONFIG_ENABLE_DEPRECATED_FEATURES
void vTSMInitTasksStorage(tasksContainer_t *pxNewTasksStorage, taskFunc_t *pxTasksArr, uint8_t ucMaximumTasks);
#endif

tasksContainer_t *pxTSMSetTasksStorage(tasksContainer_t *pxNewTasksStorage);
tasksContainer_t *pxTSMGetTasksStorage(void);
void vTSMSetMaxTasks(uint8_t ucMaximumTasks);
  
void vTSMrmSameTasks(void);  // still not ready
void vTSMDefragTasksMemory(void);
uint8_t ucTSMSearchTask(pFunc_t xTask);
uint8_t ucTSMAvalibleTasks(void);
uint16_t usTSMAvalibleRam(void);
__attribute__ ((noreturn)) void vTSMPanic(uint8_t ucErrorCode);
  
void vTSMPrintTasksMem(uint16_t usOffset);

#if TSM_CONFIG_USE_IDLE_FUNC
void vTSMSetIdleFunc(pFunc_t xTask);
#endif  

#if TSM_CONFIG_USE_GFX_LIB
void vTSMSetGfxFunc(fPrint_t, fFillRect_t, fSetCursor_t, fDrawFastVLine_t);
#endif
//---------------------------------------------------------------------------//
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif /*_TINY_STATE_MACHINE_H*/
