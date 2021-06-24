#include "scheduler.h"


#define schedTHREAD_LOCAL_STORAGE_POINTER_INDEX 0

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
	#define schedUSE_TCB_SORTED_LIST 1
#else
	#define schedUSE_TCB_ARRAY 1
#endif /* schedSCHEDULING_POLICY_EDF */

/* Extended Task control block for managing periodic tasks within this library. */
typedef struct xExtended_TCB
{
	TaskFunction_t pvTaskCode; 		/* Function pointer to the code that will be run periodically. */
	const char *pcName; 			/* Name of the task. */
	UBaseType_t uxStackDepth; 		/* Stack size of the task. */
	void *pvParameters; 			/* Parameters to the task function. */
	UBaseType_t uxPriority; 		/* Priority of the task. */
	TaskHandle_t *pxTaskHandle;		/* Task handle for the task. */
	TickType_t xReleaseTime;		/* Release time of the task. */
	TickType_t xRelativeDeadline;	/* Relative deadline of the task. */
	TickType_t xAbsoluteDeadline;	/* Absolute deadline of the task. */
	TickType_t xPeriod;				/* Task period. */
	TickType_t xLastWakeTime; 		/* Last time stamp when the task was running. */
	TickType_t xMaxExecTime;		/* Worst-case execution time of the task. */
	TickType_t xExecTime;			/* Current execution time of the task. */
	BaseType_t xWorkIsDone; 		/* pdFALSE if the job is not finished, pdTRUE if the job is finished. */

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xPriorityIsSet; 	/* pdTRUE if the priority is assigned. */
		BaseType_t xInUse; 			/* pdFALSE if this extended TCB is empty. */
	#elif( schedUSE_TCB_SORTED_LIST == 1 )
		ListItem_t xTCBListItem; 	/* Used to reference TCB from the TCB list. */
	#endif /* schedUSE_TCB_SORTED_LIST */

} SchedTCB_t;


#if( schedUSE_TCB_ARRAY == 1 )
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle );
	static void prvInitTCBArray( void );
	/* Find index for an empty entry in xTCBArray. Return -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void );
	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex );
#elif( schedUSE_TCB_SORTED_LIST == 1 )
	static void prvAddTCBToList( SchedTCB_t *pxTCB );
	static void prvDeleteTCBFromList(  SchedTCB_t *pxTCB );
#endif /* schedUSE_TCB_ARRAY */

static TickType_t xSystemStartTime = 0;

static void prvPeriodicTaskCode( void *pvParameters );
static void prvCreateAllTasks( void );


#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
	static void prvSetFixedPriorities( void );
#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
	static void prvInitEDF( void );
	#if( schedEDF_NAIVE == 1 )
		static void prvUpdatePrioritiesEDF( void );
	#endif /* schedEDF_NAIVE */
	#if( schedUSE_TCB_SORTED_LIST == 1 )
		static void prvSwapList( List_t **ppxList1, List_t **ppxList2 );
	#endif /* schedUSE_TCB_SORTED_LIST */
#endif /* schedSCHEDULING_POLICY_EDF */

#if( schedUSE_TCB_ARRAY == 1 )
	/* Array for extended TCBs. */
	static SchedTCB_t xTCBArray[ schedMAX_NUMBER_OF_PERIODIC_TASKS ] = { 0 };
	/* Counter for number of periodic tasks. */
	static BaseType_t xTaskCounter = 0;
#elif( schedUSE_TCB_SORTED_LIST == 1 )
	#if( schedEDF_NAIVE == 1 )
		static List_t xTCBList;				/* Sorted linked list for all periodic tasks. */
		static List_t xTCBTempList;			/* A temporary list used for switching lists. */
		static List_t xTCBOverflowedList; 	/* Sorted linked list for periodic tasks that have overflowed deadline. */
		static List_t *pxTCBList = NULL;  			/* Pointer to xTCBList. */
		static List_t *pxTCBTempList = NULL;		/* Pointer to xTCBTempList. */
		static List_t *pxTCBOverflowedList = NULL;	/* Pointer to xTCBOverflowedList. */
	#endif /* schedEDF_NAIVE */
