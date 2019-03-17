/* 
 * Each task execute on it timeout.
 * 
 * Povide time-shifted multitask.
 * 
 * It allows you to run undemanding tasks on time,
 * and left more CPU resources to difficult tasks,
 * which will has low influence to other tasks, 
 * if call timeout is low(rare).
 * 
 * System uptime time couter requred.
 * You can change it in TSM_SYS_TICK_TIME define,
 * but it must return milliseconds!
 * 
 * Minimal time unit: 1 millis
 * Maximum tasks:     254 (0xFF reserved as NULL)
 * RAM per task:      9 bytes (on AVR only!)
 * Language:          C
 * 
 * Author: Antonov Alexandr (Bismuth208)
 * Created:     7 December, 2015
 * Last edit:  10 January, 2019
 * e-mail: bismuth20883@gmail.com
 * 
 * 1 tab = 2 spaces
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __AVR__
 #include <avr/pgmspace.h>
#else
 #define PROGMEM
 #define pgm_read_word(a) (*(const uint16_t*)(a))
#endif

#include "tinySM.h"

//---------------------------------------------------------------------------//

// PAA - pointer acess array
#define PAA pxCurrentTasksStorage->pxTasksArr
// PAC - pointer acess count
#define PAC pxCurrentTasksStorage->ucTasksCount


#if TSM_CONFIG_USE_NULL_PTR_PANIC
  #define PAA_NULL_CHECK() if (PAA == NULL) vTSMPanic(NULL_PTR_ACCESS)
#else
  #define PAA_NULL_CHECK()
#endif

//---------------------------------------------------------------------------//
static bool ucResetTaskCount = false;

#if TSM_CONFIG_USE_MEM_PANIC
const uint8_t textPanic[] /*PROGMEM*/ = "Sorry...\nMemory panic :(\n\n";
const uint8_t textError[] /*PROGMEM*/ = "Error code: ";
#endif

#if TSM_CONFIG_USE_AUTO_DEFRAG
TSM_TIME_TYPE ulDefragLastCallTime = 0;
#endif
#if TSM_CONFIG_USE_AUTO_GEMINI
TSM_TIME_TYPE ulGeminiLastCallTime = 0;
#endif

#if TSM_CONFIG_USE_GFX_LIB
fPrint_t fpPrint = NULL;
fFillRect_t fpFillRect = NULL;
fSetCursor_t fpSetCursor = NULL;
fDrawFastVLine_t fpDrawFastVLine = NULL;
#endif

#if TSM_CONFIG_USE_IDLE_FUNC
pFunc_t xFuncIDLE = NULL;
#endif

//---------------------------------------------------------------------------//
#if !TSM_CONFIG_USE_DYNAMIC_MEM
// super puper duper ultra extreemely main structures, DO NOT TOUCH THEM!
// Used only in static memory managment
taskFunc_t xTasksArr[TSM_CONFIG_NUM_WORK_TASKS];
tasksContainer_t xTasksContainer = {&xTasksArr[0], 0};
static uint8_t ucMaxTasks = TSM_CONFIG_NUM_WORK_TASKS;

// pointer to current tasks array
static tasksContainer_t *pxCurrentTasksStorage = &xTasksContainer;
#else
static tasksContainer_t *pxCurrentTasksStorage = NULL;
#endif /* TSM_CONFIG_USE_DYNAMIC_MEM */
//---------------------------------------------------------------------------//


//------------------- Main loop reincarnation -------------------------------//
/**
 * @brief  main cycle
 * @param  None
 * @retval None
 */
