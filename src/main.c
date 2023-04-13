/*
    Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
    Copyright (c) 2012 - 2022 Xilinx, Inc. All Rights Reserved.
	SPDX-License-Identifier: MIT


    http://www.FreeRTOS.org
    http://aws.amazon.com/freertos


    1 tab == 4 spaces!
 */

//#include "xil_printf.h"

#define onOutputOnly
//#define testMode


#include "FreeRTOS.h"
#include "task.h"

#include "portable.h"

#include "perf_timer.h"

//#include "simple_random.h"

//#define FAULTDETECTOR_EXECINSW

/*-----------------------------------------------------------*/

#include <math.h>
static void prvTaskOne( void *pvParameters );
static void prvTaskTwo( void *pvParameters );
static void prvTaskThree( void *pvParameters );
static void prvTaskFour( void *pvParameters );
#include <stdio.h>

static FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS];
static u8 n_regions[FAULTDETECTOR_MAX_CHECKS];


int main( void )
{
	xRTTaskCreate( prvTaskOne,
			( const char * ) "One",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			NULL,
			12500000, //deadline
			12500000, //period
			0,
			10000000
			); //wcet

//	xRTTaskCreate( prvTaskTwo,
//			( const char * ) "Two",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			15000000, //deadline
//			20050000, //period
//			1,
//			5000000,
//			10000000
//			); //wcet


//	xRTTaskCreate( prvTaskOne,
//			( const char * ) "One",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			100000000, //deadline
//			100000000, //period
//			0,
//			20
//			); //wcet
//
//	xRTTaskCreate( prvTaskTwo,
//			( const char * ) "Two",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			150000000, //deadline
//			150000000, //period
//			1,
//			//15000000,
//			20,
//			//15000000*2,
//			20*2); //wcet

//	xRTTaskCreate( prvTaskThree,
//			( const char * ) "Three",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			20000000, //deadline
//			20000000, //period
//			2,
//			20,
//			20*2,
//			20*3); //wcet
//
//
//	xRTTaskCreate( prvTaskFour,
//			( const char * ) "Four",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			25000000, //deadline
//			25000000, //period
//			2,
//			20,
//			20*2,
//			20*3); //wcet

	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
		n_regions[i]=0;
		for (int j=0; j<FAULTDETECTOR_MAX_REGIONS; j++) {
			for (int k=0; k<=FAULTDETECTOR_MAX_AOV_DIM; k++) {
				trainedRegions[i][j].center[k]=0.0f;
				trainedRegions[i][j].max[k]=0.0f;
				trainedRegions[i][j].min[k]=0.0f;
			}
		}

	}

	vTaskStartFaultDetector(
			//#ifdef trainMode
			//			0, //do not load from sd, load from supplied trainedRegions and n_regions instead
			//#else
			//			1,
			//#endif
			0,
			trainedRegions,
			n_regions);
	//	xil_printf("%u\n", sizeof(FAULTDETECTOR_testpointShortDescriptorStr));

	perf_reset_clock();
	vTaskStartScheduler();

	//should never reach this point
	for( ;; );
}

//int i=0;
/*-----------------------------------------------------------*/
#include <inttypes.h>

static void prvTaskOne( void *pvParameters )
{
		for (;;) {
			perf_stop_clock();
			volatile uint32_t clk=get_clock_L();
			volatile uint32_t clku=get_clock_U();
			volatile uint64_t tot=(uint64_t) clk | ((uint64_t) clku) << 32;
			printf("s1 %" PRIu64 "\n", tot);
			fflush(stdout);
			perf_start_clock();

			for (int i=0; i<500000; i++) {}

			vTaskJobEnd();
		}
			//for (;;){}

//		xil_printf(" One\n");
//		for (int i=0; i<5000; i++) {}
//		FAULTDETECTOR_controlStr* contr=FAULTDET_initFaultDetection();
//		contr->AOV[0]=1;
//		contr->AOV[1]=2;
//		contr->AOV[2]=3;
//		contr->AOV[3]=4;
//		contr->checkId=1;
//		contr->uniId=1;
//		FAULTDET_testPoint();
//
//		FAULTDET_blockIfFaultDetectedInTask();
}

/*-----------------------------------------------------------*/
static void prvTaskTwo( void *pvParameters )
{
//	xPortSchedulerDisableIntr();
//	for (;;) {
//		unsigned int clk=get_clock_L();
//		xil_printf("%u\n", clk);
//		perf_reset_clock();
//		if (i>=500000)
//			xPortSchedulerDisableIntr();
//		i++;
//		vTaskJobEnd();
//	}
		//		xil_printf(" Two\n");
//		for (int i=0; i<5000; i++) {}
//		FAULTDETECTOR_controlStr* contr=FAULTDET_initFaultDetection();
//		contr->AOV[0]=1;
//		contr->AOV[1]=2;
//		contr->AOV[2]=3;
//		contr->AOV[3]=4;
//		contr->checkId=0;
//		contr->uniId=1;
//		FAULTDET_testPoint();
//
//		FAULTDET_blockIfFaultDetectedInTask();


//		fflush(stdout);
//
//		vTaskJobEnd();
//	}
}

static void prvTaskThree( void *pvParameters )
{
	for (;;) {
		xil_printf(" Three");
////		fflush(stdout);
//
//		vTaskJobEnd();
	}
}


static void prvTaskFour( void *pvParameters )
{
////	xPortSchedulerDisableIntr();
	for (;;) {
		xil_printf(" Four");
////		fflush(stdout);
//
//		vTaskJobEnd();
	}
}