#endif /* schedUSE_TCB_ARRAY */

#if( schedUSE_TCB_ARRAY == 1 )
	/* Returns index position in xTCBArray of TCB with same task handle as parameter. */
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle )
	{
	static BaseType_t xIndex = 0;
	BaseType_t xIterator;

		for( xIterator = 0; xIterator < schedMAX_NUMBER_OF_PERIODIC_TASKS; xIterator++ )
		{
		
			if( pdTRUE == xTCBArray[ xIndex ].xInUse && *xTCBArray[ xIndex ].pxTaskHandle == xTaskHandle )
			{
				return xIndex;
			}
		
			xIndex++;
			if( schedMAX_NUMBER_OF_PERIODIC_TASKS == xIndex )
			{
				xIndex = 0;
			}
		}
		return -1;
	}

	/* Initializes xTCBArray. */
	static void prvInitTCBArray( void )
	{
	UBaseType_t uxIndex;
		for( uxIndex = 0; uxIndex < schedMAX_NUMBER_OF_PERIODIC_TASKS; uxIndex++)
		{
			xTCBArray[ uxIndex ].xInUse = pdFALSE;
		}
	}

	/* Find index for an empty entry in xTCBArray. Returns -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void )
	{
	BaseType_t xIndex;
		for( xIndex = 0; xIndex < schedMAX_NUMBER_OF_PERIODIC_TASKS; xIndex++ )
		{
			if( pdFALSE == xTCBArray[ xIndex ].xInUse )
			{
				return xIndex;
			}
		}

		return -1;
	}

	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex )
	{
		configASSERT( xIndex >= 0 && xIndex < schedMAX_NUMBER_OF_PERIODIC_TASKS );
		configASSERT( pdTRUE == xTCBArray[ xIndex ].xInUse );

		if( xTCBArray[ pdTRUE == xIndex].xInUse )
		{
			xTCBArray[ xIndex ].xInUse = pdFALSE;
			xTaskCounter--;
		}
	}
	
#elif( schedUSE_TCB_SORTED_LIST == 1 )
	/* Add an extended TCB to sorted linked list. */
	static void prvAddTCBToList( SchedTCB_t *pxTCB )
	{
		/* Initialise TCB list item. */
		vListInitialiseItem( &pxTCB->xTCBListItem );
		/* Set owner of list item to the TCB. */
		listSET_LIST_ITEM_OWNER( &pxTCB->xTCBListItem, pxTCB );
		/* List is sorted by absolute deadline value. */
		listSET_LIST_ITEM_VALUE( &pxTCB->xTCBListItem, pxTCB->xAbsoluteDeadline );
		
		#if( schedEDF_EFFICIENT == 1 )
		 	/* Initialise list item for xTCBListAll. */
			vListInitialiseItem( &pxTCB->xTCBAllListItem );
			/* Set owner of list item to the TCB. */
			listSET_LIST_ITEM_OWNER( &pxTCB->xTCBAllListItem, pxTCB );
			/* There is no need to sort the list. */
		#endif /* schedEDF_EFFICIENT */

		#if( schedEDF_NAIVE == 1 )
			/* Insert TCB into list. */
			vListInsert( pxTCBList, &pxTCB->xTCBListItem );
		#elif( schedEDF_EFFICIENT == 1 )
			/* Insert TCB into ready list. */
			vListInsert( pxTCBReadyList, &pxTCB->xTCBListItem );
			/* Insert TCB into list containing tasks in any state. */
			vListInsert( pxTCBListAll, &pxTCB->xTCBAllListItem );
		#endif /* schedEDF_EFFICIENT */
	}
	
	/* Delete an extended TCB from sorted linked list. */
	static void prvDeleteTCBFromList(  SchedTCB_t *pxTCB )
	{
		#if( schedEDF_EFFICIENT == 1 )
			uxListRemove( &pxTCB->xTCBAllListItem );
		#endif /* schedEDF_EFFICIENT */
		uxListRemove( &pxTCB->xTCBListItem );
		vPortFree( pxTCB );
	}