__attribute__ ((noreturn)) void vTSMRunTasks(void)
{
  PAA_NULL_CHECK();

  uint8_t ucCurrentTask = 0;
  
  // store pointer from RAM, to reduce instructions
  taskFunc_t *pxPAATask = &PAA[0];

  for (;;) {
#if TSM_CONFIG_USE_AUTO_GEMINI
    if (((TSM_TIME_TYPE)TSM_SYS_TICK_TIME - (TSM_TIME_TYPE)ulGeminiLastCallTime)
        >= TSM_CONFIG_AUTO_GEMINI_TIMEOUT) {
      vTSMrmSameTasks();
      ulGeminiLastCallTime = TSM_SYS_TICK_TIME;
    }
#endif /* TSM_CONFIG_USE_AUTO_GEMINI */

#if TSM_CONFIG_USE_AUTO_DEFRAG
    if (((TSM_TIME_TYPE)TSM_SYS_TICK_TIME - (TSM_TIME_TYPE)ulDefragLastCallTime)
        >= (TSM_TIME_TYPE)TSM_CONFIG_AUTO_DEFRAG_TIMEOUT) {
      vTSMDefragTasksMemory();
      ulDefragLastCallTime = TSM_SYS_TICK_TIME;
    }
#endif /* TSM_CONFIG_USE_AUTO_DEFRAG */

#if TSM_CONFIG_USE_IDLE_FUNC
    if (xFuncIDLE) xFuncIDLE();
#endif /* TSM_CONFIG_USE_IDLE_FUNC */

#if TSM_CONFIG_USE_TASKS_PANIC
    if (PAC) {
#endif /* TSM_CONFIG_USE_TASKS_PANIC */

      // Have func?
      if (pxPAATask->xTask.xFunc) { // problems in future see i here
#if TSM_CONFIG_USE_EXECUTE_FLAG
        if (pxPAATask->ucExecute) { // need execute?
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
          // check timeout
          if (((TSM_TIME_TYPE)TSM_SYS_TICK_TIME - (TSM_TIME_TYPE)pxPAATask->ulLastCallTime) 
              >= (TSM_TIME_TYPE)pxPAATask->xTask.usTimeOut) {
            pxPAATask->xTask.xFunc(); // execute
            
            if (ucResetTaskCount) {
              ucResetTaskCount = false;
              goto RESET_TASKS_COUNT_LABEL; // sorry... there is no way exept this one...
            } else {
              // get time of next execution
              pxPAATask->ulLastCallTime = TSM_SYS_TICK_TIME;
            }
          }
#if TSM_CONFIG_USE_EXECUTE_FLAG
        }
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
      }
      
      if (++ucCurrentTask >= PAC) {
RESET_TASKS_COUNT_LABEL:
        ucCurrentTask = 0;
        pxPAATask = &PAA[0];
      } else {
        ++pxPAATask;
      }
#if TSM_CONFIG_USE_TASKS_PANIC
    } else {
      vTSMPanic(CHECK_TASKS_FAIL | NO_TASKS_FAIL);
    }
#endif /* TSM_CONFIG_USE_TASKS_PANIC */
  }
}

//---------------------------------------------------------------------------//
/**
 * @brief  add one task for execution
 * @param  pxTask:  pointer to task function
 * @param  usTimeToCheckTask: time period, when run task
 * @param  ucExec: need execute or not (aka reserve mem for future)
 * @retval None
 */
void vTSMAddTask(pFunc_t pxTask, uint16_t usTimeToCheckTask, bool ucExec)
{
  PAA_NULL_CHECK();
  
  // Add task by reallocate memory
  // for dynamic struct array with pointers to funtions.
  // After, place task and timeout to new index in array.

#if TSM_CONFIG_USE_MEM_PANIC
  if (PAC < MAX_TASKS) { // less than 254 tasks
    PAC++;  // increase total number of tasks in queue
  } else {
    vTSMPanic(OVER_LIMIT_FAIL | ADD_FAIL);
  }
#else
  PAC++;  // increase total number of tasks in queue
#endif /* TSM_CONFIG_USE_MEM_PANIC */

#if (TSM_CONFIG_USE_TASKS_PANIC) && (!TSM_CONFIG_USE_DYNAMIC_MEM)
  if (PAC > ucMaxTasks) {
    vTSMPanic(OVER_RANGE_FAIL | ADD_FAIL);
  }
#endif

#if TSM_CONFIG_USE_DYNAMIC_MEM
  // reallocate block of RAM for new task
  PAA = (taskFunc_t*) realloc(PAA, PAC * sizeof(taskFunc_t));
#endif

  if (PAA != NULL) {
    // aaand place params to new index
    // why -1? because we can`t allocate 0 bytes :)
   taskFunc_t *pxPAATask = &PAA[PAC-1]; // reduce instructions by acces pointer
   pxPAATask->xTask.xFunc = pxTask;
   pxPAATask->xTask.usTimeOut = usTimeToCheckTask;
   pxPAATask->ulLastCallTime = TSM_SYS_TICK_TIME;
#if TSM_CONFIG_USE_EXECUTE_FLAG
   pxPAATask->ucExecute = ucExec;
#else 
   (void)ucExec;
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
    
#if TSM_CONFIG_USE_MEM_PANIC   
  } else {
    vTSMPanic(ALLOC_FAIL | ADD_FAIL);
  }
#else
  }
