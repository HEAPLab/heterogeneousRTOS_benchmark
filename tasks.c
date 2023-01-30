/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
//fedit add
#include <stdarg.h>
//#include <math.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"


float compute_utilisation(RTTask_t tasks[], u8 number_of_tasks, u32 systemCriticalityLevel, u32 taskCriticalityLevel) {
	float val=0;
	for (int i=0; i<number_of_tasks; i++) {
		if (tasks[i].pxCriticalityLevel==taskCriticalityLevel && systemCriticalityLevel>=tasks[i].pxCriticalityLevel)
			val+=tasks[i].pxWcet[systemCriticalityLevel]/tasks[i].pxPeriod;
	}
	return val;
}

int find_k(RTTask_t tasks[], u8 number_of_tasks) {
	float u=0;
	for (int l=0; l<configCRITICALITY_LEVELS; l++)
		u+=compute_utilisation(tasks, number_of_tasks, l, l);
	if (u<=1)
		return -2;

	for (int k=0; k<configCRITICALITY_LEVELS-1; k++) {
		float first=0;
		for (int l=k+1; l<configCRITICALITY_LEVELS; l++)
			first+=compute_utilisation(tasks, number_of_tasks, k, l);

		float v1=0;
		for (int l=0; l<=k; l++)
			v1+=compute_utilisation(tasks, number_of_tasks, l, l);

		first=first/(1-v1);

		float second=0;
		float v2=0;
		for (int l=k+1; l<configCRITICALITY_LEVELS; l++)
			v2+=compute_utilisation(tasks, number_of_tasks, l, l);

		second=1-v2;
		second=second/v1;

		if (first<=second && (1-v1)>0) {
			return k; //schedulable
		}
	}
	return -1;
}

//	int compute_schedulability(RTTask_t prvRTTasksList[configMAX_RT_TASKS], u8 numberOfTasks) {
//		float util=0;
//		for (int i=0; i<configMAX_RT_TASKS; i++) {
//			util+=compute_utilisation(tasks, number_of_tasks, l, l);
//		}
//		if (util<=1)
//
//
//		return
//	}
int calculate_x(RTTask_t tasks[], u8 numberOfTasks, int k) {
	float v1=0;
	for (int l=k+1; l<configCRITICALITY_LEVELS; l++) {
		v1+=compute_utilisation(tasks, numberOfTasks, l, l);
	}
	float val=1-v1;
	float v2=0;
	for (int l=0; l<=k; l++) {
		v2+=compute_utilisation(tasks, numberOfTasks, l, l);
	}
	val=val/v2;
	return (int)val;
}

void generate_deadlines(u32 tasksDeadlines[], RTTask_t task, u32 x, u32 k) {
	u32 cumulated=0;
	for (int i=0; i<task.pxCriticalityLevel; i++) {
		u32 currDeadline;
		if (task.pxCriticalityLevel<=k) //|| k==-2)
			currDeadline=task.pxDeadline;
		else
			currDeadline=tasksDeadlines[i]=task.pxDeadline*x;

		//			if (i==0)
		//				tasksDeadlines[i]=currDeadline;
		//			else
		tasksDeadlines[i]=currDeadline-cumulated; //save increment wrt base deadline
		cumulated+=currDeadline;
	}
}

int prvSplitRTTasksList(RTTask_t prvRTTasksList[configMAX_RT_TASKS], u8 numberOfTasks,
		u8 maxTasks,
		u32 tasksTCBPtrs[configMAX_RT_TASKS],
		u32 tasksWCETs[configMAX_RT_TASKS][configCRITICALITY_LEVELS],
		u32 tasksDeadlines[configMAX_RT_TASKS][configCRITICALITY_LEVELS],
		u32 tasksPeriods[configMAX_RT_TASKS],
		u32 criticalityLevels[configMAX_RT_TASKS]) {

	int k=find_k(prvRTTasksList, numberOfTasks);
	if (k==-1) {
		printf("task set not schedulable");
		return -1;
	}

	int x;
	if (k==-2) {
		x=1;
	} else {
		x=calculate_x(prvRTTasksList, numberOfTasks, k);
	}

	for (int i = 0; i < numberOfTasks; i++) {
		tasksTCBPtrs[i]=prvRTTasksList[i].taskTCB;
		for (int j=0; j<configCRITICALITY_LEVELS; j++) {
			tasksWCETs[j][i]=prvRTTasksList[i].pxWcet[j];
		}
		generate_deadlines(tasksDeadlines[i], prvRTTasksList[i], x, k),
				tasksPeriods[i]=prvRTTasksList[i].pxPeriod;
		criticalityLevels[i]=prvRTTasksList[i].pxCriticalityLevel;
	}


	for (int i=numberOfTasks; i < maxTasks; i++) {
		tasksTCBPtrs[i]=0x0;
		for (int j=0; j<configCRITICALITY_LEVELS; j++) {
			tasksWCETs[i][j]=0xFFFFFFFF;
			tasksDeadlines[i][j]=0xFFFFFFFF;
		}
		tasksPeriods[i]=0xFFFFFFFF;
		criticalityLevels[i]=0xFFFFFFFF;
	}
	return 0;
}

/* Lint e9021, e961 and e750 are suppressed as a MISRA exception justified
 * because the MPU ports require MPU_WRAPPERS_INCLUDED_FROM_API_FILE to be defined
 * for the header files above, but not in this file, in order to generate the
 * correct privileged Vs unprivileged linkage and placement. */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE /*lint !e961 !e750 !e9021. */

/* Set configUSE_STATS_FORMATTING_FUNCTIONS to 2 to include the stats formatting
 * functions but without including stdio.h here. */
#if ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )

/* At the bottom of this file are two optional functions that can be used
 * to generate human readable text from the raw data generated by the
 * uxTaskGetSystemState() function.  Note the formatting functions are provided
 * for convenience only, and are NOT considered part of the kernel. */
#include <stdio.h>
#endif /* configUSE_STATS_FORMATTING_FUNCTIONS == 1 ) */

#if ( configUSE_PREEMPTION == 0 )

/* If the cooperative scheduler is being used then a yield should not be
 * performed just because a higher priority task has been woken. */
#define taskYIELD_IF_USING_PREEMPTION()
#else
#define taskYIELD_IF_USING_PREEMPTION()    portYIELD_WITHIN_API()
#endif

/* Values that can be assigned to the ucNotifyState member of the TCB. */
#define taskNOT_WAITING_NOTIFICATION              ( ( uint8_t ) 0 ) /* Must be zero as it is the initialised value. */
#define taskWAITING_NOTIFICATION                  ( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED                 ( ( uint8_t ) 2 )

/*
 * The value used to fill the stack of a task when the task is created.  This
 * is used purely for checking the high water mark for tasks.
 */
#define tskSTACK_FILL_BYTE                        ( 0xa5U )

/* Bits used to recored how a task's stack and TCB were allocated. */
#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB    ( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY        ( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB     ( ( uint8_t ) 2 )

/* If any of the following are set then task stacks are filled with a known
 * value so the high water mark can be determined.  If none of the following are
 * set then don't fill the stack so there is no unnecessary dependency on memset. */
#if ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )
#define tskSET_NEW_STACKS_TO_KNOWN_VALUE    1
#else
#define tskSET_NEW_STACKS_TO_KNOWN_VALUE    0
#endif

/*
 * Macros used by vListTask to indicate which state a task is in.
 */
#define tskRUNNING_CHAR      ( 'X' )
#define tskBLOCKED_CHAR      ( 'B' )
#define tskREADY_CHAR        ( 'R' )
#define tskDELETED_CHAR      ( 'D' )
#define tskSUSPENDED_CHAR    ( 'S' )

/*
 * Some kernel aware debuggers require the data the debugger needs access to be
 * global, rather than file scope.
 */
#ifdef portREMOVE_STATIC_QUALIFIER
#define static
#endif

/* The name allocated to the Idle task.  This can be overridden by defining
 * configIDLE_TASK_NAME in FreeRTOSConfig.h. */
#ifndef configIDLE_TASK_NAME
#define configIDLE_TASK_NAME    "IDLE"
#endif

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )

/* If configUSE_PORT_OPTIMISED_TASK_SELECTION is 0 then task selection is
 * performed in a generic way that is not optimised to any particular
 * microcontroller architecture. */

/* uxTopReadyPriority holds the priority of the highest priority ready
 * state task. */
#define taskRECORD_READY_PRIORITY( uxPriority ) \
		{                                               \
	if( ( uxPriority ) > uxTopReadyPriority )   \
	{                                           \
		uxTopReadyPriority = ( uxPriority );    \
	}                                           \
		} /* taskRECORD_READY_PRIORITY */

/*-----------------------------------------------------------*/

#define taskSELECT_HIGHEST_PRIORITY_TASK()                                \
		{                                                                         \
	UBaseType_t uxTopPriority = uxTopReadyPriority;                       \
	\
	/* Find the highest priority queue that contains ready tasks. */      \
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) ) \
		{                                                                     \
			configASSERT( uxTopPriority );                                    \
			--uxTopPriority;                                                  \
		}                                                                     \
		\
		/* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of \
		 * the  same priority get an equal share of the processor time. */                    \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) ); \
		uxTopReadyPriority = uxTopPriority;                                                   \
	} /* taskSELECT_HIGHEST_PRIORITY_TASK */

		/*-----------------------------------------------------------*/

		/* Define away taskRESET_READY_PRIORITY() and portRESET_READY_PRIORITY() as
		 * they are only required when a port optimised method of task selection is
		 * being used. */
#define taskRESET_READY_PRIORITY( uxPriority )
#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )

#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

		/* If configUSE_PORT_OPTIMISED_TASK_SELECTION is 1 then task selection is
		 * performed in a way that is tailored to the particular microcontroller
		 * architecture being used. */

		/* A port optimised version is provided.  Call the port defined macros. */
#define taskRECORD_READY_PRIORITY( uxPriority )    portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )

		/*-----------------------------------------------------------*/

#define taskSELECT_HIGHEST_PRIORITY_TASK()                                                  \
		{                                                                                           \
	UBaseType_t uxTopPriority;                                                              \
	\
	/* Find the highest priority list that contains ready tasks. */                         \
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );                          \
		configASSERT( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxTopPriority ] ) ) > 0 ); \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );   \
	} /* taskSELECT_HIGHEST_PRIORITY_TASK() */

		/*-----------------------------------------------------------*/

		/* A port optimised version is provided, call it only if the TCB being reset
		 * is being referenced from a ready list.  If it is referenced from a delayed
		 * or suspended list then it won't be in a ready list. */
#define taskRESET_READY_PRIORITY( uxPriority )                                                     \
		{                                                                                                  \
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 ) \
		{                                                                                              \
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );                        \
		}                                                                                              \
		}

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

		/*-----------------------------------------------------------*/

		/* pxDelayedTaskList and pxOverflowDelayedTaskList are switched when the tick
		 * count overflows. */
#define taskSWITCH_DELAYED_LISTS()                                                \
		{                                                                             \
	List_t * pxTemp;                                                          \
	\
	/* The delayed tasks list should be empty when the lists are switched. */ \
		configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList ) ) );               \
		\
		pxTemp = pxDelayedTaskList;                                               \
		pxDelayedTaskList = pxOverflowDelayedTaskList;                            \
		pxOverflowDelayedTaskList = pxTemp;                                       \
		xNumOfOverflows++;                                                        \
		prvResetNextTaskUnblockTime();                                            \
	}

		/*-----------------------------------------------------------*/

		/*
		 * Place the task represented by pxTCB into the appropriate ready list for
		 * the task.  It is inserted at the end of the list.
		 */
		//#define prvAddTaskToReadyList( pxTCB )                                                                 \
		//    traceMOVED_TASK_TO_READY_STATE( pxTCB );                                                           \
		//    taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                                                \
		//    vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
		//    tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB )

#define prvAddTaskToReadyList( pxTCB )                                                                 \
		traceMOVED_TASK_TO_READY_STATE( pxTCB );                                                           \
		taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );                                                \
		xPortSchedulerResumeTask( ( pxTCB )->uxTaskNumber );
		tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB )
		/*-----------------------------------------------------------*/

		/*
		 * Several functions take an TaskHandle_t parameter that can optionally be NULL,
		 * where NULL is used to indicate that the handle of the currently executing
		 * task should be used in place of the parameter.  This macro simply checks to
		 * see if the parameter is NULL and returns a pointer to the appropriate TCB.
		 */
#define prvGetTCBFromHandle( pxHandle )    ( ( ( pxHandle ) == NULL ) ? pxCurrentTCB : ( pxHandle ) )

		/* The item value of the event list item is normally used to hold the priority
		 * of the task to which it belongs (coded to allow it to be held in reverse
		 * priority order).  However, it is occasionally borrowed for other purposes.  It
		 * is important its value is not updated due to a task priority change while it is
		 * being used for another purpose.  The following bit definition is used to inform
		 * the scheduler that the value should not be changed - in which case it is the
		 * responsibility of whichever module is using the value to ensure it gets set back
		 * to its original value when it is released. */
#if ( configUSE_16_BIT_TICKS == 1 )
#define taskEVENT_LIST_ITEM_VALUE_IN_USE    0x8000U
#else
#define taskEVENT_LIST_ITEM_VALUE_IN_USE    0x80000000UL
#endif

		/*fedit remove
		 * Task control block.  A task control block (TCB) is allocated for each task,
		 * and stores task state information, including a pointer to the task's context
		 * (the task's run time environment, including register values)
		 */

		/*lint -save -e956 A manual analysis and inspection has been used to determine
		 * which static variables must be declared volatile. */
		PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB = NULL;

		//fmod add
		PRIVILEGED_DATA TCB_t * volatile pxIdleTCB = NULL;

		/* Lists for ready and blocked tasks. --------------------
		 * xDelayedTaskList1 and xDelayedTaskList2 could be move to function scople but
		 * doing so breaks some kernel aware debuggers and debuggers that rely on removing
		 * the static qualifier. */
		PRIVILEGED_DATA static List_t pxReadyTasksLists[configMAX_PRIORITIES]; /*< Prioritised ready tasks. */

		//fedit add
		//PRIVILEGED_DATA static RTTask_t* pxRTTasksList[ configMAX_RT_TASKS ]; /*< Created tasks. */
		//pxRTTasksList array will be located at address indicated by prvDmaSourceAddr. This address must match hardware (on Vivado) DMA source address.

		//______________data structures which will be passed to hardware scheduler (FPGA)___________________
		PRIVILEGED_DATA static RTTask_t pxRTTasksList [configMAX_RT_TASKS]; /*< Created tasks. */ //size is configMAX_RT_TASKS
		//PRIVILEGED_DATA static u32 orderedDeadlineQTaskNums [configMAX_RT_TASKS]; //tasks nums ordered by deadlines ASC
		//PRIVILEGED_DATA static u32 orderedActivationQTaskNums [configMAX_RT_TASKS]; //tasks nums ordered by activation times ASC
		//PRIVILEGED_DATA static u32 orderedDeadlineQPayload [configMAX_RT_TASKS]; //tasks deadlines ordered ASC
		//PRIVILEGED_DATA static u32 orderedActivationQPayload [configMAX_RT_TASKS]; //activation times ordered ASC
		//PRIVILEGED_DATA static u32 orderedReverseDeadlineQTaskNums [configMAX_RT_TASKS];
		//PRIVILEGED_DATA static u32 orderedReverseActivationQTaskNums [configMAX_RT_TASKS];
		//______________________________________________________________________
		PRIVILEGED_DATA static List_t xDelayedTaskList1; /*< Delayed tasks. */
		PRIVILEGED_DATA static List_t xDelayedTaskList2; /*< Delayed tasks (two lists are used - one for delays that have overflowed the current tick count. */
		PRIVILEGED_DATA static List_t * volatile pxDelayedTaskList; /*< Points to the delayed task list currently being used. */
		PRIVILEGED_DATA static List_t * volatile pxOverflowDelayedTaskList; /*< Points to the delayed task list currently being used to hold tasks that have overflowed the current tick count. */
		PRIVILEGED_DATA static List_t xPendingReadyList; /*< Tasks that have been readied while the scheduler was suspended.  They will be moved to the ready list when the scheduler is resumed. */

#if ( INCLUDE_vTaskDelete == 1 )

		PRIVILEGED_DATA static List_t xTasksWaitingTermination; /*< Tasks that have been deleted - but their memory not yet freed. */
		PRIVILEGED_DATA static volatile UBaseType_t uxDeletedTasksWaitingCleanUp =
				(UBaseType_t) 0U;

#endif

#if ( INCLUDE_vTaskSuspend == 1 )

		PRIVILEGED_DATA static List_t xSuspendedTaskList; /*< Tasks that are currently suspended. */

#endif

		/* Global POSIX errno. Its value is changed upon context switching to match
		 * the errno of the currently running task. */
#if ( configUSE_POSIX_ERRNO == 1 )
		int FreeRTOS_errno = 0;
#endif

		/* Other file private variables. --------------------------------*/
		PRIVILEGED_DATA static volatile UBaseType_t uxCurrentNumberOfTasks =
				(UBaseType_t) 0U;
		PRIVILEGED_DATA static volatile TickType_t xTickCount =
				(TickType_t) configINITIAL_TICK_COUNT;
		PRIVILEGED_DATA static volatile UBaseType_t uxTopReadyPriority =
				tskIDLE_PRIORITY;
		PRIVILEGED_DATA static volatile BaseType_t xSchedulerRunning = pdFALSE;
		PRIVILEGED_DATA static volatile TickType_t xPendedTicks = (TickType_t) 0U;
		PRIVILEGED_DATA static volatile BaseType_t xYieldPending = pdFALSE;
		PRIVILEGED_DATA static volatile BaseType_t xNumOfOverflows = (BaseType_t) 0;
		PRIVILEGED_DATA static UBaseType_t uxTaskNumber = (UBaseType_t) 0U;
		//PRIVILEGED_DATA static UBaseType_t uxRTTaskNumber = (UBaseType_t) 0U;
		PRIVILEGED_DATA static volatile TickType_t xNextTaskUnblockTime =
				(TickType_t) 0U; /* Initialised to portMAX_DELAY before the scheduler starts. */
		PRIVILEGED_DATA static TaskHandle_t xIdleTaskHandle = NULL; /*< Holds the handle of the idle task.  The idle task is created automatically when the scheduler is started. */

		/* Improve support for OpenOCD. The kernel tracks Ready tasks via priority lists.
		 * For tracking the state of remote threads, OpenOCD uses uxTopUsedPriority
		 * to determine the number of priority lists to read back from the remote target. */
		const volatile UBaseType_t uxTopUsedPriority = configMAX_PRIORITIES - 1U;

		/* Context switches are held pending while the scheduler is suspended.  Also,
		 * interrupts must not manipulate the xStateListItem of a TCB, or any of the
		 * lists the xStateListItem can be referenced from, if the scheduler is suspended.
		 * If an interrupt needs to unblock a task while the scheduler is suspended then it
		 * moves the task's event list item into the xPendingReadyList, ready for the
		 * kernel to move the task from the pending ready list into the real ready list
		 * when the scheduler is unsuspended.  The pending ready list itself can only be
		 * accessed from a critical section. */
		PRIVILEGED_DATA static volatile UBaseType_t uxSchedulerSuspended =
				(UBaseType_t) pdFALSE;

#if ( configGENERATE_RUN_TIME_STATS == 1 )

		/* Do not move these variables to function scope as doing so prevents the
		 * code working with debuggers that need to remove the static qualifier. */
		PRIVILEGED_DATA static uint32_t ulTaskSwitchedInTime = 0UL; /*< Holds the value of a timer/counter the last time a task was switched in. */
		PRIVILEGED_DATA static volatile uint32_t ulTotalRunTime = 0UL; /*< Holds the total amount of execution time as defined by the run time counter clock. */

#endif

		/*lint -restore */

		/*-----------------------------------------------------------*/

		/* File private functions. --------------------------------*/

		/**
		 * Utility task that simply returns pdTRUE if the task referenced by xTask is
		 * currently in the Suspended state, or pdFALSE if the task referenced by xTask
		 * is in any other state.
		 */
#if ( INCLUDE_vTaskSuspend == 1 )

		static BaseType_t prvTaskIsTaskSuspended(const TaskHandle_t xTask) PRIVILEGED_FUNCTION;

#endif /* INCLUDE_vTaskSuspend */

		/*
		 * Utility to ready all the lists used by the scheduler.  This is called
		 * automatically upon the creation of the first task.
		 */
		static void prvInitialiseTaskLists(void) PRIVILEGED_FUNCTION;

		/*
		 * The idle task, which as all tasks is implemented as a never ending loop.
		 * The idle task is automatically created and added to the ready lists upon
		 * creation of the first user task.
		 *
		 * The portTASK_FUNCTION_PROTO() macro is used to allow port/compiler specific
		 * language extensions.  The equivalent prototype for this function is:
		 *
		 * void prvIdleTask( void *pvParameters );
		 *
		 */
		static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters ) PRIVILEGED_FUNCTION;

		/*
		 * Utility to free all memory allocated by the scheduler to hold a TCB,
		 * including the stack pointed to by the TCB.
		 *
		 * This does not free memory allocated by the task itself (i.e. memory
		 * allocated by calls to pvPortMalloc from within the tasks application code).
		 */
#if ( INCLUDE_vTaskDelete == 1 )

		static void prvDeleteTCB(TCB_t * pxTCB) PRIVILEGED_FUNCTION;

#endif

		/*
		 * Used only by the idle task.  This checks to see if anything has been placed
		 * in the list of tasks waiting to be deleted.  If so the task is cleaned up
		 * and its TCB deleted.
		 */
		static void prvCheckTasksWaitingTermination(void) PRIVILEGED_FUNCTION;

		/*
		 * The currently executing task is entering the Blocked state.  Add the task to
		 * either the current or the overflow delayed task list.
		 */
		static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait,
				const BaseType_t xCanBlockIndefinitely) PRIVILEGED_FUNCTION;

		/*
		 * Fills an TaskStatus_t structure with information on each task that is
		 * referenced from the pxList list (which may be a ready list, a delayed list,
		 * a suspended list, etc.).
		 *
		 * THIS FUNCTION IS INTENDED FOR DEBUGGING ONLY, AND SHOULD NOT BE CALLED FROM
		 * NORMAL APPLICATION CODE.
		 */
#if ( configUSE_TRACE_FACILITY == 1 )

		static UBaseType_t prvListTasksWithinSingleList(
				TaskStatus_t * pxTaskStatusArray, List_t * pxList, eTaskState eState) PRIVILEGED_FUNCTION;

#endif

		/*
		 * Searches pxList for a task with name pcNameToQuery - returning a handle to
		 * the task if it is found, or NULL if the task is not found.
		 */
#if ( INCLUDE_xTaskGetHandle == 1 )

		static TCB_t * prvSearchForNameWithinSingleList(List_t * pxList,
				const char pcNameToQuery[]) PRIVILEGED_FUNCTION;

#endif

		u8 xTaskGetExecutionId() {
			return pxCurrentTCB->executionId;
		}

		/*
		 * When a task is created, the stack of the task is filled with a known value.
		 * This function determines the 'high water mark' of the task stack by
		 * determining how much of the stack remains at the original preset value.
		 */
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )

		static configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace(
				const uint8_t * pucStackByte) PRIVILEGED_FUNCTION;