#endif /* schedUSE_TCB_ARRAY */

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
	#if( schedUSE_TCB_SORTED_LIST == 1 )
		/* Swap content of two lists. */
		static void prvSwapList( List_t **ppxList1, List_t **ppxList2 )
		{
		List_t *pxTemp;
			pxTemp = *ppxList1;
			*ppxList1 = *ppxList2;
			*ppxList2 = pxTemp;
		}
	#endif /* schedUSE_TCB_SORTED_LIST */

	#if( schedEDF_NAIVE == 1 )
		/* Update priorities of all periodic tasks with respect to EDF policy. */
		static void prvUpdatePrioritiesEDF( void )
		{
		SchedTCB_t *pxTCB;

			#if( schedUSE_TCB_SORTED_LIST == 1 )
				ListItem_t *pxTCBListItem;
				ListItem_t *pxTCBListItemTemp;
			
				if( listLIST_IS_EMPTY( pxTCBList ) && !listLIST_IS_EMPTY( pxTCBOverflowedList ) )
				{
					prvSwapList( &pxTCBList, &pxTCBOverflowedList );
				}

				const ListItem_t *pxTCBListEndMarker = listGET_END_MARKER( pxTCBList );
				pxTCBListItem = listGET_HEAD_ENTRY( pxTCBList );

				while( pxTCBListItem != pxTCBListEndMarker )
				{
					pxTCB = listGET_LIST_ITEM_OWNER( pxTCBListItem );

					/* Update priority in the SchedTCB list. */
					listSET_LIST_ITEM_VALUE( pxTCBListItem, pxTCB->xAbsoluteDeadline );

					pxTCBListItemTemp = pxTCBListItem;
					pxTCBListItem = listGET_NEXT( pxTCBListItem );
					uxListRemove( pxTCBListItem->pxPrevious );

					/* If absolute deadline overflowed, insert TCB to overflowed list. */
					if( pxTCB->xAbsoluteDeadline < pxTCB->xLastWakeTime )
					{
						vListInsert( pxTCBOverflowedList, pxTCBListItemTemp );
					}
					else /* Insert TCB into temp list in usual case. */
					{
						vListInsert( pxTCBTempList, pxTCBListItemTemp );
					}
				}

				/* Swap list with temp list. */
				prvSwapList( &pxTCBList, &pxTCBTempList );

				BaseType_t xHighestPriority = configMAX_PRIORITIES - 1;

				const ListItem_t *pxTCBListEndMarkerAfterSwap = listGET_END_MARKER( pxTCBList );
				pxTCBListItem = listGET_HEAD_ENTRY( pxTCBList );
				while( pxTCBListItem != pxTCBListEndMarkerAfterSwap )
				{
					pxTCB = listGET_LIST_ITEM_OWNER( pxTCBListItem );
					configASSERT( -1 <= xHighestPriority );
					pxTCB->uxPriority = xHighestPriority;
					vTaskPrioritySet( *pxTCB->pxTaskHandle, pxTCB->uxPriority );

					xHighestPriority--;
					pxTCBListItem = listGET_NEXT( pxTCBListItem );
				}
			#endif /* schedUSE_TCB_SORTED_LIST */
		}
    #endif /* schedEDF_NAIVE */
#endif /* schedSCHEDULING_POLICY_EDF */

/* The whole function code that is executed by every periodic task.
 * This function wraps the task code specified by the user. */