#endif /* TSM_CONFIG_USE_MEM_PANIC */
}


/**
 * @brief  add one task for execution
 * @param  pxTasksStorage: pointer to array whith tasks
 * @param  xTask:  pointer to task function
 * @param  usTimeToCheckTask: time period, when run task
 * @param  ucExec: need execute or not (aka reserve mem for future)
 * @retval None
 */
void vTSMAddTaskToArr(tasksContainer_t *pxTasksStorage, pFunc_t xTask,
                                  uint16_t usTimeToCheckTask, bool ucExec)
{
  // Add task by reallocate memory
  // for dynamic struct array with pointers to funtions.
  // After, place task and timeout to new index in array.
#if TSM_CONFIG_USE_NULL_PTR_PANIC
  if (pxTasksStorage == NULL) vTSMPanic(NULL_PTR_ACCESS);
  if (pxTasksStorage->pxTaskArr == NULL) vTSMPanic(NULL_PTR_ACCESS);
#endif

#if TSM_CONFIG_USE_MEM_PANIC
  if (pxTasksStorage->ucTasksCount < MAX_TASKS) { // less than 254 tasks
    pxTasksStorage->ucTasksCount++;  // increase total number of tasks in queue
  } else {
    vTSMPanic(OVER_LIMIT_FAIL | ADD_TO_ARR_FAIL);
  }
#else
  pxTasksStorage->ucTasksCount++;  // increase total number of tasks in queue
#endif /* TSM_CONFIG_USE_MEM_PANIC */

#if (TSM_CONFIG_USE_TASKS_PANIC) && (!TSM_CONFIG_USE_DYNAMIC_MEM)
  if (pxTasksStorage->ucTasksCount > ucMaxTasks) {
    vTSMPanic(OVER_RANGE_FAIL | ADD_TO_ARR_FAIL);
  }
#endif

#if TSM_CONFIG_USE_DYNAMIC_MEM
  // reallocate block of RAM for new task
  pxTasksStorage->pxTaskArr = (taskFunc_t*) realloc(pxTasksStorage->pxTaskArr,
                                  pxTasksStorage->ucTasksCount * sizeof(taskFunc_t));
#endif

  if (pxTasksStorage->pxTasksArr != NULL) {
    // aaand place params to new index
    // why -1? because we can`t allocate 0 bytes :)
    taskFunc_t *pxPAATask = &pxTasksStorage->pxTasksArr[pxTasksStorage->ucTasksCount-1]; // same as vTSMAddTask()
    pxPAATask->xTask.xFunc = xTask;
    pxPAATask->xTask.usTimeOut = usTimeToCheckTask;
    pxPAATask->ulLastCallTime = TSM_SYS_TICK_TIME;
#if TSM_CONFIG_USE_EXECUTE_FLAG
    pxPAATask->ucExecute = ucExec;
#else 
   (void)ucExec;
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
    
#if TSM_CONFIG_USE_MEM_PANIC
  } else {
    vTSMPanic(ALLOC_FAIL | ADD_TO_ARR_FAIL);
  }
#else
  }
#endif /* TSM_CONFIG_USE_MEM_PANIC */
}

/**
 * @brief  add one task for execution from const (PROGMEM) section
 * @param  pxTaskP: pointer to task stucture
 * @retval None
 */