#endif

		/*
		 * Return the amount of time, in ticks, that will pass before the kernel will
		 * next move a task from the Blocked state to the Running state.
		 *
		 * This conditional compilation should use inequality to 0, not equality to 1.
		 * This is to ensure portSUPPRESS_TICKS_AND_SLEEP() can be called when user
		 * defined low power mode implementations require configUSE_TICKLESS_IDLE to be
		 * set to a value other than 1.
		 */
#if ( configUSE_TICKLESS_IDLE != 0 )

		static TickType_t prvGetExpectedIdleTime( void ) PRIVILEGED_FUNCTION;

#endif

		/*
		 * Set xNextTaskUnblockTime to the time at which the next Blocked state task
		 * will exit the Blocked state.
		 */
		static void prvResetNextTaskUnblockTime(void) PRIVILEGED_FUNCTION;

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

		/*
		 * Helper function used to pad task names with spaces when printing out
		 * human readable tables of task information.
		 */
		static char * prvWriteNameToBuffer(char * pcBuffer, const char * pcTaskName) PRIVILEGED_FUNCTION;

#endif

		/*
		 * Called after a Task_t structure has been allocated either statically or
		 * dynamically to fill in the structure's members.
		 */
		static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
				const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const uint32_t ulStackDepth, void * const pvParameters,
				UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask,
				TCB_t * pxNewTCB, const MemoryRegion_t * const xRegions) PRIVILEGED_FUNCTION;

		//fedit add
		/*
		 * Called after a new task has been created and initialised to place the task
		 * under the control of the scheduler.
		 */
		static BaseType_t prvAddNewTaskToRTTasksList(RTTask_t pxNewRTTask) PRIVILEGED_FUNCTION;

		//fedit currently unused
		///*
		// * Called after a new task has been created and initialised to place the task
		// * under the control of the scheduler.
		// */
		static void prvAddNewTaskToReadyList(TCB_t * pxNewTCB) PRIVILEGED_FUNCTION;

		/*
		 * freertos_tasks_c_additions_init() should only be called if the user definable
		 * macro FREERTOS_TASKS_C_ADDITIONS_INIT() is defined, as that is the only macro
		 * called by the function.
		 */
#ifdef FREERTOS_TASKS_C_ADDITIONS_INIT

		static void freertos_tasks_c_additions_init( void ) PRIVILEGED_FUNCTION;

#endif

		/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

		TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
				const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const uint32_t ulStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority,
				StackType_t * const puxStackBuffer,
				StaticTask_t * const pxTaskBuffer, //fedit add
				TCB_t ** const pxTCBOut )
		{
			TCB_t * pxNewTCB;
			TaskHandle_t xReturn;

			configASSERT( puxStackBuffer != NULL );
			configASSERT( pxTaskBuffer != NULL );

#if ( configASSERT_DEFINED == 1 )
			{
				/* Sanity check that the size of the structure used to declare a
				 * variable of type StaticTask_t equals the size of the real task
				 * structure. */
				volatile size_t xSize = sizeof( StaticTask_t );
				configASSERT( xSize == sizeof( TCB_t ) );
				( void ) xSize; /* Prevent lint warning when configASSERT() is not used. */
			}
#endif /* configASSERT_DEFINED */

			if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
			{
				/* The memory used for the task's TCB and stack are passed into this
				 * function - use them. */
				pxNewTCB = ( TCB_t * ) pxTaskBuffer; /*lint !e740 !e9087 Unusual cast is ok as the structures are designed to have the same alignment, and the size is checked by an assert. */
				pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;

#if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 ) /*lint !e731 !e9029 Macro has been consolidated for readability reasons. */
				{
					/* Tasks can be created statically or dynamically, so note this
					 * task was created statically in case the task is later deleted. */
					pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
				}
#endif /* tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

				prvInitialiseNewTask( pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, &xReturn, pxNewTCB, NULL );
				//fedit add
				if (pxTCBOut != NULL) {
					*pxTCBOut = pxNewTCB;
				}
				//fedit edit
				//prvAddNewTaskToRTTasksList( pxNewTCB );
			}
			else
			{
				xReturn = NULL;
			}

			return xReturn;
		}

		TaskHandle_t xRTTaskCreateStatic( TaskFunction_t pxTaskCode,
				const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const uint32_t ulStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority,
				StackType_t * const puxStackBuffer,
				StaticTask_t * const pxTaskBuffer,  //fedit add
				//RTTask_t ** const pxRTTaskOut,
				UBaseType_t const pxDeadline,
				UBaseType_t const pxPeriod,
				UBaseType_t pxCriticalityLevel, ...
		)
		{
			//RTTask_t* pxNewRTTask = ( RTTask_t * ) pvPortMalloc( sizeof( RTTask_t ) );
			RTTask_t* pxNewRTTask

			TaskHandle_t xReturn=xTaskCreateStatic(pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, puxStackBuffer, pxTaskBuffer, &( pxNewRTTask.taskTCB ) );

			pxNewRTTask.pxDeadline=pxDeadline;
			pxNewRTTask.pxPeriod=pxPeriod;
			pxNewRTTask.pxCriticalityLevel=pxCriticalityLevel;

			va_list varptr;
			va_start(varptr, pxCriticalityLevel);
			for (UBaseType_t i = 0; i <= pxCriticalityLevel; i++)
				pxNewRTTask.pxWcet[i]=va_arg(varptr, UBaseType_t);
			va_end(varptr);

			if (xReturn!=NULL) {
				//*pxRTTaskOut=pxNewRTTask;
				if (prvAddNewTaskToRTTasksList(pxNewRTTask) != pdPass)
					return NULL;
			}
			return xReturn;
		}

#endif /* SUPPORT_STATIC_ALLOCATION */
		/*-----------------------------------------------------------*/

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) )

		BaseType_t xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
				TaskHandle_t * pxCreatedTask, //fedit add
				TCB_t ** const pxTCBOut )
		{
			TCB_t * pxNewTCB;
			BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

			configASSERT( pxTaskDefinition->puxStackBuffer != NULL );
			configASSERT( pxTaskDefinition->pxTaskBuffer != NULL );

			if( ( pxTaskDefinition->puxStackBuffer != NULL ) && ( pxTaskDefinition->pxTaskBuffer != NULL ) )
			{
				/* Allocate space for the TCB.  Where the memory comes from depends
				 * on the implementation of the port malloc function and whether or
				 * not static allocation is being used. */
				pxNewTCB = ( TCB_t * ) pxTaskDefinition->pxTaskBuffer;

				/* Store the stack location in the TCB. */
				pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

#if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
				{
					/* Tasks can be created statically or dynamically, so note this
					 * task was created statically in case the task is later deleted. */
					pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
				}
#endif /* tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

				prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
						pxTaskDefinition->pcName,
						( uint32_t ) pxTaskDefinition->usStackDepth,
						pxTaskDefinition->pvParameters,
						pxTaskDefinition->uxPriority,
						pxCreatedTask, pxNewTCB,
						pxTaskDefinition->xRegions );
				//fedit add
				if (pxTCBOut != NULL) {
					*pxTCBOut = pxNewTCB;
				}
				//fedit edit
				//prvAddNewTaskToRTTasksList( pxNewTCB );
				xReturn = pdPASS;
			}

			return xReturn;
		}

		BaseType_t xRTTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
				TaskHandle_t * pxCreatedTask, //fedit add
				//RTTask_t ** const pxRTTaskOut,
				UBaseType_t const pxDeadline,
				UBaseType_t const pxPeriod,
				UBaseType_t pxCriticalityLevel, ...	)
		{
			//RTTask_t* pxNewRTTask = ( RTTask_t * ) pvPortMalloc( sizeof( RTTask_t ) );
			RTTask_t pxNewRTTask;
			BaseType_t xReturn=xRTTaskCreateRestrictedStatic(pxTaskDefinition, pxCreatedTask, &( pxNewRTTask.taskTCB ) );

			pxNewRTTask.pxDeadline=pxDeadline;
			pxNewRTTask.pxPeriod=pxPeriod;
			pxNewRTTask.pxCriticalityLevel=pxCriticalityLevel;

			va_list varptr;
			va_start(varptr, pxCriticalityLevel+1);
			for (UBaseType_t i = 0; i <= pxCriticalityLevel; i++)
				pxNewRTTask.pxWcet[i]=va_arg(varptr, UBaseType_t);
			va_end(varptr);

			if (xReturn==pdPASS) {
				//*pxRTTaskOut=pxNewRTTask;
				return prvAddNewTaskToRTTasksList(pxNewRTTask);
			}

			return xReturn;
		}

#endif /* ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
		/*-----------------------------------------------------------*/

#if ( ( portUSING_MPU_WRAPPERS == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

		BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
				TaskHandle_t * pxCreatedTask, //fedit add
				TCB_t ** const pxTCBOut )
		{
			TCB_t * pxNewTCB;
			BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

			configASSERT( pxTaskDefinition->puxStackBuffer );

			if( pxTaskDefinition->puxStackBuffer != NULL )
			{
				/* Allocate space for the TCB.  Where the memory comes from depends
				 * on the implementation of the port malloc function and whether or
				 * not static allocation is being used. */
				pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

				if( pxNewTCB != NULL )
				{
					/* Store the stack location in the TCB. */
					pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

#if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
					{
						/* Tasks can be created statically or dynamically, so note
						 * this task had a statically allocated stack in case it is
						 * later deleted.  The TCB was allocated dynamically. */
						pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_ONLY;
					}
#endif /* tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

					prvInitialiseNewTask( pxTaskDefinition->pvTaskCode,
							pxTaskDefinition->pcName,
							( uint32_t ) pxTaskDefinition->usStackDepth,
							pxTaskDefinition->pvParameters,
							pxTaskDefinition->uxPriority,
							pxCreatedTask, pxNewTCB,
							pxTaskDefinition->xRegions );
					//fedit add
					if (pxTCBOut != NULL) {
						*pxTCBOut = pxNewTCB;
					}
					//fedit edit
					//prvAddNewTaskToRTTasksList( pxNewTCB );
					xReturn = pdPASS;
				}
			}

			return xReturn;
		}

		BaseType_t xRTTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
				TaskHandle_t * pxCreatedTask, //fedit add
				//RTTask_t ** const pxRTTaskOut,
				UBaseType_t const pxDeadline,
				UBaseType_t const pxPeriod,
				UBaseType_t pxCriticalityLevel, ...	)
		{
			//RTTask_t* pxNewRTTask = ( RTTask_t * ) pvPortMalloc( sizeof( RTTask_t ) );
			RTTask_t pxNewRTTask;

			BaseType_t xReturn=xTaskCreateRestricted(pxTaskDefinition, pxCreatedTask, &( pxNewRTTask.taskTCB ) );

			pxNewRTTask.pxDeadline=pxDeadline;
			pxNewRTTask.pxPeriod=pxPeriod;
			pxNewRTTask.pxCriticalityLevel=pxCriticalityLevel;

			va_list varptr;
			va_start(varptr, pxCriticalityLevel+1);
			for (UBaseType_t i = 0; i <= pxCriticalityLevel; i++)
				pxNewRTTask.pxWcet[i]=va_arg(varptr, UBaseType_t);
			va_end(varptr);

			if (xReturn==pdPASS) {
				//*pxRTTaskOut=pxNewRTTask;
				return prvAddNewTaskToRTTasksList(pxNewRTTask);
			}

			return xReturn;
		}

#endif /* portUSING_MPU_WRAPPERS */
		/*-----------------------------------------------------------*/

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

		BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters,
				UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask, //fedit add
				TCB_t ** const pxTCBOut) {
			TCB_t * pxNewTCB;
			BaseType_t xReturn;

			/* If the stack grows down then allocate the stack then the TCB so the stack
			 * does not grow into the TCB.  Likewise if the stack grows up then allocate
			 * the TCB then the stack. */
#if ( portSTACK_GROWTH > 0 )
			{
				/* Allocate space for the TCB.  Where the memory comes from depends on
				 * the implementation of the port malloc function and whether or not static
				 * allocation is being used. */
				pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

				if( pxNewTCB != NULL )
				{
					/* Allocate space for the stack used by the task being created.
					 * The base of the stack memory stored in the TCB so the task can
					 * be deleted later if required. */
					pxNewTCB->pxStack = ( StackType_t * ) pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

					if( pxNewTCB->pxStack == NULL )
					{
						/* Could not allocate the stack.  Delete the allocated TCB. */
						vPortFree( pxNewTCB );
						pxNewTCB = NULL;
					}
				}
			}
#else /* portSTACK_GROWTH */
			{
				StackType_t * pxStack;

				/* Allocate space for the stack used by the task being created. */
				pxStack = pvPortMalloc((((size_t) usStackDepth) * sizeof(StackType_t))); /*lint !e9079 All values returned by pvPortMalloc() have at least the alignment required by the MCU's stack and this allocation is the stack. */

				if (pxStack != NULL) {
					/* Allocate space for the TCB. */
					pxNewTCB = (TCB_t *) pvPortMalloc(sizeof(TCB_t)); /*lint !e9087 !e9079 All values returned by pvPortMalloc() have at least the alignment required by the MCU's stack, and the first member of TCB_t is always a pointer to the task's stack. */

					if (pxNewTCB != NULL) {
						/* Store the stack location in the TCB. */
						pxNewTCB->pxStack = pxStack;
					} else {
						/* The stack cannot be used as the TCB was not created.  Free
						 * it again. */
						vPortFree(pxStack);
					}
				} else {
					pxNewTCB = NULL;
				}
			}
#endif /* portSTACK_GROWTH */

			if (pxNewTCB != NULL) {
#if ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 ) /*lint !e9029 !e731 Macro has been consolidated for readability reasons. */
				{
					/* Tasks can be created statically or dynamically, so note this
					 * task was created dynamically in case it is later deleted. */
					pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;
				}
#endif /* tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE */

				prvInitialiseNewTask(pxTaskCode, pcName, (uint32_t) usStackDepth,
						pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL);
				//fedit add
				if (pxTCBOut != NULL) {
					*pxTCBOut = pxNewTCB;
				}
				//fedit edit
				//prvAddNewTaskToRTTasksList( pxNewTCB );
				xReturn = pdPASS;
			} else {
				xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
			}

			return xReturn;
		}

		BaseType_t xRTTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters,
				UBaseType_t uxPriority,
				TaskHandle_t * const pxCreatedTask, //fedit add
				//RTTask_t ** const pxRTTaskOut,
				UBaseType_t const pxDeadline, UBaseType_t const pxPeriod,
				UBaseType_t pxCriticalityLevel, ...) {
			//RTTask_t* pxNewRTTask = ( RTTask_t * ) pvPortMalloc( sizeof( RTTask_t ) );
			RTTask_t pxNewRTTask;

			BaseType_t xReturn = xTaskCreate(pxTaskCode, pcName, usStackDepth,
					pvParameters, uxPriority, pxCreatedTask, &(pxNewRTTask.taskTCB));

			pxNewRTTask.pxDeadline = pxDeadline;
			pxNewRTTask.pxPeriod = pxPeriod;
			pxNewRTTask.pxCriticalityLevel=pxCriticalityLevel;

			va_list varptr;
			va_start(varptr, pxCriticalityLevel+1);
			for (UBaseType_t i = 0; i <= pxCriticalityLevel; i++)
				pxNewRTTask.pxWcet[i]=va_arg(varptr, UBaseType_t);
			va_end(varptr);
			if (xReturn == pdPASS) {
				//*pxRTTaskOut=pxNewRTTask;
				return prvAddNewTaskToRTTasksList(pxNewRTTask);
			}

			return xReturn;
		}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
		/*-----------------------------------------------------------*/

		static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
				const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
				const uint32_t ulStackDepth, void * const pvParameters,
				UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask,
				TCB_t * pxNewTCB, const MemoryRegion_t * const xRegions) {
			StackType_t * pxTopOfStack;
			UBaseType_t x;

#if ( portUSING_MPU_WRAPPERS == 1 )
			/* Should the task be created in privileged mode? */
			BaseType_t xRunPrivileged;

			if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
			{
				xRunPrivileged = pdTRUE;
			}
			else
			{
				xRunPrivileged = pdFALSE;
			}
			uxPriority &= ~portPRIVILEGE_BIT;
#endif /* portUSING_MPU_WRAPPERS == 1 */

			/* Avoid dependency on memset() if it is not required. */
#if ( tskSET_NEW_STACKS_TO_KNOWN_VALUE == 1 )
			{
				/* Fill the stack with a known value to assist debugging. */
				(void) memset(pxNewTCB->pxStack, (int) tskSTACK_FILL_BYTE,
						(size_t) ulStackDepth * sizeof(StackType_t));
			}
#endif /* tskSET_NEW_STACKS_TO_KNOWN_VALUE */

			/* Calculate the top of stack address.  This depends on whether the stack
			 * grows from high memory to low (as per the 80x86) or vice versa.
			 * portSTACK_GROWTH is used to make the result positive or negative as required
			 * by the port. */
#if ( portSTACK_GROWTH < 0 )
			{
				pxTopOfStack = &(pxNewTCB->pxStack[ulStackDepth - (uint32_t) 1]);
				pxTopOfStack = (StackType_t *) ((( portPOINTER_SIZE_TYPE ) pxTopOfStack)
						& (~(( portPOINTER_SIZE_TYPE) portBYTE_ALIGNMENT_MASK))); /*lint !e923 !e9033 !e9078 MISRA exception.  Avoiding casts between pointers and integers is not practical.  Size differences accounted for using portPOINTER_SIZE_TYPE type.  Checked by assert(). */

				/* Check the alignment of the calculated top of stack is correct. */
				configASSERT(
						( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ));

#if ( configRECORD_STACK_HIGH_ADDRESS == 1 )
				{
					/* Also record the stack's high address, which may assist
					 * debugging. */
					pxNewTCB->pxEndOfStack = pxTopOfStack;
				}
#endif /* configRECORD_STACK_HIGH_ADDRESS */
			}
#else /* portSTACK_GROWTH */
			{
				pxTopOfStack = pxNewTCB->pxStack;

				/* Check the alignment of the stack buffer is correct. */
				configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxNewTCB->pxStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

				/* The other extreme of the stack space is required if stack checking is
				 * performed. */
				pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
			}
#endif /* portSTACK_GROWTH */

			/* Store the task name in the TCB. */
			if (pcName != NULL) {
				for (x = (UBaseType_t) 0; x < (UBaseType_t) configMAX_TASK_NAME_LEN;
						x++) {
					pxNewTCB->pcTaskName[x] = pcName[x];

					/* Don't copy all configMAX_TASK_NAME_LEN if the string is shorter than
					 * configMAX_TASK_NAME_LEN characters just in case the memory after the
					 * string is not accessible (extremely unlikely). */
					if (pcName[x] == (char) 0x00) {
						break;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}

				/* Ensure the name string is terminated in the case that the string length
				 * was greater or equal to configMAX_TASK_NAME_LEN. */
				pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1] = '\0';
			} else {
				/* The task has not been given a name, so just ensure there is a NULL
				 * terminator when it is read out. */
				pxNewTCB->pcTaskName[0] = 0x00;
			}

			/* This is used as an array index so must ensure it's not too large.  First
			 * remove the privilege bit if one is present. */
			if (uxPriority >= (UBaseType_t) configMAX_PRIORITIES) {
				uxPriority = (UBaseType_t) configMAX_PRIORITIES - (UBaseType_t) 1U;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			pxNewTCB->uxPriority = uxPriority;
#if ( configUSE_MUTEXES == 1 )
			{
				pxNewTCB->uxBasePriority = uxPriority;
				pxNewTCB->uxMutexesHeld = 0;
			}
#endif /* configUSE_MUTEXES */

			vListInitialiseItem(&(pxNewTCB->xStateListItem));
			vListInitialiseItem(&(pxNewTCB->xEventListItem));

			/* Set the pxNewTCB as a link back from the ListItem_t.  This is so we can get
			 * back to  the containing TCB from a generic item in a list. */
			listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);

			/* Event lists are always in priority order. */
			listSET_LIST_ITEM_VALUE(&(pxNewTCB->xEventListItem),
					( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
			listSET_LIST_ITEM_OWNER(&(pxNewTCB->xEventListItem), pxNewTCB);

#if ( portCRITICAL_NESTING_IN_TCB == 1 )
			{
				pxNewTCB->uxCriticalNesting = ( UBaseType_t ) 0U;
			}
#endif /* portCRITICAL_NESTING_IN_TCB */

#if ( configUSE_APPLICATION_TASK_TAG == 1 )
			{
				pxNewTCB->pxTaskTag = NULL;
			}
#endif /* configUSE_APPLICATION_TASK_TAG */

#if ( configGENERATE_RUN_TIME_STATS == 1 )
			{
				pxNewTCB->ulRunTimeCounter = 0UL;
			}
#endif /* configGENERATE_RUN_TIME_STATS */

#if ( portUSING_MPU_WRAPPERS == 1 )
			{
				vPortStoreTaskMPUSettings( &( pxNewTCB->xMPUSettings ), xRegions, pxNewTCB->pxStack, ulStackDepth );
			}
#else
			{
				/* Avoid compiler warning about unreferenced parameter. */
				(void) xRegions;
			}
#endif

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
			{
				memset( ( void * ) &( pxNewTCB->pvThreadLocalStoragePointers[ 0 ] ), 0x00, sizeof( pxNewTCB->pvThreadLocalStoragePointers ) );
			}
#endif

#if ( configUSE_TASK_NOTIFICATIONS == 1 )
			{
				memset((void *) &(pxNewTCB->ulNotifiedValue[0]), 0x00,
						sizeof(pxNewTCB->ulNotifiedValue));
				memset((void *) &(pxNewTCB->ucNotifyState[0]), 0x00,
						sizeof(pxNewTCB->ucNotifyState));
			}
#endif

#if ( configUSE_NEWLIB_REENTRANT == 1 )
			{
				/* Initialise this task's Newlib reent structure.
				 * See the third party link http://www.nadler.com/embedded/newlibAndFreeRTOS.html
				 * for additional information. */
				_REENT_INIT_PTR( ( &( pxNewTCB->xNewLib_reent ) ) );
			}
#endif

#if ( INCLUDE_xTaskAbortDelay == 1 )
			{
				pxNewTCB->ucDelayAborted = pdFALSE;
			}
#endif

			//fedit add
			pxNewTCB->pxInitTopOfStack=pxNewTCB->pxTopOfStack;


			/* Initialize the TCB stack to look as if the task was already running,
			 * but had been interrupted by the scheduler.  The return address is set
			 * to the start of the task function. Once the stack has been initialised
			 * the top of stack variable is updated. */
#if ( portUSING_MPU_WRAPPERS == 1 )
			{
				/* If the port has capability to detect stack overflow,
				 * pass the stack end address to the stack initialization
				 * function as well. */
#if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
				{
#if ( portSTACK_GROWTH < 0 )
					{
						pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters, xRunPrivileged );
					}
#else /* portSTACK_GROWTH */
					{
						pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters, xRunPrivileged );
					}
#endif /* portSTACK_GROWTH */
				}
#else /* portHAS_STACK_OVERFLOW_CHECKING */
				{
					pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged );
				}
#endif /* portHAS_STACK_OVERFLOW_CHECKING */
			}