static void prvPeriodicTaskCode( void *pvParameters )
{
SchedTCB_t *pxThisTask = ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( xTaskGetCurrentTaskHandle(), schedTHREAD_LOCAL_STORAGE_POINTER_INDEX );
	configASSERT( NULL != pxThisTask );

	if( 0 != pxThisTask->xReleaseTime )
	{
		vTaskDelayUntil( &pxThisTask->xLastWakeTime, pxThisTask->xReleaseTime );
	}

	if( 0 == pxThisTask->xReleaseTime )
	{
		pxThisTask->xLastWakeTime = xSystemStartTime;
	}

	for( ; ; )
	{
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
			#if( schedEDF_NAIVE == 1 )
				/* Wake up the scheduler task to update priorities of all periodic tasks. */
				prvWakeScheduler();
			#endif /* schedEDF_NAIVE */
		#endif /* schedSCHEDULING_POLICY_EDF */
		pxThisTask->xWorkIsDone = pdFALSE;

		/*
		taskENTER_CRITICAL();
		printf( "tickcount %d Task %s Abs deadline %d lastWakeTime %d prio %d Handle %x\r\n", xTaskGetTickCount(), pxThisTask->pcName, pxThisTask->xAbsoluteDeadline, pxThisTask->xLastWakeTime, uxTaskPriorityGet( NULL ), *pxThisTask->pxTaskHandle );
		taskEXIT_CRITICAL();
		*/

		/* Execute the task function specified by the user. */
		pxThisTask->pvTaskCode( pvParameters );

		pxThisTask->xWorkIsDone = pdTRUE;

		/*
		taskENTER_CRITICAL();
		printf( "execution time %d Task %s\r\n", pxThisTask->xExecTime, pxThisTask->pcName );
		taskEXIT_CRITICAL();
		*/

		pxThisTask->xExecTime = 0;

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
			pxThisTask->xAbsoluteDeadline = pxThisTask->xLastWakeTime + pxThisTask->xPeriod + pxThisTask->xRelativeDeadline;
			#if( schedEDF_NAIVE == 1 )
				/* Wake up the scheduler task to update priorities of all periodic tasks. */
				prvWakeScheduler();
			#endif /* schedEDF_NAIVE */
		#endif /* schedSCHEDULING_POLICY_EDF */

		vTaskDelayUntil( &pxThisTask->xLastWakeTime, pxThisTask->xPeriod );
	}
}

/* Creates a periodic task. */
void vSchedulerPeriodicTaskCreate( TaskFunction_t pvTaskCode, const char *pcName, UBaseType_t uxStackDepth, void *pvParameters, UBaseType_t uxPriority,
		TaskHandle_t *pxCreatedTask, TickType_t xPhaseTick, TickType_t xPeriodTick, TickType_t xMaxExecTimeTick, TickType_t xDeadlineTick )
{
	taskENTER_CRITICAL();
SchedTCB_t *pxNewTCB;
	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex = prvFindEmptyElementIndexTCB();
		configASSERT( xTaskCounter < schedMAX_NUMBER_OF_PERIODIC_TASKS );
		configASSERT( xIndex != -1 );
		pxNewTCB = &xTCBArray[ xIndex ];
	#else
		pxNewTCB = pvPortMalloc( sizeof( SchedTCB_t ) );
	#endif /* schedUSE_TCB_ARRAY */


	/* Intialize item. */
	*pxNewTCB = ( SchedTCB_t ) { .pvTaskCode = pvTaskCode, .pcName = pcName, .uxStackDepth = uxStackDepth, .pvParameters = pvParameters,
		.uxPriority = uxPriority, .pxTaskHandle = pxCreatedTask, .xReleaseTime = xPhaseTick, .xPeriod = xPeriodTick, .xMaxExecTime = xMaxExecTimeTick,
		.xRelativeDeadline = xDeadlineTick, .xWorkIsDone = pdTRUE, .xExecTime = 0 };
	#if( schedUSE_TCB_ARRAY == 1 )
		pxNewTCB->xInUse = pdTRUE;
	#endif /* schedUSE_TCB_ARRAY */
	
	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
		pxNewTCB->xPriorityIsSet = pdFALSE;
	#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_MANUAL )
		pxNewTCB->xPriorityIsSet = pdTRUE;
	#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
		pxNewTCB->xAbsoluteDeadline = pxNewTCB->xRelativeDeadline + pxNewTCB->xReleaseTime + xSystemStartTime;
		pxNewTCB->uxPriority = -1;
	#endif /* schedSCHEDULING_POLICY */

	#if( schedUSE_TCB_ARRAY == 1 )
		xTaskCounter++;
	#elif( schedUSE_TCB_SORTED_LIST == 1 )
		prvAddTCBToList( pxNewTCB );
	#endif /* schedUSE_TCB_SORTED_LIST */
	taskEXIT_CRITICAL();
}