void vTSMAddTask_P(const taskParams_t *pxTaskP)
{
  // This fuction is really dengerous as no any checks!
  // Use it if you over 9000% sure what you're doing!
  
  // aaand place params to new index
  // increase total tasks
  taskFunc_t *pxPAATask = &PAA[PAC++]; // reduce instructions by acces pointer
  // pxPAATask->xTask.xFunc = (pFunc_t)pgm_read_word(&pTaskP->xFunc);
  // pxPAATask->xTask.usTimeOut = pgm_read_word(&pTaskP->usTimeOut);
  memcpy_P(&pxPAATask->xTask, pxTaskP, sizeof(taskParams_t));
  pxPAATask->ulLastCallTime = 0; // every fuction will call immediately
#if TSM_CONFIG_USE_EXECUTE_FLAG
  pxPAATask->ucExecute = true; // always true...
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
}

/**
 * @brief  add array of tasks for execution from PROGMEM section
 * @param  pxArr: pointer to task stucture whith pointers to tasks
 * @param  size: number of tasks to load
 * @retval None
 */
void vTSMAddTasksArray_P(tasksArr_t *pxArr)
{
  vTSMDeleteAllTasks();

  const taskParams_t *pxTask = NULL;

  // get ponter to task structure and load params from structure
  while ((pxTask = (taskParams_t*)pgm_read_word(pxArr)) != NULL) {
    vTSMAddTask_P(pxTask);
    ++pxArr;
  }
}

/**
 * @brief  remove all tasks
 * @param  none
 * @retval None
 */
void vTSMDeleteAllTasks(void)
{
  PAA_NULL_CHECK();

  uint8_t *pucBuf = (uint8_t*)PAA;
  uint16_t usSize = ucMaxTasks * sizeof(tasksContainer_t);
  do {
    *pucBuf++ = 0x00;
  } while (--usSize);
  
  PAC = 0;
  ucResetTaskCount = true;
#if TSM_CONFIG_USE_DYNAMIC_MEM
  free(PAA);
  PAA = NULL;
#endif /*TSM_CONFIG_USE_DYNAMIC_MEM*/
}

/**
 * @brief  remove only single task from execution
 * @param  xTask: pointer to task what to remove
 * @retval None
 */
void vTSMDeleteTask(pFunc_t xTask)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = ucTSMSearchTask(xTask);

  PAA[ucTaskId].xTask.xFunc = NULL;     // remove pointer
  ucResetTaskCount = true;
#if TSM_CONFIG_USE_EXECUTE_FLAG
   //PAA[ucTaskId].ucExecute = false;     // clear exec flag
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
  
  // don't forget to call: vTSMDefragTasksMemory();
}

/**
 * @brief  disable execution only for single task
 * @param  xTask: pointer to task what to disable
 * @retval None
 */
#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMDisableTask(pFunc_t xTask)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = ucTSMSearchTask(xTask);
  PAA[ucTaskId].ucExecute = false;
}
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */

/**
 * @brief  enable execution only for single task
 * @param  xTask: pointer to task what to enable
 * @retval None
 */
#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMEnableTask(pFunc_t xTask)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = ucTSMSearchTask(xTask);
  PAA[ucTaskId].ucExecute = true;
}
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */

/**
 * @brief  disable execution for all tasks
 * @param  None
 * @retval None
 */
#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMDisableAllTasks(void)
{
  PAA_NULL_CHECK();

  for (uint8_t ucCount = 0; ucCount < ucMaxTasks; ucCount++) {
    PAA[ucCount].ucExecute = false;
  }
}
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */

/**
 * @brief  enable execution for all tasks
 * @param  None
 * @retval None
 */
#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMEnableAllTasks(void)
{
  PAA_NULL_CHECK();

  for (uint8_t ucCount = 0; ucCount < ucMaxTasks; ucCount++) {
    PAA[ucCount].ucExecute = true;
  }
}
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */

/**
 * @brief  change execution flag only for single task
 * @param  xTask: pointer to task what to disable
 * @param  ucExec: new flag state
 * @retval None
 */
#if TSM_CONFIG_USE_EXECUTE_FLAG
void vTSMUpdateTaskStatus(pFunc_t xTask, bool ucExec)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = ucTSMSearchTask(xTask);
  PAA[ucTaskId].ucExecute = ucExec;
}
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */

/**
 * @brief  change execution period only for single task
 * @param  xTask: pointer to task what to disable
 * @param  usTimeToCheckTask: new period
 * @retval None
 */
void vTSMUpdateTaskTimeCheck(pFunc_t xTask, uint16_t usTimeToCheckTask)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = ucTSMSearchTask(xTask);
  taskFunc_t *pxPAATask = &PAA[ucTaskId];
  pxPAATask->xTask.usTimeOut = usTimeToCheckTask;
  pxPAATask->ulLastCallTime = TSM_SYS_TICK_TIME;
}

/**
 * @brief  replace one tast by onother one and update its params
 * @param  xOldTask: pointer to task what to replace
 * @param  xNewTask: pointer to new task
 * @param  usTimeToCheckTask: new period
 * @param  ucExec: new flag state
 * @retval None
 */
void vTSMReplaceTask(pFunc_t xOldTask, pFunc_t xNewTask, uint16_t usTimeToCheckTask, bool ucExec)
{
  PAA_NULL_CHECK();

  // Also it combine vTSMDeleteTask().
  // Just call like this: vTSMReplaceTask(taskToDelete, NULL, 0, false);

  uint8_t ucTaskId = ucTSMSearchTask(xOldTask);
  taskFunc_t *pxPAATask = &PAA[ucTaskId];

  pxPAATask->xTask.xFunc = xNewTask;
  pxPAATask->xTask.usTimeOut = usTimeToCheckTask;
  pxPAATask->ulLastCallTime = TSM_SYS_TICK_TIME;
#if TSM_CONFIG_USE_EXECUTE_FLAG
  pxPAATask->ucExecute = ucExec;
#else
  (void)ucExec;
#endif /* TSM_CONFIG_USE_EXECUTE_FLAG */
}

/**
 * @brief  remove holes in memory
 * @param  none
 * @retval None
 */
void vTSMDefragTasksMemory(void)
{
  PAA_NULL_CHECK();

  // After some time of work may appear many holes
  // more than need. It eat much of memory, and that
  // is why need to remove them.

  bool ucIsDefragged = false;
  uint8_t nullCount =0;
  uint8_t i=0, j=0;
  
  taskFunc_t xTmpTask;
  
  // First: find all NULL pointers and move them to end of array
#if 1
  // bubble sort
  for (i = 0; i < (PAC); i++) {
    for (j = 0; j < (PAC - i - 1); j++) {
      if (PAA[j].xTask.xFunc == NULL) {
        
        xTmpTask = PAA[j];
        PAA[j] = PAA[j + 1];
        PAA[j + 1] = xTmpTask;
        //memcpy(&xTmpTask, &PAA[j], sizeof(tTaskStatesArr));
        //memcpy(&PAA[j], &PAA[j + 1], sizeof(tTaskStatesArr));
        //memcpy(&PAA[j + 1], &xTmpTask, sizeof(tTaskStatesArr));
        ucIsDefragged = true;
      }
    }
  }
#else
  // another type of sorting
#endif
  // Second: if some NULL was finded
  // when cut them off.
  if (ucIsDefragged) {
    // search all NULL ponters from end of array
    for (i=PAC-1; i > 1; i--) {
      if (PAA[i].xTask.xFunc != NULL) {
        break;
      } else {
        ++nullCount; // count how much NULL`s need to cut off
      }
    }
    
    PAC -= nullCount; // Remove waste NULL`s
    ucResetTaskCount = true;
    
#if TSM_CONFIG_USE_DYNAMIC_MEM
    // free some RAM
    PAA = (taskFunc_t*)realloc(PAA, PAC * sizeof(taskFunc_t));
#if TSM_CONFIG_USE_MEM_PANIC
    if (PAA == NULL) {
      vTSMPanic(ALLOC_FAIL | DEFRAG_FAIL);
    }
#endif /*TSM_CONFIG_USE_MEM_PANIC*/
#endif /*TSM_CONFIG_USE_DYNAMIC_MEM*/
  }
}