#else /* portUSING_MPU_WRAPPERS */
			{
				/* If the port has capability to detect stack overflow,
				 * pass the stack end address to the stack initialization
				 * function as well. */
#if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
				{
#if ( portSTACK_GROWTH < 0 )
					{
						pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxStack, pxTaskCode, pvParameters );
					}
#else /* portSTACK_GROWTH */
					{
						pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxNewTCB->pxEndOfStack, pxTaskCode, pvParameters );
					}
#endif /* portSTACK_GROWTH */
				}
#else /* portHAS_STACK_OVERFLOW_CHECKING */
				{
					pxNewTCB->pxTopOfStack = pxPortInitialiseStack(pxTopOfStack,
							pxTaskCode, pvParameters);
				}
#endif /* portHAS_STACK_OVERFLOW_CHECKING */
			}
#endif /* portUSING_MPU_WRAPPERS */

			//fedit add
			pxNewTCB->pxInitTaskCode=pxTaskCode;
			pxNewTCB->pxInitParameters=(StackType_t)pvParameters;

			pxNewTCB->reExecutions=0;
			pxNewTCB->executionMode=0;
			pxNewTCB->lastError.checkId=0xFF;
			pxNewTCB->lastError.uniId=0xFFFF;
			//end of fedit add

			if (pxCreatedTask != NULL) {
				/* Pass the handle out in an anonymous way.  The handle can be used to
				 * change the created task's priority, delete the created task, etc.*/
				*pxCreatedTask = (TaskHandle_t) pxNewTCB;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}
		/*-----------------------------------------------------------*/
		//fedit currently unused
		//static void prvAddNewTaskToReadyList(TCB_t * pxNewTCB) {
		//	/* Ensure interrupts don't access the task lists while the lists are being
		//	 * updated. */
		//	taskENTER_CRITICAL()
		//	;
		//	{
		//		uxCurrentNumberOfTasks++;
		//
		//		if (pxCurrentTCB == NULL) {
		//			/* There are no other tasks, or all the other tasks are in
		//			 * the suspended state - make this the current task. */
		//			pxCurrentTCB = pxNewTCB;
		//
		//			if (uxCurrentNumberOfTasks == (UBaseType_t) 1) {
		//				/* This is the first task to be created so do the preliminary
		//				 * initialisation required.  We will not recover if this call
		//				 * fails, but we will report the failure. */
		//				prvInitialiseTaskLists();
		//			} else {
		//				mtCOVERAGE_TEST_MARKER();
		//			}
		//		} else {
		//			/* If the scheduler is not already running, make this task the
		//			 * current task if it is the highest priority task to be created
		//			 * so far. */
		//			if (xSchedulerRunning == pdFALSE) {
		//				if (pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority) {
		//					pxCurrentTCB = pxNewTCB;
		//				} else {
		//					mtCOVERAGE_TEST_MARKER();
		//				}
		//			} else {
		//				mtCOVERAGE_TEST_MARKER();
		//			}
		//		}
		//
		//		uxTaskNumber++;
		//
		//#if ( configUSE_TRACE_FACILITY == 1 )
		//		{
		//			/* Add a counter into the TCB for tracing only. */
		//			pxNewTCB->uxTCBNumber = uxTaskNumber;
		//		}
		//#endif /* configUSE_TRACE_FACILITY */
		//		traceTASK_CREATE( pxNewTCB );
		//
		//		prvAddTaskToReadyList(pxNewTCB);
		//
		//		portSETUP_TCB(pxNewTCB);
		//	}
		//	taskEXIT_CRITICAL()
		//	;
		//
		//	if (xSchedulerRunning != pdFALSE) {
		//		/* If the created task is of a higher priority than the current task
		//		 * then it should run now. */
		//		if (pxCurrentTCB->uxPriority < pxNewTCB->uxPriority) {
		//			taskYIELD_IF_USING_PREEMPTION()
		//			;
		//		} else {
		//			mtCOVERAGE_TEST_MARKER();
		//		}
		//	} else {
		//		mtCOVERAGE_TEST_MARKER();
		//	}
		//}
		/*-----------------------------------------------------------*/

		//fedit add
		static BaseType_t prvAddNewTaskToRTTasksList(RTTask_t pxNewRTTask) {
			TCB_t* pxNewTCB = pxNewRTTask.taskTCB;
			if (xSchedulerRunning == pdFALSE) {
				/* Ensure interrupts don't access the task lists while the lists are being
				 * updated. */
				taskENTER_CRITICAL()
																				;
				{
					uxCurrentNumberOfTasks++;

					if (pxCurrentTCB == NULL) {
						/* There are no other tasks - make this the current task. */
						//pxCurrentTCB = pxNewTCB;
						if (uxCurrentNumberOfTasks == (UBaseType_t) 1) {
							/* This is the first task to be created so do the preliminary
							 * initialisation required.  We will not recover if this call
							 * fails, but we will report the failure. */
							prvInitialiseTaskLists();
						} else {
							mtCOVERAGE_TEST_MARKER();
						}
					} else {
						mtCOVERAGE_TEST_MARKER();
					}

					uxTaskNumber++;

#if ( configUSE_TRACE_FACILITY == 1 )
					{
						/* Add a counter into the TCB for tracing only. */
						pxNewTCB->uxTCBNumber = uxTaskNumber;
					}
#endif /* configUSE_TRACE_FACILITY */
					traceTASK_CREATE( pxNewTCB );

					//fedit add
					//pxNewRTTask.uxTaskNumber = uxTaskNumber;
					pxNewTCB->uxTaskNumber = uxTaskNumber;

					//TODO STATIC INSTEAD OF POINTER!
					//memcpy(pxRTTasksList+sizeof(*pxNewRTTask)*(uxTaskNumber-1), pxNewRTTask, sizeof(*pxNewRTTask));

					pxRTTasksList[uxTaskNumber-1] = pxNewRTTask;
					//uxRTTaskNumber++;

					portSETUP_TCB(pxNewTCB);
				}
				taskEXIT_CRITICAL()
				;
				//printf("created tasks array address: %p", pxRTTasksList);
				return pdPASS;
			} else {
				/* Error, stasks must be added before scheduler startup */
				return errSCHEDULER_ALREADY_RUNNING;
			}
		}
		/*-----------------------------------------------------------*/

		void vTaskJobEnd() { //(TaskHandle_t xTaskToEndJob) {
			//TCB_t* pxTCB;

			//taskENTER_CRITICAL()
			//	;
			//	{
			/* If null is passed in here then it is the running task that is
			 * being suspended. */
			//pxTCB = prvGetTCBFromHandle(xTaskToEndJob);
			//pxTCB=pxCurrentTCB;

			//blockIfFaultDetectedInTask();

			//		printf(" end ");
			pxCurrentTCB->executionMode=EXECMODE_NORMAL;
			pxCurrentTCB->jobEnded=1;

			xPortSchedulerSignalJobEnded(pxCurrentTCB->uxTaskNumber, pxCurrentTCB->executionId);
			//printf(" JOBEND SENT ");

			//if (pxTCB == pxCurrentTCB) {
			//		portYIELD_WITHIN_API()
			//			;
			//		}
			//	taskEXIT_CRITICAL()
			//		;

			while(pxCurrentTCB->jobEnded) {
			}
		}

		//void vTaskJobEnd(TaskHandle_t xTaskToEndJob) {
		//	//TCB_t* pxTCB;
		//
		//	//taskENTER_CRITICAL()
		//	//	;
		//	//{
		//	//	pxTCB = prvGetTCBFromHandle(xTaskToEndJob);
		//
		//	//	traceTASK_SUSPEND(pxTCB);
		//
		//	//	xPortSchedulerSignalJobEnded(pxTCB->uxTaskNumber);
		//
		//
		//	//	/* Is the task waiting on an event also? */
		//	//	if (listLIST_ITEM_CONTAINER(&(pxTCB->xEventListItem)) != NULL) {
		//	//		(void)uxListRemove(&(pxTCB->xEventListItem));
		//	//	}
		//	//	else {
		//	//		mtCOVERAGE_TEST_MARKER();
		//	//	}
		//	//}
		//	//taskEXIT_CRITICAL()
		//	//	;
		//
		//
		//	//if (pxTCB == pxCurrentTCB) {
		//	//	portYIELD_WITHIN_API();
		//	//}
		//	TCB_t* pxTCB;
		//
		//	taskENTER_CRITICAL()
		//		;
		//	{
		//		/* If null is passed in here then it is the running task that is
		//		 * being suspended. */
		//		pxTCB = prvGetTCBFromHandle(xTaskToEndJob);
		//
		//		traceTASK_SUSPEND(pxTCB);
		//
		//		/* Is the task waiting on an event also? */
		//		if (listLIST_ITEM_CONTAINER(&(pxTCB->xEventListItem)) != NULL) {
		//			(void)uxListRemove(&(pxTCB->xEventListItem));
		//		}
		//		else {
		//			mtCOVERAGE_TEST_MARKER();
		//		}
		//
		//		xPortSchedulerSignalJobEnded(pxTCB->uxTaskNumber);
		//
		//#if ( configUSE_TASK_NOTIFICATIONS == 1 )
		//		{
		//			BaseType_t x;
		//
		//			for (x = 0; x < configTASK_NOTIFICATION_ARRAY_ENTRIES; x++) {
		//				if (pxTCB->ucNotifyState[x] == taskWAITING_NOTIFICATION) {
		//					/* The task was blocked to wait for a notification, but is
		//					 * now suspended, so no notification was received. */
		//					pxTCB->ucNotifyState[x] = taskNOT_WAITING_NOTIFICATION;
		//				}
		//			}
		//		}
		//#endif /* if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
		//	}
		//	taskEXIT_CRITICAL()
		//		;
		//
		//	//if (xSchedulerRunning != pdFALSE) {
		//	//	/* Reset the next expected unblock time in case it referred to the
		//	//	 * task that is now in the Suspended state. */
		//	//	taskENTER_CRITICAL()
		//	//		;
		//	//	{
		//	//		prvResetNextTaskUnblockTime();
		//	//	}
		//	//	taskEXIT_CRITICAL()
		//	//		;
		//	//}
		//	//else {
		//	//	mtCOVERAGE_TEST_MARKER();
		//	//}
		//
		//	if (pxTCB == pxCurrentTCB) {
		//		if (xSchedulerRunning != pdFALSE) {
		//			/* The current task has just been suspended. */
		//			configASSERT(uxSchedulerSuspended == 0);
		//			portYIELD_WITHIN_API()
		//				;
		//		}
		//		else {
		//			/* The scheduler is not running */
		//			vTaskSwitchContext();
		//		}
		//	}
		//	else {
		//		mtCOVERAGE_TEST_MARKER();
		//	}
		//}