/* Deletes a periodic task. */
void vSchedulerPeriodicTaskDelete( TaskHandle_t xTaskHandle )
{
	if( xTaskHandle != NULL )
	{
		#if( schedUSE_TCB_ARRAY == 1 )
			prvDeleteTCBFromArray( prvGetTCBIndexFromHandle( xTaskHandle ) );
		#elif( schedUSE_TCB_SORTED_LIST == 1 )
			prvDeleteTCBFromList( ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( xTaskHandle, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX ) );
		#endif /* schedUSE_TCB_ARRAY */
	}
	else
	{
		#if( schedUSE_TCB_ARRAY == 1 )
			prvDeleteTCBFromArray( prvGetTCBIndexFromHandle( xTaskGetCurrentTaskHandle() ) );
		#elif( schedUSE_TCB_SORTED_LIST == 1 )
			prvDeleteTCBFromList( ( SchedTCB_t * ) pvTaskGetThreadLocalStoragePointer( xTaskGetCurrentTaskHandle(), schedTHREAD_LOCAL_STORAGE_POINTER_INDEX ) );
		#endif /* schedUSE_TCB_ARRAY */
	}
	
	vTaskDelete( xTaskHandle );
}

/* Creates all periodic tasks stored in TCB array, or TCB list. */
static void prvCreateAllTasks( void )
{
SchedTCB_t *pxTCB;

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex;
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			configASSERT( pdTRUE == xTCBArray[ xIndex ].xInUse );
			pxTCB = &xTCBArray[ xIndex ];

			BaseType_t xReturnValue = xTaskCreate( prvPeriodicTaskCode, pxTCB->pcName, pxTCB->uxStackDepth, pxTCB->pvParameters, pxTCB->uxPriority, pxTCB->pxTaskHandle );

			if( pdPASS == xReturnValue )
			{
				vTaskSetThreadLocalStoragePointer( *pxTCB->pxTaskHandle, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX, pxTCB );
			}
			else
			{
				/* if task creation failed */
			}
		
		}
	#elif( schedUSE_TCB_SORTED_LIST == 1 )
		#if( schedEDF_NAIVE ==1 )
			const ListItem_t *pxTCBListEndMarker = listGET_END_MARKER( pxTCBList );
			ListItem_t *pxTCBListItem = listGET_HEAD_ENTRY( pxTCBList );
		#endif /* schedEDF_NAIVE */

		while( pxTCBListItem != pxTCBListEndMarker )
		{
			pxTCB = listGET_LIST_ITEM_OWNER( pxTCBListItem );
			configASSERT( NULL != pxTCB );

			BaseType_t xReturnValue = xTaskCreate( prvPeriodicTaskCode, pxTCB->pcName, pxTCB->uxStackDepth, pxTCB->pvParameters, pxTCB->uxPriority, pxTCB->pxTaskHandle );
			if( pdPASS == xReturnValue )
			{

			}
			else
			{
				/* if task creation failed */
			}
			vTaskSetThreadLocalStoragePointer( *pxTCB->pxTaskHandle, schedTHREAD_LOCAL_STORAGE_POINTER_INDEX, pxTCB );
			pxTCBListItem = listGET_NEXT( pxTCBListItem );
		}	
	#endif /* schedUSE_TCB_ARRAY */
}

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
	/* Initiazes fixed priorities of all periodic tasks with respect to RMS or
	 * DMS policy. */