/**
 * @brief  remove same tasks in memory
 * @param  none
 * @warning  still work unstable!
 * @retval None
 */
void vTSMrmSameTasks(void)
{
  PAA_NULL_CHECK();

  // If by some reasons in task arr are two or more
  // same tasks - remove them
  
  uint8_t i, j;
  
  //vTSMPrintTasksMem(0);
  //vTSMDefragTasksMemory();
  //vTSMPrintTasksMem(40);

  for (i=0; i<PAC; i++) { //
    for (j=1; j<(PAC-i); j++) {
      if ((PAA[i].xTask.xFunc) && (PAA[i+j].xTask.xFunc)) {
        if (PAA[i].xTask.xFunc == PAA[i+j].xTask.xFunc) {
          PAA[i+j].xTask.xFunc = NULL;
        }
      }
    }
  }

  ucResetTaskCount = true;
  
  //vTSMPrintTasksMem(80);
  //vTSMDefragTasksMemory();
  //vTSMPrintTasksMem(120);
}

//==================== SEARCH FUNCTION ======================================//
#ifdef TSM_CONFIG_LINEAR_SEARCH
/**
 * @brief  search task in current tasks array
 * @param  xTask: pointer to task function
 * @retval number id in tasks array
 */
uint8_t ucTSMSearchTask(pFunc_t xTask)
{
  PAA_NULL_CHECK();

  uint8_t ucTaskId = 0;
  taskFunc_t *pxPAATask = &PAA[0]; // store pointer to X register
  
#ifdef __AVR__
  taskParams_t xTmpOne, xTmpTwo;
  xTmpOne.xFunc = xTask; // tmpOne.xFunc use only r20,r21

  do {
    xTmpTwo.xFunc = (pFunc_t)pxPAATask->xTask.xFunc; // store addr to r24,r25
    // compare addr separetly, reque less instructions
    if (xTmpOne.hi == xTmpTwo.hi) { // compare r24,r21
      if (xTmpOne.low == xTmpTwo.low) { // compare r25,r20
        return ucTaskId; // ok, this is it!
      }
    }
    ++pxPAATask;
  } while ((++ucTaskId) < PAC);
#else
  do {
    if (pxPAATask->xTask.xFunc == xTask) {
      return ucTaskId; // ok, this is it!
    }
    ++pxPAATask;
  } while ((++ucTaskId) < PAC);
#endif

#if TSM_CONFIG_USE_MEM_PANIC
  vTSMPanic(FIND_TASK_FAIL | OVER_RANGE_FAIL); // warn what program will work incorrect
#endif

  return NULL_TASK;  // no such func
}
#endif /* TSM_CONFIG_LINEAR_SEARCH */
//===========================================================================//
/**
 * @brief  init base pointers
 * @param  pxNewTasksStorage: pointer to current task states
 * @param  pxTasksArr: pointer to array whith tasks
 * @param  ucMaximumTasks: maximum possible tasks in task array
 * @warning  all pointers must be inited propertly otherwise: undefined bahavor!
 * @retval none
 */
#if TSM_CONFIG_ENABLE_DEPRECATED_FEATURES
void vTSMInitTasksStorage(tasksContainer_t *pxNewTasksStorage, taskFunc_t *pxTasksArr, uint8_t ucMaximumTasks)
{
  pxCurrentTasksStorage = pxNewTasksStorage;
  PAA = pxTasksArr;
#if !TSM_CONFIG_USE_DYNAMIC_MEM
  ucMaxTasks = ucMaximumTasks;
#endif
  ucResetTaskCount = true;
}
#endif

/**
 * @brief  init base pointer
 * @param  pxNewTasksStorage: pointer to current task states
 * @retval pointer to previous task states scturct
 */
tasksContainer_t *pxTSMSetTasksStorage(tasksContainer_t *pxNewTasksStorage)
{
  tasksContainer_t *pxOldTasksStorage = pxCurrentTasksStorage; // make a copy
  pxCurrentTasksStorage = pxNewTasksStorage; // replace pointers
  ucResetTaskCount = true;

  return pxOldTasksStorage; // previous pointer
}