#if ( INCLUDE_vTaskDelete == 1 )

		void vTaskDelete(TaskHandle_t xTaskToDelete) {
			TCB_t* pxTCB;

			taskENTER_CRITICAL()
			;
			{
				/* If null is passed in here then it is the calling task that is
				 * being deleted. */
				pxTCB = prvGetTCBFromHandle(xTaskToDelete);

				xPortSchedulerSignalTaskEnded(pxTCB->uxTaskNumber);

				//		/* Remove task from the ready/delayed list. */
				//		if (uxListRemove(&(pxTCB->xStateListItem)) == (UBaseType_t) 0) {
				//			taskRESET_READY_PRIORITY(pxTCB->uxPriority);
				//		} else {
				//			mtCOVERAGE_TEST_MARKER();
				//		}

				/* Is the task waiting on an event also? */
				if (listLIST_ITEM_CONTAINER(&(pxTCB->xEventListItem)) != NULL) {
					(void)uxListRemove(&(pxTCB->xEventListItem));
				}
				else {
					mtCOVERAGE_TEST_MARKER();
				}

				/* Increment the uxTaskNumber also so kernel aware debuggers can
				 * detect that the task lists need re-generating.  This is done before
				 * portPRE_TASK_DELETE_HOOK() as in the Windows port that macro will
				 * not return. */
				uxTaskNumber++;

				if (pxTCB == pxCurrentTCB) {
					/* A task is deleting itself.  This cannot complete within the
					 * task itself, as a context switch to another task is required.
					 * Place the task in the termination list.  The idle task will
					 * check the termination list and free up any memory allocated by
					 * the scheduler for the TCB and stack of the deleted task. */
					vListInsertEnd(&xTasksWaitingTermination, &(pxTCB->xStateListItem));

					/* Increment the ucTasksDeleted variable so the idle task knows
					 * there is a task that has been deleted and that it should therefore
					 * check the xTasksWaitingTermination list. */
					++uxDeletedTasksWaitingCleanUp;

					/* Call the delete hook before portPRE_TASK_DELETE_HOOK() as
					 * portPRE_TASK_DELETE_HOOK() does not return in the Win32 port. */
					traceTASK_DELETE(pxTCB);

					/* The pre-delete hook is primarily for the Windows simulator,
					 * in which Windows specific clean up operations are performed,
					 * after which it is not possible to yield away from this task -
					 * hence xYieldPending is used to latch that a context switch is
					 * required. */
					portPRE_TASK_DELETE_HOOK(pxTCB, &xYieldPending);
				}
				else {
					--uxCurrentNumberOfTasks;
					traceTASK_DELETE(pxTCB);
					prvDeleteTCB(pxTCB);

					///* Reset the next expected unblock time in case it referred to
					// * the task that has just been deleted. */
					//prvResetNextTaskUnblockTime();
				}
			}
			taskEXIT_CRITICAL()
			;

			/* Force a reschedule if it is the currently running task that has just
			 * been deleted. */
			if (xSchedulerRunning != pdFALSE) {
				if (pxTCB == pxCurrentTCB) {
					configASSERT(uxSchedulerSuspended == 0);
					portYIELD_WITHIN_API()
					;
				}
				else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}

#endif /* INCLUDE_vTaskDelete */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskDelayUntil == 1 )

		BaseType_t xTaskDelayUntil(TickType_t * const pxPreviousWakeTime,
				const TickType_t xTimeIncrement) {
			TickType_t xTimeToWake;
			BaseType_t xAlreadyYielded, xShouldDelay = pdFALSE;

			configASSERT(pxPreviousWakeTime);
			configASSERT((xTimeIncrement > 0U));
			configASSERT(uxSchedulerSuspended == 0);

			vTaskSuspendAll();
			{
				/* Minor optimisation.  The tick count cannot change in this
				 * block. */
				const TickType_t xConstTickCount = xTickCount;

				/* Generate the tick time at which the task wants to wake. */
				xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

				if (xConstTickCount < *pxPreviousWakeTime) {
					/* The tick count has overflowed since this function was
					 * lasted called.  In this case the only time we should ever
					 * actually delay is if the wake time has also  overflowed,
					 * and the wake time is greater than the tick time.  When this
					 * is the case it is as if neither time had overflowed. */
					if ((xTimeToWake < *pxPreviousWakeTime)
							&& (xTimeToWake > xConstTickCount)) {
						xShouldDelay = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					/* The tick time has not overflowed.  In this case we will
					 * delay if either the wake time has overflowed, and/or the
					 * tick time is less than the wake time. */
					if ((xTimeToWake < *pxPreviousWakeTime)
							|| (xTimeToWake > xConstTickCount)) {
						xShouldDelay = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}

				/* Update the wake time ready for the next call. */
				*pxPreviousWakeTime = xTimeToWake;

				if (xShouldDelay != pdFALSE) {
					traceTASK_DELAY_UNTIL( xTimeToWake );

					/* prvAddCurrentTaskToDelayedList() needs the block time, not
					 * the time to wake, so subtract the current tick count. */
					prvAddCurrentTaskToDelayedList(xTimeToWake - xConstTickCount,
							pdFALSE);
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			xAlreadyYielded = xTaskResumeAll();

			/* Force a reschedule if xTaskResumeAll has not already done so, we may
			 * have put ourselves to sleep. */
			if (xAlreadyYielded == pdFALSE) {
				portYIELD_WITHIN_API()
																				;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return xShouldDelay;
		}

#endif /* INCLUDE_xTaskDelayUntil */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelay == 1 )

		void vTaskDelay(const TickType_t xTicksToDelay) {
			BaseType_t xAlreadyYielded = pdFALSE;

			/* A delay time of zero just forces a reschedule. */
			if (xTicksToDelay > (TickType_t) 0U) {
				configASSERT(uxSchedulerSuspended == 0);
				vTaskSuspendAll();
				{
					traceTASK_DELAY();

					/* A task that is removed from the event list while the
					 * scheduler is suspended will not get placed in the ready
					 * list or removed from the blocked list until the scheduler
					 * is resumed.
					 *
					 * This task cannot be in an event list as it is the currently
					 * executing task. */
					prvAddCurrentTaskToDelayedList(xTicksToDelay, pdFALSE);
				}
				xAlreadyYielded = xTaskResumeAll();
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			/* Force a reschedule if xTaskResumeAll has not already done so, we may
			 * have put ourselves to sleep. */
			if (xAlreadyYielded == pdFALSE) {
				portYIELD_WITHIN_API()
																				;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* INCLUDE_vTaskDelay */
		/*-----------------------------------------------------------*/

#if ( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_xTaskAbortDelay == 1 ) )

		eTaskState eTaskGetState(TaskHandle_t xTask) {
			eTaskState eReturn;
			List_t const * pxStateList, *pxDelayedList, *pxOverflowedDelayedList;
			const TCB_t * const pxTCB = xTask;

			configASSERT(pxTCB);

			if (pxTCB == pxCurrentTCB) {
				/* The task calling this function is querying its own state. */
				eReturn = eRunning;
			} else {
				taskENTER_CRITICAL()
																				;
				{
					pxStateList = listLIST_ITEM_CONTAINER(&(pxTCB->xStateListItem));
					pxDelayedList = pxDelayedTaskList;
					pxOverflowedDelayedList = pxOverflowDelayedTaskList;
				}
				taskEXIT_CRITICAL()
				;

				if ((pxStateList == pxDelayedList)
						|| (pxStateList == pxOverflowedDelayedList)) {
					/* The task being queried is referenced from one of the Blocked
					 * lists. */
					eReturn = eBlocked;
				}

#if ( INCLUDE_vTaskSuspend == 1 )
				else if (pxStateList == &xSuspendedTaskList) {
					/* The task being queried is referenced from the suspended
					 * list.  Is it genuinely suspended or is it blocked
					 * indefinitely? */
					if ( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL) {
#if ( configUSE_TASK_NOTIFICATIONS == 1 )
						{
							BaseType_t x;

							/* The task does not appear on the event list item of
							 * and of the RTOS objects, but could still be in the
							 * blocked state if it is waiting on its notification
							 * rather than waiting on an object.  If not, is
							 * suspended. */
							eReturn = eSuspended;

							for (x = 0; x < configTASK_NOTIFICATION_ARRAY_ENTRIES;
									x++) {
								if (pxTCB->ucNotifyState[x] == taskWAITING_NOTIFICATION) {
									eReturn = eBlocked;
									break;
								}
							}
						}
#else /* if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
						{
							eReturn = eSuspended;
						}
#endif /* if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
					} else {
						eReturn = eBlocked;
					}
				}
#endif /* if ( INCLUDE_vTaskSuspend == 1 ) */

#if ( INCLUDE_vTaskDelete == 1 )
				else if ((pxStateList == &xTasksWaitingTermination)
						|| (pxStateList == NULL)) {
					/* The task being queried is referenced from the deleted
					 * tasks list, or it is not referenced from any lists at
					 * all. */
					eReturn = eDeleted;
				}
#endif

				else /*lint !e525 Negative indentation is intended to make use of pre-processor clearer. */
				{
					/* If the task is not in any other state, it must be in the
					 * Ready (including pending ready) state. */
					eReturn = eReady;
				}
			}

			return eReturn;
		} /*lint !e818 xTask cannot be a pointer to const because it is a typedef. */

#endif /* INCLUDE_eTaskGetState */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

		UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask) {
			TCB_t const * pxTCB;
			UBaseType_t uxReturn;

			taskENTER_CRITICAL()
			;
			{
				/* If null is passed in here then it is the priority of the task
				 * that called uxTaskPriorityGet() that is being queried. */
				pxTCB = prvGetTCBFromHandle(xTask);
				uxReturn = pxTCB->uxPriority;
			}
			taskEXIT_CRITICAL()
			;

			return uxReturn;
		}

#endif /* INCLUDE_uxTaskPriorityGet */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

		UBaseType_t uxTaskPriorityGetFromISR(const TaskHandle_t xTask) {
			TCB_t const * pxTCB;
			UBaseType_t uxReturn, uxSavedInterruptState;

			/* RTOS ports that support interrupt nesting have the concept of a
			 * maximum  system call (or maximum API call) interrupt priority.
			 * Interrupts that are  above the maximum system call priority are keep
			 * permanently enabled, even when the RTOS kernel is in a critical section,
			 * but cannot make any calls to FreeRTOS API functions.  If configASSERT()
			 * is defined in FreeRTOSConfig.h then
			 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
			 * failure if a FreeRTOS API function is called from an interrupt that has
			 * been assigned a priority above the configured maximum system call
			 * priority.  Only FreeRTOS functions that end in FromISR can be called
			 * from interrupts  that have been assigned a priority at or (logically)
			 * below the maximum system call interrupt priority.  FreeRTOS maintains a
			 * separate interrupt safe API to ensure interrupt entry is as fast and as
			 * simple as possible.  More information (albeit Cortex-M specific) is
			 * provided on the following link:
			 * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
			portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

			uxSavedInterruptState = portSET_INTERRUPT_MASK_FROM_ISR();
			{
				/* If null is passed in here then it is the priority of the calling
				 * task that is being queried. */
				pxTCB = prvGetTCBFromHandle(xTask);
				uxReturn = pxTCB->uxPriority;
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptState);

			return uxReturn;
		}

#endif /* INCLUDE_uxTaskPriorityGet */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskPrioritySet == 1 )

		void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority) {
			TCB_t * pxTCB;
			UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
			BaseType_t xYieldRequired = pdFALSE;

			configASSERT(( uxNewPriority < configMAX_PRIORITIES ));

			/* Ensure the new priority is valid. */
			if (uxNewPriority >= (UBaseType_t) configMAX_PRIORITIES) {
				uxNewPriority = (UBaseType_t) configMAX_PRIORITIES - (UBaseType_t) 1U;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			taskENTER_CRITICAL()
			;
			{
				/* If null is passed in here then it is the priority of the calling
				 * task that is being changed. */
				pxTCB = prvGetTCBFromHandle(xTask);

				traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );

#if ( configUSE_MUTEXES == 1 )
				{
					uxCurrentBasePriority = pxTCB->uxBasePriority;
				}
#else
				{
					uxCurrentBasePriority = pxTCB->uxPriority;
				}
#endif

				if (uxCurrentBasePriority != uxNewPriority) {
					/* The priority change may have readied a task of higher
					 * priority than the calling task. */
					if (uxNewPriority > uxCurrentBasePriority) {
						if (pxTCB != pxCurrentTCB) {
							/* The priority of a task other than the currently
							 * running task is being raised.  Is the priority being
							 * raised above that of the running task? */
							if (uxNewPriority >= pxCurrentTCB->uxPriority) {
								xYieldRequired = pdTRUE;
							} else {
								mtCOVERAGE_TEST_MARKER();
							}
						} else {
							/* The priority of the running task is being raised,
							 * but the running task must already be the highest
							 * priority task able to run so no yield is required. */
						}
					} else if (pxTCB == pxCurrentTCB) {
						/* Setting the priority of the running task down means
						 * there may now be another task of higher priority that
						 * is ready to execute. */
						xYieldRequired = pdTRUE;
					} else {
						/* Setting the priority of any other task down does not
						 * require a yield as the running task must be above the
						 * new priority of the task being modified. */
					}

					/* Remember the ready list the task might be referenced from
					 * before its uxPriority member is changed so the
					 * taskRESET_READY_PRIORITY() macro can function correctly. */
					uxPriorityUsedOnEntry = pxTCB->uxPriority;

#if ( configUSE_MUTEXES == 1 )
					{
						/* Only change the priority being used if the task is not
						 * currently using an inherited priority. */
						if (pxTCB->uxBasePriority == pxTCB->uxPriority) {
							pxTCB->uxPriority = uxNewPriority;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						/* The base priority gets set whatever. */
						pxTCB->uxBasePriority = uxNewPriority;
					}
#else /* if ( configUSE_MUTEXES == 1 ) */
					{
						pxTCB->uxPriority = uxNewPriority;
					}
#endif /* if ( configUSE_MUTEXES == 1 ) */

					/* Only reset the event list item value if the value is not
					 * being used for anything else. */
					if (( listGET_LIST_ITEM_VALUE(&(pxTCB->xEventListItem))
							& taskEVENT_LIST_ITEM_VALUE_IN_USE) == 0UL) {
						listSET_LIST_ITEM_VALUE(&(pxTCB->xEventListItem),
								( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxNewPriority )); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
					} else {
						mtCOVERAGE_TEST_MARKER();
					}

					/* If the task is in the blocked or suspended list we need do
					 * nothing more than change its priority variable. However, if
					 * the task is in a ready list it needs to be removed and placed
					 * in the list appropriate to its new priority. */
					if ( listIS_CONTAINED_WITHIN(
							&(pxReadyTasksLists[uxPriorityUsedOnEntry]),
							&(pxTCB->xStateListItem)) != pdFALSE) {
						/* The task is currently in its ready list - remove before
						 * adding it to it's new ready list.  As we are in a critical
						 * section we can do this even if the scheduler is suspended. */
						if (uxListRemove(&(pxTCB->xStateListItem)) == (UBaseType_t) 0) {
							/* It is known that the task is in its ready list so
							 * there is no need to check again and the port level
							 * reset macro can be called directly. */
							portRESET_READY_PRIORITY(uxPriorityUsedOnEntry,
									uxTopReadyPriority);
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						prvAddTaskToReadyList(pxTCB);
					} else {
						mtCOVERAGE_TEST_MARKER();
					}

					if (xYieldRequired != pdFALSE) {
						taskYIELD_IF_USING_PREEMPTION()
																						;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}

					/* Remove compiler warning about unused variables when the port
					 * optimised task selection is not being used. */
					(void) uxPriorityUsedOnEntry;
				}
			}
			taskEXIT_CRITICAL()
			;
		}

#endif /* INCLUDE_vTaskPrioritySet */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

		void vTaskSuspend(TaskHandle_t xTaskToSuspend) {
			TCB_t * pxTCB;

			taskENTER_CRITICAL()
			;
			{
				/* If null is passed in here then it is the running task that is
				 * being suspended. */
				pxTCB = prvGetTCBFromHandle(xTaskToSuspend);

				traceTASK_SUSPEND( pxTCB );

				/* Remove task from the ready/delayed list and place in the
				 * suspended list. */
				if (uxListRemove(&(pxTCB->xStateListItem)) == (UBaseType_t) 0) {
					taskRESET_READY_PRIORITY(pxTCB->uxPriority);
				} else {
					mtCOVERAGE_TEST_MARKER();
				}

				/* Is the task waiting on an event also? */
				if ( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL) {
					(void) uxListRemove(&(pxTCB->xEventListItem));
				} else {
					mtCOVERAGE_TEST_MARKER();
				}

				vListInsertEnd(&xSuspendedTaskList, &(pxTCB->xStateListItem));

#if ( configUSE_TASK_NOTIFICATIONS == 1 )
				{
					BaseType_t x;

					for (x = 0; x < configTASK_NOTIFICATION_ARRAY_ENTRIES; x++) {
						if (pxTCB->ucNotifyState[x] == taskWAITING_NOTIFICATION) {
							/* The task was blocked to wait for a notification, but is
							 * now suspended, so no notification was received. */
							pxTCB->ucNotifyState[x] = taskNOT_WAITING_NOTIFICATION;
						}
					}
				}
#endif /* if ( configUSE_TASK_NOTIFICATIONS == 1 ) */
			}
			taskEXIT_CRITICAL()
			;

			if (xSchedulerRunning != pdFALSE) {
				/* Reset the next expected unblock time in case it referred to the
				 * task that is now in the Suspended state. */
				taskENTER_CRITICAL()
																				;
				{
					prvResetNextTaskUnblockTime();
				}
				taskEXIT_CRITICAL()
				;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			if (pxTCB == pxCurrentTCB) {
				if (xSchedulerRunning != pdFALSE) {
					/* The current task has just been suspended. */
					configASSERT(uxSchedulerSuspended == 0);
					portYIELD_WITHIN_API()
					;
				} else {
					/* The scheduler is not running, but the task that was pointed
					 * to by pxCurrentTCB has just been suspended and pxCurrentTCB
					 * must be adjusted to point to a different task. */
					if ( listCURRENT_LIST_LENGTH(&xSuspendedTaskList)
							== uxCurrentNumberOfTasks) /*lint !e931 Right has no side effect, just volatile. */
					{
						/* No other tasks are ready, so set pxCurrentTCB back to
						 * NULL so when the next task is created pxCurrentTCB will
						 * be set to point to it no matter what its relative priority
						 * is. */
						pxCurrentTCB = NULL;
					} else {
						vTaskSwitchContext();
					}
				}
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* INCLUDE_vTaskSuspend */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

		static BaseType_t prvTaskIsTaskSuspended(const TaskHandle_t xTask) {
			BaseType_t xReturn = pdFALSE;
			const TCB_t * const pxTCB = xTask;

			/* Accesses xPendingReadyList so must be called from a critical
			 * section. */

			/* It does not make sense to check if the calling task is suspended. */
			configASSERT(xTask);

			/* Is the task being resumed actually in the suspended list? */
			if ( listIS_CONTAINED_WITHIN(&xSuspendedTaskList,
					&(pxTCB->xStateListItem)) != pdFALSE) {
				/* Has the task already been resumed from within an ISR? */
				if ( listIS_CONTAINED_WITHIN(&xPendingReadyList,
						&(pxTCB->xEventListItem)) == pdFALSE) {
					/* Is it in the suspended list because it is in the Suspended
					 * state, or because is is blocked with no timeout? */
					if ( listIS_CONTAINED_WITHIN(NULL,
							&(pxTCB->xEventListItem)) != pdFALSE) /*lint !e961.  The cast is only redundant when NULL is used. */
					{
						xReturn = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return xReturn;
		} /*lint !e818 xTask cannot be a pointer to const because it is a typedef. */

#endif /* INCLUDE_vTaskSuspend */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

		void vTaskResume(TaskHandle_t xTaskToResume) {
			TCB_t * const pxTCB = xTaskToResume;

			/* It does not make sense to resume the calling task. */
			configASSERT(xTaskToResume);

			/* The parameter cannot be NULL as it is impossible to resume the
			 * currently executing task. */
			if ((pxTCB != pxCurrentTCB) && (pxTCB != NULL)) {
				taskENTER_CRITICAL()
																				;
				{
					if (prvTaskIsTaskSuspended(pxTCB) != pdFALSE) {
						traceTASK_RESUME( pxTCB );

						/* The ready list can be accessed even if the scheduler is
						 * suspended because this is inside a critical section. */
						(void) uxListRemove(&(pxTCB->xStateListItem));
						prvAddTaskToReadyList(pxTCB);

						/* A higher priority task may have just been resumed. */
						if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority) {
							/* This yield may not cause the task just resumed to run,
							 * but will leave the lists in the correct state for the
							 * next yield. */
							taskYIELD_IF_USING_PREEMPTION()
																							;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}
				taskEXIT_CRITICAL()
				;
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* INCLUDE_vTaskSuspend */

		/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

		BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume) {
			BaseType_t xYieldRequired = pdFALSE;
			TCB_t * const pxTCB = xTaskToResume;
			UBaseType_t uxSavedInterruptStatus;

			configASSERT(xTaskToResume);

			/* RTOS ports that support interrupt nesting have the concept of a
			 * maximum  system call (or maximum API call) interrupt priority.
			 * Interrupts that are  above the maximum system call priority are keep
			 * permanently enabled, even when the RTOS kernel is in a critical section,
			 * but cannot make any calls to FreeRTOS API functions.  If configASSERT()
			 * is defined in FreeRTOSConfig.h then
			 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
			 * failure if a FreeRTOS API function is called from an interrupt that has
			 * been assigned a priority above the configured maximum system call
			 * priority.  Only FreeRTOS functions that end in FromISR can be called
			 * from interrupts  that have been assigned a priority at or (logically)
			 * below the maximum system call interrupt priority.  FreeRTOS maintains a
			 * separate interrupt safe API to ensure interrupt entry is as fast and as
			 * simple as possible.  More information (albeit Cortex-M specific) is
			 * provided on the following link:
			 * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
			portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

			uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
			{
				if (prvTaskIsTaskSuspended(pxTCB) != pdFALSE) {
					traceTASK_RESUME_FROM_ISR( pxTCB );

					/* Check the ready lists can be accessed. */
					if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
						/* Ready lists can be accessed so move the task from the
						 * suspended list to the ready list directly. */
						if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority) {
							xYieldRequired = pdTRUE;

							/* Mark that a yield is pending in case the user is not
							 * using the return value to initiate a context switch
							 * from the ISR using portYIELD_FROM_ISR. */
							xYieldPending = pdTRUE;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						(void) uxListRemove(&(pxTCB->xStateListItem));
						prvAddTaskToReadyList(pxTCB);
					} else {
						/* The delayed or ready lists cannot be accessed so the task
						 * is held in the pending ready list until the scheduler is
						 * unsuspended. */
						vListInsertEnd(&(xPendingReadyList), &(pxTCB->xEventListItem));
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

			return xYieldRequired;
		}

#endif /* ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) ) */
		/*-----------------------------------------------------------*/



		void vTaskStartFaultDetector(u8 restoreTrainDataFromSd, FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions[FAULTDETECTOR_MAX_CHECKS]) {
			FAULTDET_init(restoreTrainDataFromSd, trainedRegions, n_regions);
#ifndef FAULTDETECTOR_EXECINSW
			FAULTDET_start();
#endif
		}


		void vTaskStartScheduler() {
			BaseType_t xReturn;

			u32 tasksTCBPtrs[ configMAX_RT_TASKS ];
			u32 tasksWCETs[ configMAX_RT_TASKS ][ configCRITICALITY_LEVELS ];
			u32 tasksDeadlines[ configMAX_RT_TASKS ][ configMAX_RT_TASKS ];
			u32 tasksPeriods[ configMAX_RT_TASKS ];
			u32 tasksCriticalityLevels[ configMAX_RT_TASKS ];

			if (prvSplitRTTasksList(pxRTTasksList, uxTaskNumber,
					configMAX_RT_TASKS,
					tasksTCBPtrs,
					tasksWCETs,
					tasksDeadlines,
					tasksPeriods,
					tasksCriticalityLevels
			)!=0) {
				printf("error, cannot generate schedule");
				return;
			}

			if (xPortInitScheduler( (u32) uxTaskNumber,
					(void *) tasksTCBPtrs,
					(void *) tasksWCETs,
					(void *) tasksDeadlines,
					(void *) tasksPeriods,
					(void *) tasksCriticalityLevels,
					(u32*) &pxCurrentTCB) == pdPASS) {
				/* Add the idle task at the lowest priority. */
#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
				{
					StaticTask_t * pxIdleTaskTCBBuffer = NULL;
					StackType_t * pxIdleTaskStackBuffer = NULL;
					uint32_t ulIdleTaskStackSize;

					/* The Idle task is created using user provided RAM - obtain the
					 * address of the RAM then create the idle task. */
					vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &ulIdleTaskStackSize );
					xIdleTaskHandle = xTaskCreateStatic( prvIdleTask,
							configIDLE_TASK_NAME,
							ulIdleTaskStackSize,
							( void * ) NULL, /*lint !e961.  The cast is not redundant for all compilers. */
							portPRIVILEGE_BIT, /* In effect ( tskIDLE_PRIORITY | portPRIVILEGE_BIT ), but tskIDLE_PRIORITY is zero. */
							pxIdleTaskStackBuffer,
							pxIdleTaskTCBBuffer, //fedit add
							&pxIdleTCB ); /*lint !e961 MISRA exception, justified as it is not a redundant explicit cast to all supported compilers. */

					if( xIdleTaskHandle != NULL )
					{
						xReturn = pdPASS;
					}
					else
					{
						xReturn = pdFAIL;
					}
				}
#else /* if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
				{
					/* The Idle task is being created using dynamically allocated RAM. */
					xReturn = xTaskCreate(prvIdleTask,
							configIDLE_TASK_NAME,
							configMINIMAL_STACK_SIZE, (void *) NULL,
							portPRIVILEGE_BIT, /* In effect ( tskIDLE_PRIORITY | portPRIVILEGE_BIT ), but tskIDLE_PRIORITY is zero. */
							&xIdleTaskHandle, //fedit add
							&pxIdleTCB); /*lint !e961 MISRA exception, justified as it is not a redundant explicit cast to all supported compilers. */
				}
#endif /* configSUPPORT_STATIC_ALLOCATION */

#if ( configUSE_TIMERS == 1 )
				{
					if( xReturn == pdPASS )
					{
						//fedit remove TODO
						//xReturn = xTimerCreateTimerTask();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
#endif /* configUSE_TIMERS */

				if (xReturn == pdPASS) {
					/* freertos_tasks_c_additions_init() should only be called if the user
					 * definable macro FREERTOS_TASKS_C_ADDITIONS_INIT() is defined, as that is
					 * the only macro called by the function. */
#ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
					{
						freertos_tasks_c_additions_init();
					}
#endif

					/* Interrupts are turned off here, to ensure a tick does not occur
					 * before or during the call to xPortStartScheduler().  The stacks of
					 * the created tasks contain a status word with interrupts switched on
					 * so interrupts will automatically get re-enabled when the first task
					 * starts to run. */
					portDISABLE_INTERRUPTS();

#if ( configUSE_NEWLIB_REENTRANT == 1 )
					{
						/* Switch Newlib's _impure_ptr variable to point to the _reent
						 * structure specific to the task that will run first.
						 * See the third party link http://www.nadler.com/embedded/newlibAndFreeRTOS.html
						 * for additional information. */
						_impure_ptr = &( pxCurrentTCB->xNewLib_reent );
					}
#endif /* configUSE_NEWLIB_REENTRANT */

					xNextTaskUnblockTime = portMAX_DELAY;
					xSchedulerRunning = pdTRUE;
					xTickCount = (TickType_t) configINITIAL_TICK_COUNT;

					/* If configGENERATE_RUN_TIME_STATS is defined then the following
					 * macro must be defined to configure the timer/counter used to generate
					 * the run time counter time base.   NOTE:  If configGENERATE_RUN_TIME_STATS
					 * is set to 0 and the following line fails to build then ensure you do not
					 * have portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() defined in your
					 * FreeRTOSConfig.h file. */
					portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();

					traceTASK_SWITCHED_IN();

					//printf("idle task global pointer in tasks.c: %p", &pxIdleTCB);

					//fedit add
					/* now schedule idle task; first -not idle - task will be scheduled on FPGA interrupt */
					pxCurrentTCB = pxIdleTCB;

					/* Setting up the timer tick is hardware specific and thus in the
					 * portable interface. */

					if (xPortStartScheduler() != pdFALSE) {
						/* Should not reach here as if the scheduler is running the
						 * function will not return. */
					} else {
						/* Should only reach here if a task calls xTaskEndScheduler(). */
					}
				} else {
					/* This line will only be reached if the kernel could not be started,
					 * because there was not enough FreeRTOS heap to create the idle task
					 * or the timer task. */
					configASSERT(xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY);
				}
			} else {
				/* Error in initialising the scheduler */
				configASSERT(FALSE);
			}

			/* Prevent compiler warnings if INCLUDE_xTaskGetIdleTaskHandle is set to 0,
			 * meaning xIdleTaskHandle is not used anywhere else. */
			(void) xIdleTaskHandle;

			/* OpenOCD makes use of uxTopUsedPriority for thread debugging. Prevent uxTopUsedPriority
			 * from getting optimized out as it is no longer used by the kernel. */
			(void) uxTopUsedPriority;
		}
		/*-----------------------------------------------------------*/

		void vTaskEndScheduler(void) {
			/* Stop the scheduler interrupts and call the portable scheduler end
			 * routine so the original ISRs can be restored if necessary.  The port
			 * layer must ensure interrupts enable  bit is left in the correct state. */
			portDISABLE_INTERRUPTS();
			xSchedulerRunning = pdFALSE;
			vPortEndScheduler();
		}
		/*----------------------------------------------------------*/

		void vTaskSuspendAll(void) {
			/* A critical section is not required as the variable is of type
			 * BaseType_t.  Please read Richard Barry's reply in the following link to a
			 * post in the FreeRTOS support forum before reporting this as a bug! -
			 * https://goo.gl/wu4acr */

			/* portSOFRWARE_BARRIER() is only implemented for emulated/simulated ports that
			 * do not otherwise exhibit real time behaviour. */
			portSOFTWARE_BARRIER();

			/* The scheduler is suspended if uxSchedulerSuspended is non-zero.  An increment
			 * is used to allow calls to vTaskSuspendAll() to nest. */
			++uxSchedulerSuspended;

			/* Enforces ordering for ports and optimised compilers that may otherwise place
			 * the above increment elsewhere. */
			portMEMORY_BARRIER();
		}
		/*----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE != 0 )

		static TickType_t prvGetExpectedIdleTime( void )
		{
			TickType_t xReturn;
			UBaseType_t uxHigherPriorityReadyTasks = pdFALSE;

			/* uxHigherPriorityReadyTasks takes care of the case where
			 * configUSE_PREEMPTION is 0, so there may be tasks above the idle priority
			 * task that are in the Ready state, even though the idle task is
			 * running. */
#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
			{
				if( uxTopReadyPriority > tskIDLE_PRIORITY )
				{
					uxHigherPriorityReadyTasks = pdTRUE;
				}
			}
#else
			{
				const UBaseType_t uxLeastSignificantBit = ( UBaseType_t ) 0x01;

				/* When port optimised task selection is used the uxTopReadyPriority
				 * variable is used as a bit map.  If bits other than the least
				 * significant bit are set then there are tasks that have a priority
				 * above the idle priority that are in the Ready state.  This takes
				 * care of the case where the co-operative scheduler is in use. */
				if( uxTopReadyPriority > uxLeastSignificantBit )
				{
					uxHigherPriorityReadyTasks = pdTRUE;
				}
			}
#endif /* if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 ) */

			if( pxCurrentTCB->uxPriority > tskIDLE_PRIORITY )
			{
				xReturn = 0;
			}
			else if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > 1 )
			{
				/* There are other idle priority tasks in the ready state.  If
				 * time slicing is used then the very next tick interrupt must be
				 * processed. */
				xReturn = 0;
			}
			else if( uxHigherPriorityReadyTasks != pdFALSE )
			{
				/* There are tasks in the Ready state that have a priority above the
				 * idle priority.  This path can only be reached if
				 * configUSE_PREEMPTION is 0. */
				xReturn = 0;
			}
			else
			{
				xReturn = xNextTaskUnblockTime - xTickCount;
			}

			return xReturn;
		}

#endif /* configUSE_TICKLESS_IDLE */
		/*----------------------------------------------------------*/

		BaseType_t xTaskResumeAll(void) {
			TCB_t * pxTCB = NULL;
			BaseType_t xAlreadyYielded = pdFALSE;

			/* If uxSchedulerSuspended is zero then this function does not match a
			 * previous call to vTaskSuspendAll(). */
			configASSERT(uxSchedulerSuspended);

			/* It is possible that an ISR caused a task to be removed from an event
			 * list while the scheduler was suspended.  If this was the case then the
			 * removed task will have been added to the xPendingReadyList.  Once the
			 * scheduler has been resumed it is safe to move all the pending ready
			 * tasks from this list into their appropriate ready list. */
			taskENTER_CRITICAL()
			;
			{
				--uxSchedulerSuspended;

				if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
					if (uxCurrentNumberOfTasks > (UBaseType_t) 0U) {
						/* Move any readied tasks from the pending list into the
						 * appropriate ready list. */
						while ( listLIST_IS_EMPTY( &xPendingReadyList ) == pdFALSE) {
							pxTCB = listGET_OWNER_OF_HEAD_ENTRY((&xPendingReadyList)); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
							(void) uxListRemove(&(pxTCB->xEventListItem));
							(void) uxListRemove(&(pxTCB->xStateListItem));
							prvAddTaskToReadyList(pxTCB);

							/* If the moved task has a priority higher than the current
							 * task then a yield must be performed. */
							if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority) {
								xYieldPending = pdTRUE;
							} else {
								mtCOVERAGE_TEST_MARKER();
							}
						}

						if (pxTCB != NULL) {
							/* A task was unblocked while the scheduler was suspended,
							 * which may have prevented the next unblock time from being
							 * re-calculated, in which case re-calculate it now.  Mainly
							 * important for low power tickless implementations, where
							 * this can prevent an unnecessary exit from low power
							 * state. */
							prvResetNextTaskUnblockTime();
						}

						///* If any ticks occurred while the scheduler was suspended then
						// * they should be processed now.  This ensures the tick count does
						// * not  slip, and that any delayed tasks are resumed at the correct
						// * time. */
						//{
						//	TickType_t xPendedCounts = xPendedTicks; /* Non-volatile copy. */

						//	if (xPendedCounts > (TickType_t) 0U) {
						//		do {
						//			if (xTaskIncrementTick() != pdFALSE) {
						//				xYieldPending = pdTRUE;
						//			} else {
						//				mtCOVERAGE_TEST_MARKER();
						//			}

						//			--xPendedCounts;
						//		} while (xPendedCounts > (TickType_t) 0U);

						//		xPendedTicks = 0;
						//	} else {
						//		mtCOVERAGE_TEST_MARKER();
						//	}
						//}

						if (xYieldPending != pdFALSE) {
#if ( configUSE_PREEMPTION != 0 )
							{
								xAlreadyYielded = pdTRUE;
							}
#endif
							taskYIELD_IF_USING_PREEMPTION()
							;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL()
			;

			return xAlreadyYielded;
		}
		/*-----------------------------------------------------------*/

		TickType_t xTaskGetTickCount(void) {
			TickType_t xTicks;

			/* Critical section required if running on a 16 bit processor. */
			portTICK_TYPE_ENTER_CRITICAL();
			{
				xTicks = xTickCount;
			}portTICK_TYPE_EXIT_CRITICAL();

			return xTicks;
		}
		/*-----------------------------------------------------------*/

		TickType_t xTaskGetTickCountFromISR(void) {
			TickType_t xReturn;
			UBaseType_t uxSavedInterruptStatus;

			/* RTOS ports that support interrupt nesting have the concept of a maximum
			 * system call (or maximum API call) interrupt priority.  Interrupts that are
			 * above the maximum system call priority are kept permanently enabled, even
			 * when the RTOS kernel is in a critical section, but cannot make any calls to
			 * FreeRTOS API functions.  If configASSERT() is defined in FreeRTOSConfig.h
			 * then portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
			 * failure if a FreeRTOS API function is called from an interrupt that has been
			 * assigned a priority above the configured maximum system call priority.
			 * Only FreeRTOS functions that end in FromISR can be called from interrupts
			 * that have been assigned a priority at or (logically) below the maximum
			 * system call  interrupt priority.  FreeRTOS maintains a separate interrupt
			 * safe API to ensure interrupt entry is as fast and as simple as possible.
			 * More information (albeit Cortex-M specific) is provided on the following
			 * link: https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
			portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

			uxSavedInterruptStatus = portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
			{
				xReturn = xTickCount;
			}
			portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

			return xReturn;
		}
		/*-----------------------------------------------------------*/

		UBaseType_t uxTaskGetNumberOfTasks(void) {
			/* A critical section is not required because the variables are of type
			 * BaseType_t. */
			return uxCurrentNumberOfTasks;
		}
		/*-----------------------------------------------------------*/

		char * pcTaskGetName(TaskHandle_t xTaskToQuery) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
		{
			TCB_t * pxTCB;

			/* If null is passed in here then the name of the calling task is being
			 * queried. */
			pxTCB = prvGetTCBFromHandle(xTaskToQuery);
			configASSERT(pxTCB);
			return &(pxTCB->pcTaskName[0]);
		}
		/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )

		static TCB_t * prvSearchForNameWithinSingleList(List_t * pxList,
				const char pcNameToQuery[]) {
			TCB_t * pxNextTCB, *pxFirstTCB, *pxReturn = NULL;
			UBaseType_t x;
			char cNextChar;
			BaseType_t xBreakLoop;

			/* This function is called with the scheduler suspended. */

			if ( listCURRENT_LIST_LENGTH( pxList ) > (UBaseType_t) 0) {
				listGET_OWNER_OF_NEXT_ENTRY(pxFirstTCB, pxList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */

				do {
					listGET_OWNER_OF_NEXT_ENTRY(pxNextTCB, pxList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */

					/* Check each character in the name looking for a match or
					 * mismatch. */
					xBreakLoop = pdFALSE;

					for (x = (UBaseType_t) 0; x < (UBaseType_t) configMAX_TASK_NAME_LEN;
							x++) {
						cNextChar = pxNextTCB->pcTaskName[x];

						if (cNextChar != pcNameToQuery[x]) {
							/* Characters didn't match. */
							xBreakLoop = pdTRUE;
						} else if (cNextChar == (char) 0x00) {
							/* Both strings terminated, a match must have been
							 * found. */
							pxReturn = pxNextTCB;
							xBreakLoop = pdTRUE;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						if (xBreakLoop != pdFALSE) {
							break;
						}
					}

					if (pxReturn != NULL) {
						/* The handle has been found. */
						break;
					}
				} while (pxNextTCB != pxFirstTCB);
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return pxReturn;
		}

#endif /* INCLUDE_xTaskGetHandle */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )

		TaskHandle_t xTaskGetHandle(const char * pcNameToQuery) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
		{
			UBaseType_t uxQueue = configMAX_PRIORITIES;
			TCB_t * pxTCB;

			/* Task names will be truncated to configMAX_TASK_NAME_LEN - 1 bytes. */
			configASSERT(strlen( pcNameToQuery ) < configMAX_TASK_NAME_LEN);

			vTaskSuspendAll();
			{
				/* Search the ready lists. */
				do {
					uxQueue--;
					pxTCB = prvSearchForNameWithinSingleList(
							(List_t *) &(pxReadyTasksLists[uxQueue]), pcNameToQuery);

					if (pxTCB != NULL) {
						/* Found the handle. */
						break;
					}
				} while (uxQueue > (UBaseType_t) tskIDLE_PRIORITY); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

				/* Search the delayed lists. */
				if (pxTCB == NULL) {
					pxTCB = prvSearchForNameWithinSingleList(
							(List_t *) pxDelayedTaskList, pcNameToQuery);
				}

				if (pxTCB == NULL) {
					pxTCB = prvSearchForNameWithinSingleList(
							(List_t *) pxOverflowDelayedTaskList, pcNameToQuery);
				}

#if ( INCLUDE_vTaskSuspend == 1 )
				{
					if (pxTCB == NULL) {
						/* Search the suspended list. */
						pxTCB = prvSearchForNameWithinSingleList(&xSuspendedTaskList,
								pcNameToQuery);
					}
				}
#endif

#if ( INCLUDE_vTaskDelete == 1 )
				{
					if (pxTCB == NULL) {
						/* Search the deleted list. */
						pxTCB = prvSearchForNameWithinSingleList(
								&xTasksWaitingTermination, pcNameToQuery);
					}
				}
#endif
			}
			(void) xTaskResumeAll();

			return pxTCB;
		}

#endif /* INCLUDE_xTaskGetHandle */
		/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

		UBaseType_t uxTaskGetSystemState(TaskStatus_t * const pxTaskStatusArray,
				const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime) {
			UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

			vTaskSuspendAll();
			{
				/* Is there a space in the array for each task in the system? */
				if (uxArraySize >= uxCurrentNumberOfTasks) {
					/* Fill in an TaskStatus_t structure with information on each
					 * task in the Ready state. */
					do {
						uxQueue--;
						uxTask += prvListTasksWithinSingleList(
								&(pxTaskStatusArray[uxTask]),
								&(pxReadyTasksLists[uxQueue]), eReady);
					} while (uxQueue > (UBaseType_t) tskIDLE_PRIORITY); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

					/* Fill in an TaskStatus_t structure with information on each
					 * task in the Blocked state. */
					uxTask += prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]),
							(List_t *) pxDelayedTaskList, eBlocked);
					uxTask += prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]),
							(List_t *) pxOverflowDelayedTaskList, eBlocked);

#if ( INCLUDE_vTaskDelete == 1 )
					{
						/* Fill in an TaskStatus_t structure with information on
						 * each task that has been deleted but not yet cleaned up. */
						uxTask += prvListTasksWithinSingleList(
								&(pxTaskStatusArray[uxTask]), &xTasksWaitingTermination,
								eDeleted);
					}
#endif

#if ( INCLUDE_vTaskSuspend == 1 )
					{
						/* Fill in an TaskStatus_t structure with information on
						 * each task in the Suspended state. */
						uxTask += prvListTasksWithinSingleList(
								&(pxTaskStatusArray[uxTask]), &xSuspendedTaskList,
								eSuspended);
					}
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 )
					{
						if( pulTotalRunTime != NULL )
						{
#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
							portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
#else
							*pulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
#endif
						}
					}
#else /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
					{
						if (pulTotalRunTime != NULL) {
							*pulTotalRunTime = 0;
						}
					}
#endif /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			(void) xTaskResumeAll();

			return uxTask;
		}

#endif /* configUSE_TRACE_FACILITY */
		/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

		TaskHandle_t xTaskGetIdleTaskHandle( void )
		{
			/* If xTaskGetIdleTaskHandle() is called before the scheduler has been
			 * started, then xIdleTaskHandle will be NULL. */
			configASSERT( ( xIdleTaskHandle != NULL ) );
			return xIdleTaskHandle;
		}

#endif /* INCLUDE_xTaskGetIdleTaskHandle */
		/*----------------------------------------------------------*/

		/* This conditional compilation should use inequality to 0, not equality to 1.
		 * This is to ensure vTaskStepTick() is available when user defined low power mode
		 * implementations require configUSE_TICKLESS_IDLE to be set to a value other than
		 * 1. */
#if ( configUSE_TICKLESS_IDLE != 0 )

		void vTaskStepTick( const TickType_t xTicksToJump )
		{
			/* Correct the tick count value after a period during which the tick
			 * was suppressed.  Note this does *not* call the tick hook function for
			 * each stepped tick. */
			configASSERT( ( xTickCount + xTicksToJump ) <= xNextTaskUnblockTime );
			xTickCount += xTicksToJump;
			traceINCREASE_TICK_COUNT( xTicksToJump );
		}

#endif /* configUSE_TICKLESS_IDLE */
		/*----------------------------------------------------------*/

		BaseType_t xTaskCatchUpTicks(TickType_t xTicksToCatchUp) {
			BaseType_t xYieldOccurred;

			/* Must not be called with the scheduler suspended as the implementation
			 * relies on xPendedTicks being wound down to 0 in xTaskResumeAll(). */
			configASSERT(uxSchedulerSuspended == 0);

			/* Use xPendedTicks to mimic xTicksToCatchUp number of ticks occurring when
			 * the scheduler is suspended so the ticks are executed in xTaskResumeAll(). */
			vTaskSuspendAll();
			xPendedTicks += xTicksToCatchUp;
			xYieldOccurred = xTaskResumeAll();

			return xYieldOccurred;
		}
		/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskAbortDelay == 1 )

		BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
		{
			TCB_t * pxTCB = xTask;
			BaseType_t xReturn;

			configASSERT( pxTCB );

			vTaskSuspendAll();
			{
				/* A task can only be prematurely removed from the Blocked state if
				 * it is actually in the Blocked state. */
				if( eTaskGetState( xTask ) == eBlocked )
				{
					xReturn = pdPASS;

					/* Remove the reference to the task from the blocked list.  An
					 * interrupt won't touch the xStateListItem because the
					 * scheduler is suspended. */
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );

					/* Is the task waiting on an event also?  If so remove it from
					 * the event list too.  Interrupts can touch the event list item,
					 * even though the scheduler is suspended, so a critical section
					 * is used. */
					taskENTER_CRITICAL();
					{
						if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
						{
							( void ) uxListRemove( &( pxTCB->xEventListItem ) );

							/* This lets the task know it was forcibly removed from the
							 * blocked state so it should not re-evaluate its block time and
							 * then block again. */
							pxTCB->ucDelayAborted = pdTRUE;
						}
						else
						{
							mtCOVERAGE_TEST_MARKER();
						}
					}
					taskEXIT_CRITICAL();

					/* Place the unblocked task into the appropriate ready list. */
					prvAddTaskToReadyList( pxTCB );

					/* A task being unblocked cannot cause an immediate context
					 * switch if preemption is turned off. */
#if ( configUSE_PREEMPTION == 1 )
					{
						/* Preemption is on, but a context switch should only be
						 *  performed if the unblocked task has a priority that is
						 *  equal to or higher than the currently executing task. */
						if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )
						{
							/* Pend the yield to be performed when the scheduler
							 * is unsuspended. */
							xYieldPending = pdTRUE;
						}
						else
						{
							mtCOVERAGE_TEST_MARKER();
						}
					}
#endif /* configUSE_PREEMPTION */
				}
				else
				{
					xReturn = pdFAIL;
				}
			}
			( void ) xTaskResumeAll();

			return xReturn;
		}

#endif /* INCLUDE_xTaskAbortDelay */
		/*----------------------------------------------------------*/

		//BaseType_t xTaskIncrementTick(void) {
		//	TCB_t * pxTCB;
		//	TickType_t xItemValue;
		//	BaseType_t xSwitchRequired = pdFALSE;
		//
		//	/* Called by the portable layer each time a tick interrupt occurs.
		//	 * Increments the tick then checks to see if the new tick value will cause any
		//	 * tasks to be unblocked. */
		//	traceTASK_INCREMENT_TICK( xTickCount );
		//
		//	if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
		//		/* Minor optimisation.  The tick count cannot change in this
		//		 * block. */
		//		const TickType_t xConstTickCount = xTickCount + (TickType_t) 1;
		//
		//		/* Increment the RTOS tick, switching the delayed and overflowed
		//		 * delayed lists if it wraps to 0. */
		//		xTickCount = xConstTickCount;
		//
		//		if (xConstTickCount == (TickType_t) 0U) /*lint !e774 'if' does not always evaluate to false as it is looking for an overflow. */
		//		{
		//			taskSWITCH_DELAYED_LISTS()
		//			;
		//		} else {
		//			mtCOVERAGE_TEST_MARKER();
		//		}
		//
		//		/* See if this tick has made a timeout expire.  Tasks are stored in
		//		 * the  queue in the order of their wake time - meaning once one task
		//		 * has been found whose block time has not expired there is no need to
		//		 * look any further down the list. */
		//		if (xConstTickCount >= xNextTaskUnblockTime) {
		//			for (;;) {
		//				if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE) {
		//					/* The delayed list is empty.  Set xNextTaskUnblockTime
		//					 * to the maximum possible value so it is extremely
		//					 * unlikely that the
		//					 * if( xTickCount >= xNextTaskUnblockTime ) test will pass
		//					 * next time through. */
		//					xNextTaskUnblockTime = portMAX_DELAY; /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
		//					break;
		//				} else {
		//					/* The delayed list is not empty, get the value of the
		//					 * item at the head of the delayed list.  This is the time
		//					 * at which the task at the head of the delayed list must
		//					 * be removed from the Blocked state. */
		//					pxTCB = listGET_OWNER_OF_HEAD_ENTRY(pxDelayedTaskList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
		//					xItemValue = listGET_LIST_ITEM_VALUE(
		//							&(pxTCB->xStateListItem));
		//
		//					if (xConstTickCount < xItemValue) {
		//						/* It is not time to unblock this item yet, but the
		//						 * item value is the time at which the task at the head
		//						 * of the blocked list must be removed from the Blocked
		//						 * state -  so record the item value in
		//						 * xNextTaskUnblockTime. */
		//						xNextTaskUnblockTime = xItemValue;
		//						break; /*lint !e9011 Code structure here is deedmed easier to understand with multiple breaks. */
		//					} else {
		//						mtCOVERAGE_TEST_MARKER();
		//					}
		//
		//					/* It is time to remove the item from the Blocked state. */
		//					(void) uxListRemove(&(pxTCB->xStateListItem));
		//
		//					/* Is the task waiting on an event also?  If so remove
		//					 * it from the event list. */
		//					if ( listLIST_ITEM_CONTAINER(
		//							&(pxTCB->xEventListItem)) != NULL) {
		//						(void) uxListRemove(&(pxTCB->xEventListItem));
		//					} else {
		//						mtCOVERAGE_TEST_MARKER();
		//					}
		//
		//					/* Place the unblocked task into the appropriate ready
		//					 * list. */
		//					prvAddTaskToReadyList(pxTCB);
		//
		//					/* A task being unblocked cannot cause an immediate
		//					 * context switch if preemption is turned off. */
		//#if ( configUSE_PREEMPTION == 1 )
		//					{
		//						/* Preemption is on, but a context switch should
		//						 * only be performed if the unblocked task has a
		//						 * priority that is equal to or higher than the
		//						 * currently executing task. */
		//						if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority) {
		//							xSwitchRequired = pdTRUE;
		//						} else {
		//							mtCOVERAGE_TEST_MARKER();
		//						}
		//					}
		//#endif /* configUSE_PREEMPTION */
		//				}
		//			}
		//		}
		//
		//		/* Tasks of equal priority to the currently running task will share
		//		 * processing time (time slice) if preemption is on, and the application
		//		 * writer has not explicitly turned time slicing off. */
		//#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
		//		{
		//			if ( listCURRENT_LIST_LENGTH(
		//					&(pxReadyTasksLists[pxCurrentTCB->uxPriority]))
		//					> (UBaseType_t) 1) {
		//				xSwitchRequired = pdTRUE;
		//			} else {
		//				mtCOVERAGE_TEST_MARKER();
		//			}
		//		}
		//#endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */
		//
		//#if ( configUSE_TICK_HOOK == 1 )
		//		{
		//			/* Guard against the tick hook being called when the pended tick
		//			 * count is being unwound (when the scheduler is being unlocked). */
		//			if( xPendedTicks == ( TickType_t ) 0 )
		//			{
		//				vApplicationTickHook();
		//			}
		//			else
		//			{
		//				mtCOVERAGE_TEST_MARKER();
		//			}
		//		}
		//#endif /* configUSE_TICK_HOOK */
		//
		//#if ( configUSE_PREEMPTION == 1 )
		//		{
		//			if (xYieldPending != pdFALSE) {
		//				xSwitchRequired = pdTRUE;
		//			} else {
		//				mtCOVERAGE_TEST_MARKER();
		//			}
		//		}
		//#endif /* configUSE_PREEMPTION */
		//	} else {
		//		++xPendedTicks;
		//
		//		/* The tick hook gets called at regular intervals, even if the
		//		 * scheduler is locked. */
		//#if ( configUSE_TICK_HOOK == 1 )
		//		{
		//			vApplicationTickHook();
		//		}
		//#endif
		//	}
		//
		//	return xSwitchRequired;
		//}
		/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

		void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
				TaskHookFunction_t pxHookFunction )
		{
			TCB_t * xTCB;

			/* If xTask is NULL then it is the task hook of the calling task that is
			 * getting set. */
			if( xTask == NULL )
			{
				xTCB = ( TCB_t * ) pxCurrentTCB;
			}
			else
			{
				xTCB = xTask;
			}

			/* Save the hook function in the TCB.  A critical section is required as
			 * the value can be accessed from an interrupt. */
			taskENTER_CRITICAL();
			{
				xTCB->pxTaskTag = pxHookFunction;
			}
			taskEXIT_CRITICAL();
		}

#endif /* configUSE_APPLICATION_TASK_TAG */
		/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

		TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
		{
			TCB_t * pxTCB;
			TaskHookFunction_t xReturn;

			/* If xTask is NULL then set the calling task's hook. */
			pxTCB = prvGetTCBFromHandle( xTask );

			/* Save the hook function in the TCB.  A critical section is required as
			 * the value can be accessed from an interrupt. */
			taskENTER_CRITICAL();
			{
				xReturn = pxTCB->pxTaskTag;
			}
			taskEXIT_CRITICAL();

			return xReturn;
		}

#endif /* configUSE_APPLICATION_TASK_TAG */
		/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

		TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask )
		{
			TCB_t * pxTCB;
			TaskHookFunction_t xReturn;
			UBaseType_t uxSavedInterruptStatus;

			/* If xTask is NULL then set the calling task's hook. */
			pxTCB = prvGetTCBFromHandle( xTask );

			/* Save the hook function in the TCB.  A critical section is required as
			 * the value can be accessed from an interrupt. */
			uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
			{
				xReturn = pxTCB->pxTaskTag;
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

			return xReturn;
		}

#endif /* configUSE_APPLICATION_TASK_TAG */
		/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

		BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
				void * pvParameter )
		{
			TCB_t * xTCB;
			BaseType_t xReturn;

			/* If xTask is NULL then we are calling our own task hook. */
			if( xTask == NULL )
			{
				xTCB = pxCurrentTCB;
			}
			else
			{
				xTCB = xTask;
			}

			if( xTCB->pxTaskTag != NULL )
			{
				xReturn = xTCB->pxTaskTag( pvParameter );
			}
			else
			{
				xReturn = pdFAIL;
			}

			return xReturn;
		}

#endif /* configUSE_APPLICATION_TASK_TAG */
		/*-----------------------------------------------------------*/
		/*
void SchedulerNewTaskIntrHandl(void)
{
	//portDISABLE_INTERRUPTS();
	//printf("new task, ptr: %X", *((u32*)0x20018000));
	//Xil_MemCpy(pxCurrentTCB_ptr, (u32*)0x20018000, (u32)4);
	//portCPU_IRQ_DISABLE();
	//vPortSaveTaskContext();
	pxCurrentTCB=*((u32*)0x20018000);
	SCHEDULER_ACKInterrupt(SCHEDULER_BASEADDR);
}*/

		void vTaskSwitchContext(void) {
			if (uxSchedulerSuspended != (UBaseType_t) pdFALSE) {
				/* The scheduler is currently suspended - do not allow a context
				 * switch. */
				xYieldPending = pdTRUE;
			} else {
				xYieldPending = pdFALSE;
				traceTASK_SWITCHED_OUT();

#if ( configGENERATE_RUN_TIME_STATS == 1 )
				{
#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
					portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime );
#else
					ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
#endif

					/* Add the amount of time the task has been running to the
					 * accumulated time so far.  The time the task started running was
					 * stored in ulTaskSwitchedInTime.  Note that there is no overflow
					 * protection here so count values are only valid until the timer
					 * overflows.  The guard against negative values is to protect
					 * against suspect run time stat counter implementations - which
					 * are provided by the application, not the kernel. */
					if( ulTotalRunTime > ulTaskSwitchedInTime )
					{
						pxCurrentTCB->ulRunTimeCounter += ( ulTotalRunTime - ulTaskSwitchedInTime );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					ulTaskSwitchedInTime = ulTotalRunTime;
				}
#endif /* configGENERATE_RUN_TIME_STATS */

				/* Check for stack overflow, if configured. */
				taskCHECK_FOR_STACK_OVERFLOW()
				;

				/* Before the currently running task is switched out, save its errno. */
#if ( configUSE_POSIX_ERRNO == 1 )
				{
					pxCurrentTCB->iTaskErrno = FreeRTOS_errno;
				}
#endif
				//ORIG____________
				///* Select a new task to run using either the generic C or port
				// * optimised asm code. */
				//taskSELECT_HIGHEST_PRIORITY_TASK()
				//; /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
				//ORIG______________
				//fedit add
				//always schedule the idle task, FPGA will cause an interrupt when a new task is available
				pxCurrentTCB = pxIdleTCB;

				traceTASK_SWITCHED_IN();

				/* After the new task is switched in, update the global errno. */
#if ( configUSE_POSIX_ERRNO == 1 )
				{
					FreeRTOS_errno = pxCurrentTCB->iTaskErrno;
				}
#endif

#if ( configUSE_NEWLIB_REENTRANT == 1 )
				{
					/* Switch Newlib's _impure_ptr variable to point to the _reent
					 * structure specific to this task.
					 * See the third party link http://www.nadler.com/embedded/newlibAndFreeRTOS.html
					 * for additional information. */
					_impure_ptr = &( pxCurrentTCB->xNewLib_reent );
				}
#endif /* configUSE_NEWLIB_REENTRANT */
			}
		}
		/*-----------------------------------------------------------*/

		void vTaskPlaceOnEventList(List_t * const pxEventList,
				const TickType_t xTicksToWait) {
			configASSERT(pxEventList);

			/* THIS FUNCTION MUST BE CALLED WITH EITHER INTERRUPTS DISABLED OR THE
			 * SCHEDULER SUSPENDED AND THE QUEUE BEING ACCESSED LOCKED. */

			/* Place the event list item of the TCB in the appropriate event list.
			 * This is placed in the list in priority order so the highest priority task
			 * is the first to be woken by the event.  The queue that contains the event
			 * list is locked, preventing simultaneous access from interrupts. */
			vListInsert(pxEventList, &(pxCurrentTCB->xEventListItem));

			prvAddCurrentTaskToDelayedList(xTicksToWait, pdTRUE);
		}
		/*-----------------------------------------------------------*/

		void vTaskPlaceOnUnorderedEventList(List_t * pxEventList,
				const TickType_t xItemValue, const TickType_t xTicksToWait) {
			configASSERT(pxEventList);

			/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.  It is used by
			 * the event groups implementation. */
			configASSERT(uxSchedulerSuspended != 0);

			/* Store the item value in the event list item.  It is safe to access the
			 * event list item here as interrupts won't access the event list item of a
			 * task that is not in the Blocked state. */
			listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xEventListItem),
					xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE);

			/* Place the event list item of the TCB at the end of the appropriate event
			 * list.  It is safe to access the event list here because it is part of an
			 * event group implementation - and interrupts don't access event groups
			 * directly (instead they access them indirectly by pending function calls to
			 * the task level). */
			vListInsertEnd(pxEventList, &(pxCurrentTCB->xEventListItem));

			prvAddCurrentTaskToDelayedList(xTicksToWait, pdTRUE);
		}
		/*-----------------------------------------------------------*/

#if ( configUSE_TIMERS == 1 )

		void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
				TickType_t xTicksToWait,
				const BaseType_t xWaitIndefinitely )
		{
			configASSERT( pxEventList );

			/* This function should not be called by application code hence the
			 * 'Restricted' in its name.  It is not part of the public API.  It is
			 * designed for use by kernel code, and has special calling requirements -
			 * it should be called with the scheduler suspended. */

			/* Place the event list item of the TCB in the appropriate event list.
			 * In this case it is assume that this is the only task that is going to
			 * be waiting on this event list, so the faster vListInsertEnd() function
			 * can be used in place of vListInsert. */
			vListInsertEnd( pxEventList, &( pxCurrentTCB->xEventListItem ) );

			/* If the task should block indefinitely then set the block time to a
			 * value that will be recognised as an indefinite delay inside the
			 * prvAddCurrentTaskToDelayedList() function. */
			if( xWaitIndefinitely != pdFALSE )
			{
				xTicksToWait = portMAX_DELAY;
			}

			traceTASK_DELAY_UNTIL( ( xTickCount + xTicksToWait ) );
			prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );
		}

#endif /* configUSE_TIMERS */
		/*-----------------------------------------------------------*/

		BaseType_t xTaskRemoveFromEventList(const List_t * const pxEventList) {
			TCB_t * pxUnblockedTCB;
			BaseType_t xReturn;

			/* THIS FUNCTION MUST BE CALLED FROM A CRITICAL SECTION.  It can also be
			 * called from a critical section within an ISR. */

			/* The event list is sorted in priority order, so the first in the list can
			 * be removed as it is known to be the highest priority.  Remove the TCB from
			 * the delayed list, and add it to the ready list.
			 *
			 * If an event is for a queue that is locked then this function will never
			 * get called - the lock count on the queue will get modified instead.  This
			 * means exclusive access to the event list is guaranteed here.
			 *
			 * This function assumes that a check has already been made to ensure that
			 * pxEventList is not empty. */
			pxUnblockedTCB = listGET_OWNER_OF_HEAD_ENTRY(pxEventList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
			configASSERT(pxUnblockedTCB);
			(void) uxListRemove(&(pxUnblockedTCB->xEventListItem));

			if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
				(void) uxListRemove(&(pxUnblockedTCB->xStateListItem));
				prvAddTaskToReadyList(pxUnblockedTCB);

#if ( configUSE_TICKLESS_IDLE != 0 )
				{
					/* If a task is blocked on a kernel object then xNextTaskUnblockTime
					 * might be set to the blocked task's time out time.  If the task is
					 * unblocked for a reason other than a timeout xNextTaskUnblockTime is
					 * normally left unchanged, because it is automatically reset to a new
					 * value when the tick count equals xNextTaskUnblockTime.  However if
					 * tickless idling is used it might be more important to enter sleep mode
					 * at the earliest possible time - so reset xNextTaskUnblockTime here to
					 * ensure it is updated at the earliest possible time. */
					prvResetNextTaskUnblockTime();
				}
#endif
			} else {
				/* The delayed and ready lists cannot be accessed, so hold this task
				 * pending until the scheduler is resumed. */
				vListInsertEnd(&(xPendingReadyList), &(pxUnblockedTCB->xEventListItem));
			}

			if (pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority) {
				/* Return true if the task removed from the event list has a higher
				 * priority than the calling task.  This allows the calling task to know if
				 * it should force a context switch now. */
				xReturn = pdTRUE;

				/* Mark that a yield is pending in case the user is not using the
				 * "xHigherPriorityTaskWoken" parameter to an ISR safe FreeRTOS function. */
				xYieldPending = pdTRUE;
			} else {
				xReturn = pdFALSE;
			}

			return xReturn;
		}
		/*-----------------------------------------------------------*/

		void vTaskRemoveFromUnorderedEventList(ListItem_t * pxEventListItem,
				const TickType_t xItemValue) {
			TCB_t * pxUnblockedTCB;

			/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.  It is used by
			 * the event flags implementation. */
			configASSERT(uxSchedulerSuspended != pdFALSE);

			/* Store the new item value in the event list. */
			listSET_LIST_ITEM_VALUE(pxEventListItem,
					xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE);

			/* Remove the event list form the event flag.  Interrupts do not access
			 * event flags. */
			pxUnblockedTCB = listGET_LIST_ITEM_OWNER(pxEventListItem); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
			configASSERT(pxUnblockedTCB);
			(void) uxListRemove(pxEventListItem);

#if ( configUSE_TICKLESS_IDLE != 0 )
			{
				/* If a task is blocked on a kernel object then xNextTaskUnblockTime
				 * might be set to the blocked task's time out time.  If the task is
				 * unblocked for a reason other than a timeout xNextTaskUnblockTime is
				 * normally left unchanged, because it is automatically reset to a new
				 * value when the tick count equals xNextTaskUnblockTime.  However if
				 * tickless idling is used it might be more important to enter sleep mode
				 * at the earliest possible time - so reset xNextTaskUnblockTime here to
				 * ensure it is updated at the earliest possible time. */
				prvResetNextTaskUnblockTime();
			}
#endif

			/* Remove the task from the delayed list and add it to the ready list.  The
			 * scheduler is suspended so interrupts will not be accessing the ready
			 * lists. */
			(void) uxListRemove(&(pxUnblockedTCB->xStateListItem));
			prvAddTaskToReadyList(pxUnblockedTCB);

			if (pxUnblockedTCB->uxPriority > pxCurrentTCB->uxPriority) {
				/* The unblocked task has a priority above that of the calling task, so
				 * a context switch is required.  This function is called with the
				 * scheduler suspended so xYieldPending is set so the context switch
				 * occurs immediately that the scheduler is resumed (unsuspended). */
				xYieldPending = pdTRUE;
			}
		}
		/*-----------------------------------------------------------*/

		void vTaskSetTimeOutState(TimeOut_t * const pxTimeOut) {
			configASSERT(pxTimeOut);
			taskENTER_CRITICAL()
			;
			{
				pxTimeOut->xOverflowCount = xNumOfOverflows;
				pxTimeOut->xTimeOnEntering = xTickCount;
			}
			taskEXIT_CRITICAL()
			;
		}
		/*-----------------------------------------------------------*/

		void vTaskInternalSetTimeOutState(TimeOut_t * const pxTimeOut) {
			/* For internal use only as it does not use a critical section. */
			pxTimeOut->xOverflowCount = xNumOfOverflows;
			pxTimeOut->xTimeOnEntering = xTickCount;
		}
		/*-----------------------------------------------------------*/

		BaseType_t xTaskCheckForTimeOut(TimeOut_t * const pxTimeOut,
				TickType_t * const pxTicksToWait) {
			BaseType_t xReturn;

			configASSERT(pxTimeOut);
			configASSERT(pxTicksToWait);

			taskENTER_CRITICAL()
			;
			{
				/* Minor optimisation.  The tick count cannot change in this block. */
				const TickType_t xConstTickCount = xTickCount;
				const TickType_t xElapsedTime = xConstTickCount
						- pxTimeOut->xTimeOnEntering;

#if ( INCLUDE_xTaskAbortDelay == 1 )
				if( pxCurrentTCB->ucDelayAborted != ( uint8_t ) pdFALSE )
				{
					/* The delay was aborted, which is not the same as a time out,
					 * but has the same result. */
					pxCurrentTCB->ucDelayAborted = pdFALSE;
					xReturn = pdTRUE;
				}
				else
#endif

#if ( INCLUDE_vTaskSuspend == 1 )
					if (*pxTicksToWait == portMAX_DELAY) {
						/* If INCLUDE_vTaskSuspend is set to 1 and the block time
						 * specified is the maximum block time then the task should block
						 * indefinitely, and therefore never time out. */
						xReturn = pdFALSE;
					} else
#endif

						if ((xNumOfOverflows != pxTimeOut->xOverflowCount)
								&& (xConstTickCount >= pxTimeOut->xTimeOnEntering)) /*lint !e525 Indentation preferred as is to make code within pre-processor directives clearer. */
						{
							/* The tick count is greater than the time at which
							 * vTaskSetTimeout() was called, but has also overflowed since
							 * vTaskSetTimeOut() was called.  It must have wrapped all the way
							 * around and gone past again. This passed since vTaskSetTimeout()
							 * was called. */
							xReturn = pdTRUE;
							*pxTicksToWait = (TickType_t) 0;
						} else if (xElapsedTime < *pxTicksToWait) /*lint !e961 Explicit casting is only redundant with some compilers, whereas others require it to prevent integer conversion errors. */
						{
							/* Not a genuine timeout. Adjust parameters for time remaining. */
							*pxTicksToWait -= xElapsedTime;
							vTaskInternalSetTimeOutState(pxTimeOut);
							xReturn = pdFALSE;
						} else {
							*pxTicksToWait = (TickType_t) 0;
							xReturn = pdTRUE;
						}
			}
			taskEXIT_CRITICAL()
			;

			return xReturn;
		}
		/*-----------------------------------------------------------*/

		void vTaskMissedYield(void) {
			xYieldPending = pdTRUE;
		}
		/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

		UBaseType_t uxTaskGetTaskNumber(TaskHandle_t xTask) {
			UBaseType_t uxReturn;
			TCB_t const * pxTCB;

			if (xTask != NULL) {
				pxTCB = xTask;
				uxReturn = pxTCB->uxTaskNumber;
			} else {
				uxReturn = 0U;
			}

			return uxReturn;
		}

#endif /* configUSE_TRACE_FACILITY */
		/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

		void vTaskSetTaskNumber(TaskHandle_t xTask, const UBaseType_t uxHandle) {
			TCB_t * pxTCB;

			if (xTask != NULL) {
				pxTCB = xTask;
				pxTCB->uxTaskNumber = uxHandle;
			}
		}

#endif /* configUSE_TRACE_FACILITY */

		/*
		 * -----------------------------------------------------------
		 * The Idle task.
		 * ----------------------------------------------------------
		 *
		 * The portTASK_FUNCTION() macro is used to allow port/compiler specific
		 * language extensions.  The equivalent prototype for this function is:
		 *
		 * void prvIdleTask( void *pvParameters );
		 *
		 */
		static portTASK_FUNCTION( prvIdleTask, pvParameters ) {
			/* Stop warnings. */
			(void) pvParameters;

			/** THIS IS THE RTOS IDLE TASK - WHICH IS CREATED AUTOMATICALLY WHEN THE
			 * SCHEDULER IS STARTED. **/

			/* In case a task that has a secure context deletes itself, in which case
			 * the idle task is responsible for deleting the task's secure context, if
			 * any. */
			portALLOCATE_SECURE_CONTEXT( configMINIMAL_SECURE_STACK_SIZE );

			for (;;) {
				/* See if any tasks have deleted themselves - if so then the idle task
				 * is responsible for freeing the deleted task's TCB and stack. */
				prvCheckTasksWaitingTermination();

				//fedit add
				//    	printf("idle task running");
				//fedit remove : removed yelds from IDLE task since tasks to be allocated will be called via an interrupt of the fpga-queue is not available to PS at the moment, idle task will simply run until preempted by someone else
				//        #if ( configUSE_PREEMPTION == 0 )
				//            {
				//                /* If we are not using preemption we keep forcing a task switch to
				//                 * see if any other task has become available.  If we are using
				//                 * preemption we don't need to do this as any task becoming available
				//                 * will automatically get the processor anyway. */
				//                taskYIELD();
				//            }
				//        #endif /* configUSE_PREEMPTION */
				//
				//        #if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
				//            {
				//              /* When using preemption tasks of equal priority will be
				//             * timesliced.  If a task that is sharing the idle priority is ready
				//                 * to run then the idle task should yield before the end of the
				//                 * timeslice.
				//                 *
				//                 * A critical region is not required here as we are just reading from
				//                 * the list, and an occasional incorrect value will not matter.  If
				//                 * the ready list at the idle priority contains more than one task
				//                 * then a task other than the idle task is ready to execute. */
				//                if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) 1 )
				//                {
				//                    taskYIELD();
				//                }
				//                else
				//                {
				//                    mtCOVERAGE_TEST_MARKER();
				//                }
				//            }
				//        #endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) ) */
				//
				//#if ( configUSE_IDLE_HOOK == 1 )
				//		{
				//			extern void vApplicationIdleHook( void );
				//
				//			/* Call the user defined function from within the idle task.  This
				//			 * allows the application designer to add background functionality
				//			 * without the overhead of a separate task.
				//			 * NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
				//			 * CALL A FUNCTION THAT MIGHT BLOCK. */
				//			vApplicationIdleHook();
				//		}
				//#endif /* configUSE_IDLE_HOOK */
				//
				//		/* This conditional compilation should use inequality to 0, not equality
				//		 * to 1.  This is to ensure portSUPPRESS_TICKS_AND_SLEEP() is called when
				//		 * user defined low power mode  implementations require
				//		 * configUSE_TICKLESS_IDLE to be set to a value other than 1. */
				//#if ( configUSE_TICKLESS_IDLE != 0 )
				//		{
				//			TickType_t xExpectedIdleTime;
				//
				//			/* It is not desirable to suspend then resume the scheduler on
				//			 * each iteration of the idle task.  Therefore, a preliminary
				//			 * test of the expected idle time is performed without the
				//			 * scheduler suspended.  The result here is not necessarily
				//			 * valid. */
				//			xExpectedIdleTime = prvGetExpectedIdleTime();
				//
				//			if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
				//			{
				//				vTaskSuspendAll();
				//				{
				//					/* Now the scheduler is suspended, the expected idle
				//					 * time can be sampled again, and this time its value can
				//					 * be used. */
				//					configASSERT( xNextTaskUnblockTime >= xTickCount );
				//					xExpectedIdleTime = prvGetExpectedIdleTime();
				//
				//					/* Define the following macro to set xExpectedIdleTime to 0
				//					 * if the application does not want
				//					 * portSUPPRESS_TICKS_AND_SLEEP() to be called. */
				//					configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( xExpectedIdleTime );
				//
				//					if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
				//					{
				//						traceLOW_POWER_IDLE_BEGIN();
				//						portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
				//						traceLOW_POWER_IDLE_END();
				//					}
				//					else
				//					{
				//						mtCOVERAGE_TEST_MARKER();
				//					}
				//				}
				//				( void ) xTaskResumeAll();
				//			}
				//			else
				//			{
				//				mtCOVERAGE_TEST_MARKER();
				//			}
				//		}
				//#endif /* configUSE_TICKLESS_IDLE */
			}
		}
		/*-----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE != 0 )

		eSleepModeStatus eTaskConfirmSleepModeStatus( void )
		{
			/* The idle task exists in addition to the application tasks. */
			const UBaseType_t uxNonApplicationTasks = 1;
			eSleepModeStatus eReturn = eStandardSleep;

			/* This function must be called from a critical section. */

			if( listCURRENT_LIST_LENGTH( &xPendingReadyList ) != 0 )
			{
				/* A task was made ready while the scheduler was suspended. */
				eReturn = eAbortSleep;
			}
			else if( xYieldPending != pdFALSE )
			{
				/* A yield was pended while the scheduler was suspended. */
				eReturn = eAbortSleep;
			}
			else if( xPendedTicks != 0 )
			{
				/* A tick interrupt has already occurred but was held pending
				 * because the scheduler is suspended. */
				eReturn = eAbortSleep;
			}
			else
			{
				/* If all the tasks are in the suspended list (which might mean they
				 * have an infinite block time rather than actually being suspended)
				 * then it is safe to turn all clocks off and just wait for external
				 * interrupts. */
				if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == ( uxCurrentNumberOfTasks - uxNonApplicationTasks ) )
				{
					eReturn = eNoTasksWaitingTimeout;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}

			return eReturn;
		}

#endif /* configUSE_TICKLESS_IDLE */
		/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

		void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
				BaseType_t xIndex,
				void * pvValue )
		{
			TCB_t * pxTCB;

			if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
			{
				pxTCB = prvGetTCBFromHandle( xTaskToSet );
				configASSERT( pxTCB != NULL );
				pxTCB->pvThreadLocalStoragePointers[ xIndex ] = pvValue;
			}
		}

#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS */
		/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

		void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
				BaseType_t xIndex )
		{
			void * pvReturn = NULL;
			TCB_t * pxTCB;

			if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
			{
				pxTCB = prvGetTCBFromHandle( xTaskToQuery );
				pvReturn = pxTCB->pvThreadLocalStoragePointers[ xIndex ];
			}
			else
			{
				pvReturn = NULL;
			}

			return pvReturn;
		}

#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS */
		/*-----------------------------------------------------------*/

#if ( portUSING_MPU_WRAPPERS == 1 )

		void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
				const MemoryRegion_t * const xRegions )
		{
			TCB_t * pxTCB;

			/* If null is passed in here then we are modifying the MPU settings of
			 * the calling task. */
			pxTCB = prvGetTCBFromHandle( xTaskToModify );

			vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, NULL, 0 );
		}

#endif /* portUSING_MPU_WRAPPERS */
		/*-----------------------------------------------------------*/

		static void prvInitialiseTaskLists(void) {
			//fedit add
			//vListInitialise( &pxRTTasksList );
			/*for (int i=0; i<configMAX_RT_TASKS; i++) {
	 pxRTTasksList[i]=0;
	 }*/

			//todo remove other lists init on next steps of project
			UBaseType_t uxPriority;

			for (uxPriority = (UBaseType_t) 0U;
					uxPriority < (UBaseType_t) configMAX_PRIORITIES; uxPriority++) {
				vListInitialise(&(pxReadyTasksLists[uxPriority]));
			}

			vListInitialise(&xDelayedTaskList1);
			vListInitialise(&xDelayedTaskList2);
			vListInitialise(&xPendingReadyList);

#if ( INCLUDE_vTaskDelete == 1 )
			{
				vListInitialise(&xTasksWaitingTermination);
			}
#endif /* INCLUDE_vTaskDelete */

#if ( INCLUDE_vTaskSuspend == 1 )
			{
				vListInitialise(&xSuspendedTaskList);
			}
#endif /* INCLUDE_vTaskSuspend */

			/* Start with pxDelayedTaskList using list1 and the pxOverflowDelayedTaskList
			 * using list2. */
			pxDelayedTaskList = &xDelayedTaskList1;
			pxOverflowDelayedTaskList = &xDelayedTaskList2;
		}
		/*-----------------------------------------------------------*/

		static void prvCheckTasksWaitingTermination(void) {
			/** THIS FUNCTION IS CALLED FROM THE RTOS IDLE TASK **/

#if ( INCLUDE_vTaskDelete == 1 )
			{
				TCB_t * pxTCB;

				/* uxDeletedTasksWaitingCleanUp is used to prevent taskENTER_CRITICAL()
				 * being called too often in the idle task. */
				while (uxDeletedTasksWaitingCleanUp > (UBaseType_t) 0U) {
					taskENTER_CRITICAL()
																					;
					{
						pxTCB = listGET_OWNER_OF_HEAD_ENTRY(
								(&xTasksWaitingTermination)); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
						(void) uxListRemove(&(pxTCB->xStateListItem));
						--uxCurrentNumberOfTasks;
						--uxDeletedTasksWaitingCleanUp;
					}
					taskEXIT_CRITICAL()
					;

					prvDeleteTCB(pxTCB);
				}
			}
#endif /* INCLUDE_vTaskDelete */
		}
		/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

		void vTaskGetInfo(TaskHandle_t xTask, TaskStatus_t * pxTaskStatus,
				BaseType_t xGetFreeStackSpace, eTaskState eState) {
			TCB_t * pxTCB;

			/* xTask is NULL then get the state of the calling task. */
			pxTCB = prvGetTCBFromHandle(xTask);

			pxTaskStatus->xHandle = (TaskHandle_t) pxTCB;
			pxTaskStatus->pcTaskName = (const char *) &(pxTCB->pcTaskName[0]);
			pxTaskStatus->uxCurrentPriority = pxTCB->uxPriority;
			pxTaskStatus->pxStackBase = pxTCB->pxStack;
			pxTaskStatus->xTaskNumber = pxTCB->uxTCBNumber;

#if ( configUSE_MUTEXES == 1 )
			{
				pxTaskStatus->uxBasePriority = pxTCB->uxBasePriority;
			}
#else
			{
				pxTaskStatus->uxBasePriority = 0;
			}
#endif

#if ( configGENERATE_RUN_TIME_STATS == 1 )
			{
				pxTaskStatus->ulRunTimeCounter = pxTCB->ulRunTimeCounter;
			}
#else
			{
				pxTaskStatus->ulRunTimeCounter = 0;
			}
#endif

			/* Obtaining the task state is a little fiddly, so is only done if the
			 * value of eState passed into this function is eInvalid - otherwise the
			 * state is just set to whatever is passed in. */
			if (eState != eInvalid) {
				if (pxTCB == pxCurrentTCB) {
					pxTaskStatus->eCurrentState = eRunning;
				} else {
					pxTaskStatus->eCurrentState = eState;

#if ( INCLUDE_vTaskSuspend == 1 )
					{
						/* If the task is in the suspended list then there is a
						 *  chance it is actually just blocked indefinitely - so really
						 *  it should be reported as being in the Blocked state. */
						if (eState == eSuspended) {
							vTaskSuspendAll();
							{
								if ( listLIST_ITEM_CONTAINER(
										&(pxTCB->xEventListItem)) != NULL) {
									pxTaskStatus->eCurrentState = eBlocked;
								}
							}
							(void) xTaskResumeAll();
						}
					}
#endif /* INCLUDE_vTaskSuspend */
				}
			} else {
				pxTaskStatus->eCurrentState = eTaskGetState(pxTCB);
			}

			/* Obtaining the stack space takes some time, so the xGetFreeStackSpace
			 * parameter is provided to allow it to be skipped. */
			if (xGetFreeStackSpace != pdFALSE) {
#if ( portSTACK_GROWTH > 0 )
				{
					pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxEndOfStack );
				}
#else
				{
					pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace(
							(uint8_t *) pxTCB->pxStack);
				}
#endif
			} else {
				pxTaskStatus->usStackHighWaterMark = 0;
			}
		}

#endif /* configUSE_TRACE_FACILITY */
		/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

		static UBaseType_t prvListTasksWithinSingleList(
				TaskStatus_t * pxTaskStatusArray, List_t * pxList, eTaskState eState) {
			configLIST_VOLATILE TCB_t * pxNextTCB, *pxFirstTCB;
			UBaseType_t uxTask = 0;

			if ( listCURRENT_LIST_LENGTH( pxList ) > (UBaseType_t) 0) {
				listGET_OWNER_OF_NEXT_ENTRY(pxFirstTCB, pxList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */

				/* Populate an TaskStatus_t structure within the
				 * pxTaskStatusArray array for each task that is referenced from
				 * pxList.  See the definition of TaskStatus_t in task.h for the
				 * meaning of each TaskStatus_t structure member. */
				do {
					listGET_OWNER_OF_NEXT_ENTRY(pxNextTCB, pxList); /*lint !e9079 void * is used as this macro is used with timers and co-routines too.  Alignment is known to be fine as the type of the pointer stored and retrieved is the same. */
					vTaskGetInfo((TaskHandle_t) pxNextTCB, &(pxTaskStatusArray[uxTask]),
							pdTRUE, eState);
					uxTask++;
				} while (pxNextTCB != pxFirstTCB);
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return uxTask;
		}

#endif /* configUSE_TRACE_FACILITY */
		/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) )

		static configSTACK_DEPTH_TYPE prvTaskCheckFreeStackSpace(
				const uint8_t * pucStackByte) {
			uint32_t ulCount = 0U;

			while (*pucStackByte == (uint8_t) tskSTACK_FILL_BYTE) {
				pucStackByte -= portSTACK_GROWTH;
				ulCount++;
			}

			ulCount /= (uint32_t) sizeof(StackType_t); /*lint !e961 Casting is not redundant on smaller architectures. */

			return ( configSTACK_DEPTH_TYPE ) ulCount;
		}

#endif /* ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 ) ) */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark2 == 1 )

		/* uxTaskGetStackHighWaterMark() and uxTaskGetStackHighWaterMark2() are the
		 * same except for their return type.  Using configSTACK_DEPTH_TYPE allows the
		 * user to determine the return type.  It gets around the problem of the value
		 * overflowing on 8-bit types without breaking backward compatibility for
		 * applications that expect an 8-bit return type. */
		configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask )
		{
			TCB_t * pxTCB;
			uint8_t * pucEndOfStack;
			configSTACK_DEPTH_TYPE uxReturn;

			/* uxTaskGetStackHighWaterMark() and uxTaskGetStackHighWaterMark2() are
			 * the same except for their return type.  Using configSTACK_DEPTH_TYPE
			 * allows the user to determine the return type.  It gets around the
			 * problem of the value overflowing on 8-bit types without breaking
			 * backward compatibility for applications that expect an 8-bit return
			 * type. */

			pxTCB = prvGetTCBFromHandle( xTask );

#if portSTACK_GROWTH < 0
			{
				pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
			}
#else
			{
				pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
			}
#endif

			uxReturn = prvTaskCheckFreeStackSpace( pucEndOfStack );

			return uxReturn;
		}