static void prvSetFixedPriorities( void )
{
BaseType_t xIter, xIndex;
TickType_t xShortest, xPreviousShortest=0;
SchedTCB_t *pxShortestTaskPointer, *pxTCB;

	#if( schedUSE_SCHEDULER_TASK == 1 )
		BaseType_t xHighestPriority = schedSCHEDULER_PRIORITY;
	#else
		BaseType_t xHighestPriority = configMAX_PRIORITIES;
	#endif /* schedUSE_SCHEDULER_TASK */

	for( xIter = 0; xIter < xTaskCounter; xIter++ )
	{
		xShortest = portMAX_DELAY;

		/* search for shortest period/deadline */
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			pxTCB = &xTCBArray[ xIndex ];
			configASSERT( pdTRUE == pxTCB->xInUse );
			if(pdTRUE == pxTCB->xPriorityIsSet)
			{
				continue;
			}

			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
				if( pxTCB->xPeriod <= xShortest )
				{
					xShortest = pxTCB->xPeriod;
					pxShortestTaskPointer = pxTCB;
				}
			#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
				if( pxTCB->xRelativeDeadline <= xShortest )
				{
					xShortest = pxTCB->xRelativeDeadline;
					pxShortestTaskPointer = pxTCB;
				}
			#endif /* schedSCHEDULING_POLICY */
		}
		configASSERT( -1 <= xHighestPriority );
		if( xPreviousShortest != xShortest )
		{
			xHighestPriority--;
		}
		/* set highest priority to task with xShortest period (the highest priority is configMAX_PRIORITIES-1) */
		pxShortestTaskPointer->uxPriority = xHighestPriority;
		pxShortestTaskPointer->xPriorityIsSet = pdTRUE;

		xPreviousShortest = xShortest;
	}
}
#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
	/* Initializes priorities of all periodic tasks with respect to EDF policy. */
	static void prvInitEDF( void )
	{
	SchedTCB_t *pxTCB;

		#if( schedEDF_NAIVE == 1 )
			
			UBaseType_t uxHighestPriority = configMAX_PRIORITIES - 1;

			const ListItem_t *pxTCBListEndMarker = listGET_END_MARKER( pxTCBList );
			ListItem_t *pxTCBListItem = listGET_HEAD_ENTRY( pxTCBList );

			while( pxTCBListItem != pxTCBListEndMarker )
			{
				pxTCB = listGET_LIST_ITEM_OWNER( pxTCBListItem );

				pxTCB->uxPriority = uxHighestPriority;
				uxHighestPriority--;

				pxTCBListItem = listGET_NEXT( pxTCBListItem );
			}
		#endif /* schedEDF_NAIVE */
	}
#endif /* schedSCHEDULING_POLICY */


/* This function must be called before any other function call from this module. */
void vSchedulerInit( void )
{
	#if( schedUSE_TCB_ARRAY == 1 )
		prvInitTCBArray();
	#elif( schedUSE_TCB_SORTED_LIST == 1 )
		#if( schedEDF_NAIVE == 1 )
			vListInitialise( &xTCBList );
			vListInitialise( &xTCBTempList );
			vListInitialise( &xTCBOverflowedList );
			pxTCBList = &xTCBList;
			pxTCBTempList = &xTCBTempList;
			pxTCBOverflowedList = &xTCBOverflowedList;
		#endif /* schedEDF_NAIVE */
	#endif /* schedUSE_TCB_ARRAY */
}

/* Starts scheduling tasks. All periodic tasks (including polling server) must
 * have been created with API function before calling this function. */
void vSchedulerStart( void )
{

	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_DMS )
		prvSetFixedPriorities();
	#elif( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
		prvInitEDF();
	#endif /* schedSCHEDULING_POLICY */

	prvCreateAllTasks();

	xSystemStartTime = xTaskGetTickCount();
	vTaskStartScheduler();
}