/**
 * @brief  get current task states
 * @param  none
 * @retval pointer to current task states
 */
tasksContainer_t *pxTSMGetTasksStorage(void)
{
  return pxCurrentTasksStorage;
}

#if !TSM_CONFIG_USE_DYNAMIC_MEM
// Use only when TSM_CONFIG_USE_DYNAMIC_MEM is 0
void vTSMSetMaxTasks(uint8_t ucMaximumTasks)
{
  ucMaxTasks = ucMaximumTasks;
  ucResetTaskCount = true;
}
#endif

// ------------------------------- helpfull -------------------------- //
/**
 * @brief  put error code to specific output system; stop exec functions and stay in it
 * @param  ucErrorCode: code whith reason where and why panic happened
 * @retval none
 */
__attribute__ ((noreturn)) void vTSMPanic(uint8_t ucErrorCode)
{
  (void)ucErrorCode;
  
#if TSM_CONFIG_USE_MEM_PANIC
  char errBuf[3];
  
  itoa(ucErrorCode, errBuf, 16);
  
 #if TSM_CONFIG_USE_GFX_LIB
  fpSetCursor(0,0);
    
  fpFillRect(0,0,GFX_SCR_W,GFX_SCR_H,COLOR_BLUE);
  fpPrint(textPanic);
  fpPrint(textError);
  
  fPrint(errBuf);
 #else
  // place here some another way to print error code
 #endif /*TSM_CONFIG_USE_GFX_LIB*/
#endif /*TSM_CONFIG_USE_MEM_PANIC*/
  
  while (1); // panic mode: on
}

/**
 * @brief  get number of tasks stored in current task array
 * @param  none
 * @retval number of tasks stored in current task array
 */
uint8_t ucTSMAvalibleTasks(void)
{
  return PAC;
}

/**
 * @brief  get size of avaliable RAM
 * @param  none
 * @retval number of free bytes in RAM
 */
uint16_t usTSMAvalibleRam(void) // space between the heap and the stack
{
#ifdef __AVR__
  // GCC unicorn magic...
  extern uint16_t __heap_start, *__brkval;
  uint16_t v;
  return (uint16_t) &v - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval);
#else // another arch, don`t know how to check
  return 0;
#endif
}

#if TSM_CONFIG_USE_IDLE_FUNC
void vTSMSetIdleFunc(pFunc_t xTask)
{
  xFuncIDLE = xTask;
}
#endif

#if TSM_CONFIG_USE_GFX_LIB
/**
 * @brief  print all tasks adreses stored in RAM
 * @param  offset: position in pixels X ordinate
 * @warning  this is for debug only! and reque graphycs screen!
 * @retval none
 */
void vTSMPrintTasksMem(uint16_t offset)
{
  char buf[10];
  char adrBuf[5];
  
  fpFillRect(offset, 0, 36, 8*PAC, GFX_COLOR_BLUE);
  fpDrawFastVLine(offset+37, 0, 8*PAC, GFX_COLOR_WHITE);
  
  for (uint8_t i=0; i<PAC; i++) {
    memset(buf, 0x00, 10);
    memset(adrBuf, 0x00, 5);

    fpSetCursor(offset, 8*i);
    
    strcat(buf, "0x");
    strcat(buf, itoa(PAA[i].xTask.xFunc, adrBuf, 16));
    strcat(buf, "\n");
    
    fpPrint(buf);
  }
}

/**
 * @brief  setup GFX function pointers
 * @param  fPrint: draw text
 * @param  fFillRect: draw rectangle
 * @param  fSetCursor: set text cursor on X and Y position
 * @param  fDrawFastVLine: draw vertical line
 * @retval none
 */
void vTSMSetGfxFunc(fPrint_t fPrint, fFillRect_t fFillRect, fSetCursor_t fSetCursor,
                                                fDrawFastVLine_t fDrawFastVLine)
{
  fpPrint = fPrint;
  fpFillRect = fFillRect;
  fpSetCursor = fSetCursor;
  fpDrawFastVLine = fDrawFastVLine;
}
#endif /* TSM_CONFIG_USE_GFX_LIB */
// ------------------------------------------------------------------- //