#endif /* INCLUDE_uxTaskGetStackHighWaterMark2 */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

		UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
		{
			TCB_t * pxTCB;
			uint8_t * pucEndOfStack;
			UBaseType_t uxReturn;

			pxTCB = prvGetTCBFromHandle( xTask );

#if portSTACK_GROWTH < 0
			{
				pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
			}
#else
			{
				pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
			}
#endif

			uxReturn = ( UBaseType_t ) prvTaskCheckFreeStackSpace( pucEndOfStack );

			return uxReturn;
		}

#endif /* INCLUDE_uxTaskGetStackHighWaterMark */
		/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )

		static void prvDeleteTCB(TCB_t * pxTCB) {
			/* This call is required specifically for the TriCore port.  It must be
			 * above the vPortFree() calls.  The call is also used by ports/demos that
			 * want to allocate and clean RAM statically. */
			portCLEAN_UP_TCB(pxTCB);

			/* Free up the memory allocated by the scheduler for the task.  It is up
			 * to the task to free any memory allocated at the application level.
			 * See the third party link http://www.nadler.com/embedded/newlibAndFreeRTOS.html
			 * for additional information. */
#if ( configUSE_NEWLIB_REENTRANT == 1 )
			{
				_reclaim_reent( &( pxTCB->xNewLib_reent ) );
			}
#endif /* configUSE_NEWLIB_REENTRANT */

#if ( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
			{
				/* The task can only have been allocated dynamically - free both
				 * the stack and TCB. */
				vPortFree(pxTCB->pxStack);
				vPortFree(pxTCB);
			}
#elif ( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 ) /*lint !e731 !e9029 Macro has been consolidated for readability reasons. */
			{
				/* The task could have been allocated statically or dynamically, so
				 * check what was statically allocated before trying to free the
				 * memory. */
				if( pxTCB->ucStaticallyAllocated == tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB )
				{
					/* Both the stack and TCB were allocated dynamically, so both
					 * must be freed. */
					vPortFree( pxTCB->pxStack );
					vPortFree( pxTCB );
				}
				else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
				{
					/* Only the stack was statically allocated, so the TCB is the
					 * only memory that must be freed. */
					vPortFree( pxTCB );
				}
				else
				{
					/* Neither the stack nor the TCB were allocated dynamically, so
					 * nothing needs to be freed. */
					configASSERT( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB );
					mtCOVERAGE_TEST_MARKER();
				}
			}
#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
		}

#endif /* INCLUDE_vTaskDelete */
		/*-----------------------------------------------------------*/

		static void prvResetNextTaskUnblockTime(void) {
			if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE) {
				/* The new current delayed list is empty.  Set xNextTaskUnblockTime to
				 * the maximum possible value so it is  extremely unlikely that the
				 * if( xTickCount >= xNextTaskUnblockTime ) test will pass until
				 * there is an item in the delayed list. */
				xNextTaskUnblockTime = portMAX_DELAY;
			} else {
				/* The new current delayed list is not empty, get the value of
				 * the item at the head of the delayed list.  This is the time at
				 * which the task at the head of the delayed list should be removed
				 * from the Blocked state. */
				xNextTaskUnblockTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY(
						pxDelayedTaskList);
			}
		}
		/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )

		TaskHandle_t xTaskGetCurrentTaskHandle(void) {
			TaskHandle_t xReturn;

			/* A critical section is not required as this is not called from
			 * an interrupt and the current TCB will always be the same for any
			 * individual execution thread. */
			xReturn = pxCurrentTCB;

			return xReturn;
		}

#endif /* ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) ) */
		/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )

		BaseType_t xTaskGetSchedulerState( void )
		{
			BaseType_t xReturn;

			if( xSchedulerRunning == pdFALSE )
			{
				xReturn = taskSCHEDULER_NOT_STARTED;
			}
			else
			{
				if( uxSchedulerSuspended == ( UBaseType_t ) pdFALSE )
				{
					xReturn = taskSCHEDULER_RUNNING;
				}
				else
				{
					xReturn = taskSCHEDULER_SUSPENDED;
				}
			}

			return xReturn;
		}

#endif /* ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) ) */
		/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

		BaseType_t xTaskPriorityInherit(TaskHandle_t const pxMutexHolder) {
			TCB_t * const pxMutexHolderTCB = pxMutexHolder;
			BaseType_t xReturn = pdFALSE;

			/* If the mutex was given back by an interrupt while the queue was
			 * locked then the mutex holder might now be NULL.  _RB_ Is this still
			 * needed as interrupts can no longer use mutexes? */
			if (pxMutexHolder != NULL) {
				/* If the holder of the mutex has a priority below the priority of
				 * the task attempting to obtain the mutex then it will temporarily
				 * inherit the priority of the task attempting to obtain the mutex. */
				if (pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority) {
					/* Adjust the mutex holder state to account for its new
					 * priority.  Only reset the event list item value if the value is
					 * not being used for anything else. */
					if (( listGET_LIST_ITEM_VALUE(&(pxMutexHolderTCB->xEventListItem))
							& taskEVENT_LIST_ITEM_VALUE_IN_USE) == 0UL) {
						listSET_LIST_ITEM_VALUE(&(pxMutexHolderTCB->xEventListItem),
								( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
					} else {
						mtCOVERAGE_TEST_MARKER();
					}

					/* If the task being modified is in the ready state it will need
					 * to be moved into a new list. */
					if ( listIS_CONTAINED_WITHIN(
							&(pxReadyTasksLists[pxMutexHolderTCB->uxPriority]),
							&(pxMutexHolderTCB->xStateListItem)) != pdFALSE) {
						if (uxListRemove(&(pxMutexHolderTCB->xStateListItem))
								== (UBaseType_t) 0) {
							/* It is known that the task is in its ready list so
							 * there is no need to check again and the port level
							 * reset macro can be called directly. */
							portRESET_READY_PRIORITY(pxMutexHolderTCB->uxPriority,
									uxTopReadyPriority);
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						/* Inherit the priority before being moved into the new list. */
						pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
						prvAddTaskToReadyList(pxMutexHolderTCB);
					} else {
						/* Just inherit the priority. */
						pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
					}

					traceTASK_PRIORITY_INHERIT( pxMutexHolderTCB, pxCurrentTCB->uxPriority );

					/* Inheritance occurred. */
					xReturn = pdTRUE;
				} else {
					if (pxMutexHolderTCB->uxBasePriority < pxCurrentTCB->uxPriority) {
						/* The base priority of the mutex holder is lower than the
						 * priority of the task attempting to take the mutex, but the
						 * current priority of the mutex holder is not lower than the
						 * priority of the task attempting to take the mutex.
						 * Therefore the mutex holder must have already inherited a
						 * priority, but inheritance would have occurred if that had
						 * not been the case. */
						xReturn = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return xReturn;
		}

#endif /* configUSE_MUTEXES */
		/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

		BaseType_t xTaskPriorityDisinherit(TaskHandle_t const pxMutexHolder) {
			TCB_t * const pxTCB = pxMutexHolder;
			BaseType_t xReturn = pdFALSE;

			if (pxMutexHolder != NULL) {
				/* A task can only have an inherited priority if it holds the mutex.
				 * If the mutex is held by a task then it cannot be given from an
				 * interrupt, and if a mutex is given by the holding task then it must
				 * be the running state task. */
				configASSERT(pxTCB == pxCurrentTCB);
				configASSERT(pxTCB->uxMutexesHeld);
				(pxTCB->uxMutexesHeld)--;

				/* Has the holder of the mutex inherited the priority of another
				 * task? */
				if (pxTCB->uxPriority != pxTCB->uxBasePriority) {
					/* Only disinherit if no other mutexes are held. */
					if (pxTCB->uxMutexesHeld == (UBaseType_t) 0) {
						/* A task can only have an inherited priority if it holds
						 * the mutex.  If the mutex is held by a task then it cannot be
						 * given from an interrupt, and if a mutex is given by the
						 * holding task then it must be the running state task.  Remove
						 * the holding task from the ready list. */
						if (uxListRemove(&(pxTCB->xStateListItem)) == (UBaseType_t) 0) {
							portRESET_READY_PRIORITY(pxTCB->uxPriority,
									uxTopReadyPriority);
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						/* Disinherit the priority before adding the task into the
						 * new  ready list. */
						traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
						pxTCB->uxPriority = pxTCB->uxBasePriority;

						/* Reset the event list item value.  It cannot be in use for
						 * any other purpose if this task is running, and it must be
						 * running to give back the mutex. */
						listSET_LIST_ITEM_VALUE(&(pxTCB->xEventListItem),
								( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxTCB->uxPriority); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
						prvAddTaskToReadyList(pxTCB);

						/* Return true to indicate that a context switch is required.
						 * This is only actually required in the corner case whereby
						 * multiple mutexes were held and the mutexes were given back
						 * in an order different to that in which they were taken.
						 * If a context switch did not occur when the first mutex was
						 * returned, even if a task was waiting on it, then a context
						 * switch should occur when the last mutex is returned whether
						 * a task is waiting on it or not. */
						xReturn = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

			return xReturn;
		}

#endif /* configUSE_MUTEXES */
		/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

		void vTaskPriorityDisinheritAfterTimeout(TaskHandle_t const pxMutexHolder,
				UBaseType_t uxHighestPriorityWaitingTask) {
			TCB_t * const pxTCB = pxMutexHolder;
			UBaseType_t uxPriorityUsedOnEntry, uxPriorityToUse;
			const UBaseType_t uxOnlyOneMutexHeld = (UBaseType_t) 1;

			if (pxMutexHolder != NULL) {
				/* If pxMutexHolder is not NULL then the holder must hold at least
				 * one mutex. */
				configASSERT(pxTCB->uxMutexesHeld);

				/* Determine the priority to which the priority of the task that
				 * holds the mutex should be set.  This will be the greater of the
				 * holding task's base priority and the priority of the highest
				 * priority task that is waiting to obtain the mutex. */
				if (pxTCB->uxBasePriority < uxHighestPriorityWaitingTask) {
					uxPriorityToUse = uxHighestPriorityWaitingTask;
				} else {
					uxPriorityToUse = pxTCB->uxBasePriority;
				}

				/* Does the priority need to change? */
				if (pxTCB->uxPriority != uxPriorityToUse) {
					/* Only disinherit if no other mutexes are held.  This is a
					 * simplification in the priority inheritance implementation.  If
					 * the task that holds the mutex is also holding other mutexes then
					 * the other mutexes may have caused the priority inheritance. */
					if (pxTCB->uxMutexesHeld == uxOnlyOneMutexHeld) {
						/* If a task has timed out because it already holds the
						 * mutex it was trying to obtain then it cannot of inherited
						 * its own priority. */
						configASSERT(pxTCB != pxCurrentTCB);

						/* Disinherit the priority, remembering the previous
						 * priority to facilitate determining the subject task's
						 * state. */
						traceTASK_PRIORITY_DISINHERIT( pxTCB, uxPriorityToUse );
						uxPriorityUsedOnEntry = pxTCB->uxPriority;
						pxTCB->uxPriority = uxPriorityToUse;

						/* Only reset the event list item value if the value is not
						 * being used for anything else. */
						if (( listGET_LIST_ITEM_VALUE(&(pxTCB->xEventListItem))
								& taskEVENT_LIST_ITEM_VALUE_IN_USE) == 0UL) {
							listSET_LIST_ITEM_VALUE(&(pxTCB->xEventListItem),
									( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriorityToUse); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
						} else {
							mtCOVERAGE_TEST_MARKER();
						}

						/* If the running task is not the task that holds the mutex
						 * then the task that holds the mutex could be in either the
						 * Ready, Blocked or Suspended states.  Only remove the task
						 * from its current state list if it is in the Ready state as
						 * the task's priority is going to change and there is one
						 * Ready list per priority. */
						if ( listIS_CONTAINED_WITHIN(
								&(pxReadyTasksLists[uxPriorityUsedOnEntry]),
								&(pxTCB->xStateListItem)) != pdFALSE) {
							if (uxListRemove(&(pxTCB->xStateListItem))
									== (UBaseType_t) 0) {
								/* It is known that the task is in its ready list so
								 * there is no need to check again and the port level
								 * reset macro can be called directly. */
								portRESET_READY_PRIORITY(pxTCB->uxPriority,
										uxTopReadyPriority);
							} else {
								mtCOVERAGE_TEST_MARKER();
							}

							prvAddTaskToReadyList(pxTCB);
						} else {
							mtCOVERAGE_TEST_MARKER();
						}
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* configUSE_MUTEXES */
		/*-----------------------------------------------------------*/

#if ( portCRITICAL_NESTING_IN_TCB == 1 )

		void vTaskEnterCritical( void )
		{
			portDISABLE_INTERRUPTS();

			if( xSchedulerRunning != pdFALSE )
			{
				( pxCurrentTCB->uxCriticalNesting )++;

				/* This is not the interrupt safe version of the enter critical
				 * function so  assert() if it is being called from an interrupt
				 * context.  Only API functions that end in "FromISR" can be used in an
				 * interrupt.  Only assert if the critical nesting count is 1 to
				 * protect against recursive calls if the assert function also uses a
				 * critical section. */
				if( pxCurrentTCB->uxCriticalNesting == 1 )
				{
					portASSERT_IF_IN_ISR();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* portCRITICAL_NESTING_IN_TCB */
		/*-----------------------------------------------------------*/

#if ( portCRITICAL_NESTING_IN_TCB == 1 )

		void vTaskExitCritical( void )
		{
			if( xSchedulerRunning != pdFALSE )
			{
				if( pxCurrentTCB->uxCriticalNesting > 0U )
				{
					( pxCurrentTCB->uxCriticalNesting )--;

					if( pxCurrentTCB->uxCriticalNesting == 0U )
					{
						portENABLE_INTERRUPTS();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* portCRITICAL_NESTING_IN_TCB */
		/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

		static char * prvWriteNameToBuffer(char * pcBuffer, const char * pcTaskName) {
			size_t x;

			/* Start by copying the entire string. */
			strcpy(pcBuffer, pcTaskName);

			/* Pad the end of the string with spaces to ensure columns line up when
			 * printed out. */
			for (x = strlen(pcBuffer); x < (size_t)( configMAX_TASK_NAME_LEN - 1);
					x++) {
				pcBuffer[x] = ' ';
			}

			/* Terminate. */
			pcBuffer[x] = (char) 0x00;

			/* Return the new end of string. */
			return &(pcBuffer[x]);
		}

#endif /* ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) */
		/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

		void vTaskList(char * pcWriteBuffer) {
			TaskStatus_t * pxTaskStatusArray;
			UBaseType_t uxArraySize, x;
			char cStatus;

			/*
			 * PLEASE NOTE:
			 *
			 * This function is provided for convenience only, and is used by many
			 * of the demo applications.  Do not consider it to be part of the
			 * scheduler.
			 *
			 * vTaskList() calls uxTaskGetSystemState(), then formats part of the
			 * uxTaskGetSystemState() output into a human readable table that
			 * displays task names, states and stack usage.
			 *
			 * vTaskList() has a dependency on the sprintf() C library function that
			 * might bloat the code size, use a lot of stack, and provide different
			 * results on different platforms.  An alternative, tiny, third party,
			 * and limited functionality implementation of sprintf() is provided in
			 * many of the FreeRTOS/Demo sub-directories in a file called
			 * printf-stdarg.c (note printf-stdarg.c does not provide a full
			 * snprintf() implementation!).
			 *
			 * It is recommended that production systems call uxTaskGetSystemState()
			 * directly to get access to raw stats data, rather than indirectly
			 * through a call to vTaskList().
			 */

			/* Make sure the write buffer does not contain a string. */
			*pcWriteBuffer = (char) 0x00;

			/* Take a snapshot of the number of tasks in case it changes while this
			 * function is executing. */
			uxArraySize = uxCurrentNumberOfTasks;

			/* Allocate an array index for each task.  NOTE!  if
			 * configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
			 * equate to NULL. */
			pxTaskStatusArray = pvPortMalloc(
					uxCurrentNumberOfTasks * sizeof(TaskStatus_t)); /*lint !e9079 All values returned by pvPortMalloc() have at least the alignment required by the MCU's stack and this allocation allocates a struct that has the alignment requirements of a pointer. */

			if (pxTaskStatusArray != NULL) {
				/* Generate the (binary) data. */
				uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize,
						NULL);

				/* Create a human readable table from the binary data. */
				for (x = 0; x < uxArraySize; x++) {
					switch (pxTaskStatusArray[x].eCurrentState) {
					case eRunning:
						cStatus = tskRUNNING_CHAR;
						break;

					case eReady:
						cStatus = tskREADY_CHAR;
						break;

					case eBlocked:
						cStatus = tskBLOCKED_CHAR;
						break;

					case eSuspended:
						cStatus = tskSUSPENDED_CHAR;
						break;

					case eDeleted:
						cStatus = tskDELETED_CHAR;
						break;

					case eInvalid: /* Fall through. */
					default: /* Should not get here, but it is included
					 * to prevent static checking errors. */
						cStatus = (char) 0x00;
						break;
					}

					/* Write the task name to the string, padding with spaces so it
					 * can be printed in tabular form more easily. */
					pcWriteBuffer = prvWriteNameToBuffer(pcWriteBuffer,
							pxTaskStatusArray[x].pcTaskName);

					/* Write the rest of the string. */
					sprintf(pcWriteBuffer, "\t%c\t%u\t%u\t%u\r\n", cStatus,
							(unsigned int) pxTaskStatusArray[x].uxCurrentPriority,
							(unsigned int) pxTaskStatusArray[x].usStackHighWaterMark,
							(unsigned int) pxTaskStatusArray[x].xTaskNumber); /*lint !e586 sprintf() allowed as this is compiled with many compilers and this is a utility function only - not part of the core kernel implementation. */
					pcWriteBuffer += strlen(pcWriteBuffer); /*lint !e9016 Pointer arithmetic ok on char pointers especially as in this case where it best denotes the intent of the code. */
				}

				/* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
				 * is 0 then vPortFree() will be #defined to nothing. */
				vPortFree(pxTaskStatusArray);
			} else {
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) */
		/*----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )

		void vTaskGetRunTimeStats( char * pcWriteBuffer )
		{
			TaskStatus_t * pxTaskStatusArray;
			UBaseType_t uxArraySize, x;
			uint32_t ulTotalTime, ulStatsAsPercentage;

#if ( configUSE_TRACE_FACILITY != 1 )
			{
#error configUSE_TRACE_FACILITY must also be set to 1 in FreeRTOSConfig.h to use vTaskGetRunTimeStats().
			}
#endif

			/*
			 * PLEASE NOTE:
			 *
			 * This function is provided for convenience only, and is used by many
			 * of the demo applications.  Do not consider it to be part of the
			 * scheduler.
			 *
			 * vTaskGetRunTimeStats() calls uxTaskGetSystemState(), then formats part
			 * of the uxTaskGetSystemState() output into a human readable table that
			 * displays the amount of time each task has spent in the Running state
			 * in both absolute and percentage terms.
			 *
			 * vTaskGetRunTimeStats() has a dependency on the sprintf() C library
			 * function that might bloat the code size, use a lot of stack, and
			 * provide different results on different platforms.  An alternative,
			 * tiny, third party, and limited functionality implementation of
			 * sprintf() is provided in many of the FreeRTOS/Demo sub-directories in
			 * a file called printf-stdarg.c (note printf-stdarg.c does not provide
			 * a full snprintf() implementation!).
			 *
			 * It is recommended that production systems call uxTaskGetSystemState()
			 * directly to get access to raw stats data, rather than indirectly
			 * through a call to vTaskGetRunTimeStats().
			 */

			/* Make sure the write buffer does not contain a string. */
			*pcWriteBuffer = ( char ) 0x00;

			/* Take a snapshot of the number of tasks in case it changes while this
			 * function is executing. */
			uxArraySize = uxCurrentNumberOfTasks;

			/* Allocate an array index for each task.  NOTE!  If
			 * configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
			 * equate to NULL. */
			pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks * sizeof( TaskStatus_t ) ); /*lint !e9079 All values returned by pvPortMalloc() have at least the alignment required by the MCU's stack and this allocation allocates a struct that has the alignment requirements of a pointer. */

			if( pxTaskStatusArray != NULL )
			{
				/* Generate the (binary) data. */
				uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

				/* For percentage calculations. */
				ulTotalTime /= 100UL;

				/* Avoid divide by zero errors. */
				if( ulTotalTime > 0UL )
				{
					/* Create a human readable table from the binary data. */
					for( x = 0; x < uxArraySize; x++ )
					{
						/* What percentage of the total run time has the task used?
						 * This will always be rounded down to the nearest integer.
						 * ulTotalRunTimeDiv100 has already been divided by 100. */
						ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalTime;

						/* Write the task name to the string, padding with
						 * spaces so it can be printed in tabular form more
						 * easily. */
						pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );

						if( ulStatsAsPercentage > 0UL )
						{
#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
							{
								sprintf( pcWriteBuffer, "\t%lu\t\t%lu%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
							}
#else
							{
								/* sizeof( int ) == sizeof( long ) so a smaller
								 * printf() library can be used. */
								sprintf( pcWriteBuffer, "\t%u\t\t%u%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter, ( unsigned int ) ulStatsAsPercentage ); /*lint !e586 sprintf() allowed as this is compiled with many compilers and this is a utility function only - not part of the core kernel implementation. */
							}
#endif
						}
						else
						{
							/* If the percentage is zero here then the task has
							 * consumed less than 1% of the total run time. */
#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
							{
								sprintf( pcWriteBuffer, "\t%lu\t\t<1%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter );
							}
#else
							{
								/* sizeof( int ) == sizeof( long ) so a smaller
								 * printf() library can be used. */
								sprintf( pcWriteBuffer, "\t%u\t\t<1%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter ); /*lint !e586 sprintf() allowed as this is compiled with many compilers and this is a utility function only - not part of the core kernel implementation. */
							}
#endif
						}

						pcWriteBuffer += strlen( pcWriteBuffer ); /*lint !e9016 Pointer arithmetic ok on char pointers especially as in this case where it best denotes the intent of the code. */
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				/* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
				 * is 0 then vPortFree() will be #defined to nothing. */
				vPortFree( pxTaskStatusArray );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

#endif /* ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */
		/*-----------------------------------------------------------*/

		TickType_t uxTaskResetEventItemValue(void) {
			TickType_t uxReturn;

			uxReturn = listGET_LIST_ITEM_VALUE(&(pxCurrentTCB->xEventListItem));

			/* Reset the event list item to its normal value - so it can be used with
			 * queues and semaphores. */
			listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xEventListItem),
					( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB->uxPriority )); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

			return uxReturn;
		}
		/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

		TaskHandle_t pvTaskIncrementMutexHeldCount(void) {
			/* If xSemaphoreCreateMutex() is called before any tasks have been created
			 * then pxCurrentTCB will be NULL. */
			if (pxCurrentTCB != NULL) {
				(pxCurrentTCB->uxMutexesHeld)++;
			}

			return pxCurrentTCB;
		}

#endif /* configUSE_MUTEXES */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		uint32_t ulTaskGenericNotifyTake(UBaseType_t uxIndexToWait,
				BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
			uint32_t ulReturn;

			configASSERT(uxIndexToWait < configTASK_NOTIFICATION_ARRAY_ENTRIES);

			taskENTER_CRITICAL()
			;
			{
				/* Only block if the notification count is not already non-zero. */
				if (pxCurrentTCB->ulNotifiedValue[uxIndexToWait] == 0UL) {
					/* Mark this task as waiting for a notification. */
					pxCurrentTCB->ucNotifyState[uxIndexToWait] =
							taskWAITING_NOTIFICATION;

					if (xTicksToWait > (TickType_t) 0) {
						prvAddCurrentTaskToDelayedList(xTicksToWait, pdTRUE);
						traceTASK_NOTIFY_TAKE_BLOCK( uxIndexToWait );

						/* All ports are written to allow a yield in a critical
						 * section (some will yield immediately, others wait until the
						 * critical section exits) - but it is not something that
						 * application code should ever do. */
						portYIELD_WITHIN_API()
						;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL()
			;

			taskENTER_CRITICAL()
			;
			{
				traceTASK_NOTIFY_TAKE( uxIndexToWait );
				ulReturn = pxCurrentTCB->ulNotifiedValue[uxIndexToWait];

				if (ulReturn != 0UL) {
					if (xClearCountOnExit != pdFALSE) {
						pxCurrentTCB->ulNotifiedValue[uxIndexToWait] = 0UL;
					} else {
						pxCurrentTCB->ulNotifiedValue[uxIndexToWait] = ulReturn
								- (uint32_t) 1;
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}

				pxCurrentTCB->ucNotifyState[uxIndexToWait] =
						taskNOT_WAITING_NOTIFICATION;
			}
			taskEXIT_CRITICAL()
			;

			return ulReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		BaseType_t xTaskGenericNotifyWait(UBaseType_t uxIndexToWait,
				uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
				uint32_t * pulNotificationValue, TickType_t xTicksToWait) {
			BaseType_t xReturn;

			configASSERT(uxIndexToWait < configTASK_NOTIFICATION_ARRAY_ENTRIES);

			taskENTER_CRITICAL()
			;
			{
				/* Only block if a notification is not already pending. */
				if (pxCurrentTCB->ucNotifyState[uxIndexToWait]
												!= taskNOTIFICATION_RECEIVED) {
					/* Clear bits in the task's notification value as bits may get
					 * set  by the notifying task or interrupt.  This can be used to
					 * clear the value to zero. */
					pxCurrentTCB->ulNotifiedValue[uxIndexToWait] &=
							~ulBitsToClearOnEntry;

					/* Mark this task as waiting for a notification. */
					pxCurrentTCB->ucNotifyState[uxIndexToWait] =
							taskWAITING_NOTIFICATION;

					if (xTicksToWait > (TickType_t) 0) {
						prvAddCurrentTaskToDelayedList(xTicksToWait, pdTRUE);
						traceTASK_NOTIFY_WAIT_BLOCK( uxIndexToWait );

						/* All ports are written to allow a yield in a critical
						 * section (some will yield immediately, others wait until the
						 * critical section exits) - but it is not something that
						 * application code should ever do. */
						portYIELD_WITHIN_API()
						;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL()
			;

			taskENTER_CRITICAL()
			;
			{
				traceTASK_NOTIFY_WAIT( uxIndexToWait );

				if (pulNotificationValue != NULL) {
					/* Output the current notification value, which may or may not
					 * have changed. */
					*pulNotificationValue =
							pxCurrentTCB->ulNotifiedValue[uxIndexToWait];
				}

				/* If ucNotifyValue is set then either the task never entered the
				 * blocked state (because a notification was already pending) or the
				 * task unblocked because of a notification.  Otherwise the task
				 * unblocked because of a timeout. */
				if (pxCurrentTCB->ucNotifyState[uxIndexToWait]
												!= taskNOTIFICATION_RECEIVED) {
					/* A notification was not received. */
					xReturn = pdFALSE;
				} else {
					/* A notification was already pending or a notification was
					 * received while the task was waiting. */
					pxCurrentTCB->ulNotifiedValue[uxIndexToWait] &=
							~ulBitsToClearOnExit;
					xReturn = pdTRUE;
				}

				pxCurrentTCB->ucNotifyState[uxIndexToWait] =
						taskNOT_WAITING_NOTIFICATION;
			}
			taskEXIT_CRITICAL()
			;

			return xReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		BaseType_t xTaskGenericNotify(TaskHandle_t xTaskToNotify,
				UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction,
				uint32_t * pulPreviousNotificationValue) {
			TCB_t * pxTCB;
			BaseType_t xReturn = pdPASS;
			uint8_t ucOriginalNotifyState;

			configASSERT(uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES);
			configASSERT(xTaskToNotify);
			pxTCB = xTaskToNotify;

			taskENTER_CRITICAL()
			;
			{
				if (pulPreviousNotificationValue != NULL) {
					*pulPreviousNotificationValue =
							pxTCB->ulNotifiedValue[uxIndexToNotify];
				}

				ucOriginalNotifyState = pxTCB->ucNotifyState[uxIndexToNotify];

				pxTCB->ucNotifyState[uxIndexToNotify] = taskNOTIFICATION_RECEIVED;

				switch (eAction) {
				case eSetBits:
					pxTCB->ulNotifiedValue[uxIndexToNotify] |= ulValue;
					break;

				case eIncrement:
					(pxTCB->ulNotifiedValue[uxIndexToNotify])++;
					break;

				case eSetValueWithOverwrite:
					pxTCB->ulNotifiedValue[uxIndexToNotify] = ulValue;
					break;

				case eSetValueWithoutOverwrite:

					if (ucOriginalNotifyState != taskNOTIFICATION_RECEIVED) {
						pxTCB->ulNotifiedValue[uxIndexToNotify] = ulValue;
					} else {
						/* The value could not be written to the task. */
						xReturn = pdFAIL;
					}

					break;

				case eNoAction:

					/* The task is being notified without its notify value being
					 * updated. */
					break;

				default:

					/* Should not get here if all enums are handled.
					 * Artificially force an assert by testing a value the
					 * compiler can't assume is const. */
					configASSERT(xTickCount == (TickType_t ) 0);

					break;
				}

				traceTASK_NOTIFY( uxIndexToNotify );

				/* If the task is in the blocked state specifically to wait for a
				 * notification then unblock it now. */
				if (ucOriginalNotifyState == taskWAITING_NOTIFICATION) {
					(void) uxListRemove(&(pxTCB->xStateListItem));
					prvAddTaskToReadyList(pxTCB);

					/* The task should not have been on an event list. */
					configASSERT(
							listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL);

#if ( configUSE_TICKLESS_IDLE != 0 )
					{
						/* If a task is blocked waiting for a notification then
						 * xNextTaskUnblockTime might be set to the blocked task's time
						 * out time.  If the task is unblocked for a reason other than
						 * a timeout xNextTaskUnblockTime is normally left unchanged,
						 * because it will automatically get reset to a new value when
						 * the tick count equals xNextTaskUnblockTime.  However if
						 * tickless idling is used it might be more important to enter
						 * sleep mode at the earliest possible time - so reset
						 * xNextTaskUnblockTime here to ensure it is updated at the
						 * earliest possible time. */
						prvResetNextTaskUnblockTime();
					}
#endif

					if (pxTCB->uxPriority > pxCurrentTCB->uxPriority) {
						/* The notified task has a priority above the currently
						 * executing task so a yield is required. */
						taskYIELD_IF_USING_PREEMPTION()
																						;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				} else {
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL()
			;

			return xReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		BaseType_t xTaskGenericNotifyFromISR(TaskHandle_t xTaskToNotify,
				UBaseType_t uxIndexToNotify, uint32_t ulValue, eNotifyAction eAction,
				uint32_t * pulPreviousNotificationValue,
				BaseType_t * pxHigherPriorityTaskWoken) {
			TCB_t * pxTCB;
			uint8_t ucOriginalNotifyState;
			BaseType_t xReturn = pdPASS;
			UBaseType_t uxSavedInterruptStatus;

			configASSERT(xTaskToNotify);
			configASSERT(uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES);

			/* RTOS ports that support interrupt nesting have the concept of a
			 * maximum  system call (or maximum API call) interrupt priority.
			 * Interrupts that are  above the maximum system call priority are keep
			 * permanently enabled, even when the RTOS kernel is in a critical section,
			 * but cannot make any calls to FreeRTOS API functions.  If configASSERT()
			 * is defined in FreeRTOSConfig.h then
			 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
			 * failure if a FreeRTOS API function is called from an interrupt that has
			 * been assigned a priority above the configured maximum system call
			 * priority.  Only FreeRTOS functions that end in FromISR can be called
			 * from interrupts  that have been assigned a priority at or (logically)
			 * below the maximum system call interrupt priority.  FreeRTOS maintains a
			 * separate interrupt safe API to ensure interrupt entry is as fast and as
			 * simple as possible.  More information (albeit Cortex-M specific) is
			 * provided on the following link:
			 * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
			portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

			pxTCB = xTaskToNotify;

			uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
			{
				if (pulPreviousNotificationValue != NULL) {
					*pulPreviousNotificationValue =
							pxTCB->ulNotifiedValue[uxIndexToNotify];
				}

				ucOriginalNotifyState = pxTCB->ucNotifyState[uxIndexToNotify];
				pxTCB->ucNotifyState[uxIndexToNotify] = taskNOTIFICATION_RECEIVED;

				switch (eAction) {
				case eSetBits:
					pxTCB->ulNotifiedValue[uxIndexToNotify] |= ulValue;
					break;

				case eIncrement:
					(pxTCB->ulNotifiedValue[uxIndexToNotify])++;
					break;

				case eSetValueWithOverwrite:
					pxTCB->ulNotifiedValue[uxIndexToNotify] = ulValue;
					break;

				case eSetValueWithoutOverwrite:

					if (ucOriginalNotifyState != taskNOTIFICATION_RECEIVED) {
						pxTCB->ulNotifiedValue[uxIndexToNotify] = ulValue;
					} else {
						/* The value could not be written to the task. */
						xReturn = pdFAIL;
					}

					break;

				case eNoAction:

					/* The task is being notified without its notify value being
					 * updated. */
					break;

				default:

					/* Should not get here if all enums are handled.
					 * Artificially force an assert by testing a value the
					 * compiler can't assume is const. */
					configASSERT(xTickCount == (TickType_t ) 0);
					break;
				}

				traceTASK_NOTIFY_FROM_ISR( uxIndexToNotify );

				/* If the task is in the blocked state specifically to wait for a
				 * notification then unblock it now. */
				if (ucOriginalNotifyState == taskWAITING_NOTIFICATION) {
					/* The task should not have been on an event list. */
					configASSERT(
							listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL);

					if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
						(void) uxListRemove(&(pxTCB->xStateListItem));
						prvAddTaskToReadyList(pxTCB);
					} else {
						/* The delayed and ready lists cannot be accessed, so hold
						 * this task pending until the scheduler is resumed. */
						vListInsertEnd(&(xPendingReadyList), &(pxTCB->xEventListItem));
					}

					if (pxTCB->uxPriority > pxCurrentTCB->uxPriority) {
						/* The notified task has a priority above the currently
						 * executing task so a yield is required. */
						if (pxHigherPriorityTaskWoken != NULL) {
							*pxHigherPriorityTaskWoken = pdTRUE;
						}

						/* Mark that a yield is pending in case the user is not
						 * using the "xHigherPriorityTaskWoken" parameter to an ISR
						 * safe FreeRTOS function. */
						xYieldPending = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);

			return xReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		void vTaskGenericNotifyGiveFromISR(TaskHandle_t xTaskToNotify,
				UBaseType_t uxIndexToNotify, BaseType_t * pxHigherPriorityTaskWoken) {
			TCB_t * pxTCB;
			uint8_t ucOriginalNotifyState;
			UBaseType_t uxSavedInterruptStatus;

			configASSERT(xTaskToNotify);
			configASSERT(uxIndexToNotify < configTASK_NOTIFICATION_ARRAY_ENTRIES);

			/* RTOS ports that support interrupt nesting have the concept of a
			 * maximum  system call (or maximum API call) interrupt priority.
			 * Interrupts that are  above the maximum system call priority are keep
			 * permanently enabled, even when the RTOS kernel is in a critical section,
			 * but cannot make any calls to FreeRTOS API functions.  If configASSERT()
			 * is defined in FreeRTOSConfig.h then
			 * portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
			 * failure if a FreeRTOS API function is called from an interrupt that has
			 * been assigned a priority above the configured maximum system call
			 * priority.  Only FreeRTOS functions that end in FromISR can be called
			 * from interrupts  that have been assigned a priority at or (logically)
			 * below the maximum system call interrupt priority.  FreeRTOS maintains a
			 * separate interrupt safe API to ensure interrupt entry is as fast and as
			 * simple as possible.  More information (albeit Cortex-M specific) is
			 * provided on the following link:
			 * https://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
			portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

			pxTCB = xTaskToNotify;

			uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
			{
				ucOriginalNotifyState = pxTCB->ucNotifyState[uxIndexToNotify];
				pxTCB->ucNotifyState[uxIndexToNotify] = taskNOTIFICATION_RECEIVED;

				/* 'Giving' is equivalent to incrementing a count in a counting
				 * semaphore. */
				(pxTCB->ulNotifiedValue[uxIndexToNotify])++;

				traceTASK_NOTIFY_GIVE_FROM_ISR( uxIndexToNotify );

				/* If the task is in the blocked state specifically to wait for a
				 * notification then unblock it now. */
				if (ucOriginalNotifyState == taskWAITING_NOTIFICATION) {
					/* The task should not have been on an event list. */
					configASSERT(
							listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL);

					if (uxSchedulerSuspended == (UBaseType_t) pdFALSE) {
						(void) uxListRemove(&(pxTCB->xStateListItem));
						prvAddTaskToReadyList(pxTCB);
					} else {
						/* The delayed and ready lists cannot be accessed, so hold
						 * this task pending until the scheduler is resumed. */
						vListInsertEnd(&(xPendingReadyList), &(pxTCB->xEventListItem));
					}

					if (pxTCB->uxPriority > pxCurrentTCB->uxPriority) {
						/* The notified task has a priority above the currently
						 * executing task so a yield is required. */
						if (pxHigherPriorityTaskWoken != NULL) {
							*pxHigherPriorityTaskWoken = pdTRUE;
						}

						/* Mark that a yield is pending in case the user is not
						 * using the "xHigherPriorityTaskWoken" parameter in an ISR
						 * safe FreeRTOS function. */
						xYieldPending = pdTRUE;
					} else {
						mtCOVERAGE_TEST_MARKER();
					}
				}
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		BaseType_t xTaskGenericNotifyStateClear(TaskHandle_t xTask,
				UBaseType_t uxIndexToClear) {
			TCB_t * pxTCB;
			BaseType_t xReturn;

			configASSERT(uxIndexToClear < configTASK_NOTIFICATION_ARRAY_ENTRIES);

			/* If null is passed in here then it is the calling task that is having
			 * its notification state cleared. */
			pxTCB = prvGetTCBFromHandle(xTask);

			taskENTER_CRITICAL()
			;
			{
				if (pxTCB->ucNotifyState[uxIndexToClear] == taskNOTIFICATION_RECEIVED) {
					pxTCB->ucNotifyState[uxIndexToClear] = taskNOT_WAITING_NOTIFICATION;
					xReturn = pdPASS;
				} else {
					xReturn = pdFAIL;
				}
			}
			taskEXIT_CRITICAL()
			;

			return xReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( configUSE_TASK_NOTIFICATIONS == 1 )

		uint32_t ulTaskGenericNotifyValueClear(TaskHandle_t xTask,
				UBaseType_t uxIndexToClear, uint32_t ulBitsToClear) {
			TCB_t * pxTCB;
			uint32_t ulReturn;

			/* If null is passed in here then it is the calling task that is having
			 * its notification state cleared. */
			pxTCB = prvGetTCBFromHandle(xTask);

			taskENTER_CRITICAL()
			;
			{
				/* Return the notification as it was before the bits were cleared,
				 * then clear the bit mask. */
				ulReturn = pxTCB->ulNotifiedValue[uxIndexToClear];
				pxTCB->ulNotifiedValue[uxIndexToClear] &= ~ulBitsToClear;
			}
			taskEXIT_CRITICAL()
			;

			return ulReturn;
		}

#endif /* configUSE_TASK_NOTIFICATIONS */
		/*-----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( INCLUDE_xTaskGetIdleTaskHandle == 1 ) )

		uint32_t ulTaskGetIdleRunTimeCounter( void )
		{
			return xIdleTaskHandle->ulRunTimeCounter;
		}

#endif
		/*-----------------------------------------------------------*/

		static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait,
				const BaseType_t xCanBlockIndefinitely) {
			TickType_t xTimeToWake;
			const TickType_t xConstTickCount = xTickCount;

#if ( INCLUDE_xTaskAbortDelay == 1 )
			{
				/* About to enter a delayed list, so ensure the ucDelayAborted flag is
				 * reset to pdFALSE so it can be detected as having been set to pdTRUE
				 * when the task leaves the Blocked state. */
				pxCurrentTCB->ucDelayAborted = pdFALSE;
			}
#endif

			/* Remove the task from the ready list before adding it to the blocked list
			 * as the same list item is used for both lists. */
			if (uxListRemove(&(pxCurrentTCB->xStateListItem)) == (UBaseType_t) 0) {
				/* The current task must be in a ready list, so there is no need to
				 * check, and the port reset macro can be called directly. */
				portRESET_READY_PRIORITY(pxCurrentTCB->uxPriority, uxTopReadyPriority); /*lint !e931 pxCurrentTCB cannot change as it is the calling task.  pxCurrentTCB->uxPriority and uxTopReadyPriority cannot change as called with scheduler suspended or in a critical section. */
			} else {
				mtCOVERAGE_TEST_MARKER();
			}

#if ( INCLUDE_vTaskSuspend == 1 )
			{
				if ((xTicksToWait == portMAX_DELAY )
						&& (xCanBlockIndefinitely != pdFALSE)) {
					/* Add the task to the suspended task list instead of a delayed task
					 * list to ensure it is not woken by a timing event.  It will block
					 * indefinitely. */
					vListInsertEnd(&xSuspendedTaskList,
							&(pxCurrentTCB->xStateListItem));
				} else {
					/* Calculate the time at which the task should be woken if the event
					 * does not occur.  This may overflow but this doesn't matter, the
					 * kernel will manage it correctly. */
					xTimeToWake = xConstTickCount + xTicksToWait;

					/* The list item will be inserted in wake time order. */
					listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem),
							xTimeToWake);

					if (xTimeToWake < xConstTickCount) {
						/* Wake time has overflowed.  Place this item in the overflow
						 * list. */
						vListInsert(pxOverflowDelayedTaskList,
								&(pxCurrentTCB->xStateListItem));
					} else {
						/* The wake time has not overflowed, so the current block list
						 * is used. */
						vListInsert(pxDelayedTaskList, &(pxCurrentTCB->xStateListItem));

						/* If the task entering the blocked state was placed at the
						 * head of the list of blocked tasks then xNextTaskUnblockTime
						 * needs to be updated too. */
						if (xTimeToWake < xNextTaskUnblockTime) {
							xNextTaskUnblockTime = xTimeToWake;
						} else {
							mtCOVERAGE_TEST_MARKER();
						}
					}
				}
			}
#else /* INCLUDE_vTaskSuspend */
			{
				/* Calculate the time at which the task should be woken if the event
				 * does not occur.  This may overflow but this doesn't matter, the kernel
				 * will manage it correctly. */
				xTimeToWake = xConstTickCount + xTicksToWait;

				/* The list item will be inserted in wake time order. */
				listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );

				if( xTimeToWake < xConstTickCount )
				{
					/* Wake time has overflowed.  Place this item in the overflow list. */
					vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
				}
				else
				{
					/* The wake time has not overflowed, so the current block list is used. */
					vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );

					/* If the task entering the blocked state was placed at the head of the
					 * list of blocked tasks then xNextTaskUnblockTime needs to be updated
					 * too. */
					if( xTimeToWake < xNextTaskUnblockTime )
					{
						xNextTaskUnblockTime = xTimeToWake;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				/* Avoid compiler warning when INCLUDE_vTaskSuspend is not 1. */
				( void ) xCanBlockIndefinitely;
			}
#endif /* INCLUDE_vTaskSuspend */
		}

		/* Code below here allows additional code to be inserted into this source file,
		 * especially where access to file scope functions and data is needed (for example
		 * when performing module tests). */

#ifdef FREERTOS_MODULE_TEST
#include "tasks_test_access_functions.h"
#endif

#if ( configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1 )

#include "freertos_tasks_c_additions.h"

#ifdef FREERTOS_TASKS_C_ADDITIONS_INIT
		static void freertos_tasks_c_additions_init( void )
		{
			FREERTOS_TASKS_C_ADDITIONS_INIT();
		}
#endif

#endif /* if ( configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H == 1 ) */
