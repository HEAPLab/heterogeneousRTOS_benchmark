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
//	for (int i=0; i<FAULTDETECTOR_MAX_AOV_DIM; i++) {
//		trainedRegions[0][0].center[i]=0;
//		trainedRegions[0][0].max[i]=200;
//		trainedRegions[0][0].min[i]=-200;
//		n_regions[0]=1;
//	}

//	xRTTaskCreate( prvTaskOne,
//			( const char * ) "One",
//			configMINIMAL_STACK_SIZE,
//			NULL,
//			tskIDLE_PRIORITY,
//			NULL,
//			2500, //deadline
//			2500, //period
//			1,
//			1000,
//			1000
//			); //wcet


	xRTTaskCreate( prvTaskTwo,
			( const char * ) "Two",
			configMINIMAL_STACK_SIZE,
			NULL,
			tskIDLE_PRIORITY,
			NULL,
			5000, //deadline
			5000, //period
			0,
			2500
	); //wcet



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

	//	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
	//		n_regions[i]=0;
	//		for (int j=0; j<FAULTDETECTOR_MAX_REGIONS; j++) {
	//			for (int k=0; k<=FAULTDETECTOR_MAX_AOV_DIM; k++) {
	//				trainedRegions[i][j].center[k]=0.0f;
	//				trainedRegions[i][j].max[k]=0.0f;
	//				trainedRegions[i][j].min[k]=0.0f;
	//			}
	//		}
	//
	//	}

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

typedef struct {
	unsigned int job;
	char ev;
	unsigned int tskId;
	u32 timer;
} regEvent;
#define outlimit 200
regEvent outcontainera[outlimit];
regEvent outcontainerb[outlimit];

unsigned int outa=0;
unsigned int outb=0;

void printout() {
	unsigned int outaread=0;
	unsigned int outbread=0;

//	for (int outaread=0; outaread<=outa; outaread++) {
//		u32 res=(outcontainera[outaread].timer*(2000000000.0/666666687.0));
//		xil_printf("%c t%u j%u %"PRIu32"\n",outcontainera[outaread].ev,outcontainera[outaread].tskId, outcontainera[outaread].job, res);
//	}
	while (outaread<outa || outbread<outb) {
		if (outaread<=outa && ((outbread>outb) || (outcontainera[outaread].timer<outcontainerb[outbread].timer))) {
			u32 res=(outcontainera[outaread].timer*(2000000000.0/666666687.0));
			xil_printf("%c t%u j%u %"PRIu32"\n",outcontainera[outaread].ev,outcontainera[outaread].tskId, outcontainera[outaread].job, res);
			outaread++;
		}
		if (outbread<=outb && ((outaread>outa) || (outcontainera[outaread].timer>=outcontainerb[outbread].timer))) {
			u32 res=(outcontainerb[outbread].timer*(2000000000.0/666666687.0));
			xil_printf("%c t%u j%u %u\n",outcontainerb[outbread].ev,outcontainerb[outbread].tskId, outcontainerb[outbread].job, res);
			outbread++;
		}
	}
}

unsigned int task1jobidx=0;
static void prvTaskOne( void *pvParameters )
{
	for (;;) {
		outa++;
		outcontainera[outa].timer=get_clock_L();
		outcontainera[outa].ev='s';
		outcontainera[outa].tskId=1;
		outcontainera[outa].job=task1jobidx;
		outa++;
		perf_start_clock();

		if (outa>=outlimit){
			xPortSchedulerDisableIntr();
			printout();
			while(1) {}
		}

		FAULTDETECTOR_controlStr* contr=FAULTDET_initFaultDetection();
		//			perf_stop_clock();
//		volatile uint32_t clk=get_clock_L();
//		volatile uint32_t clku=get_clock_U();
//		volatile uint64_t tot=(uint64_t) clk | ((uint64_t) clku) << 32;

//		if (task2jobidx>0) {
//			outi++;
//		}
////		sprintf(&(outcontainer[outi]), "s2 %" PRIu64 "\n", tot);
//		sprintf(&(outcontainer[outi]), "s1 %u\n", get_clock_L());
//
//		outi++;

		perf_start_clock();

		for(int i=0; i<600; i++) {}

		//		contr->checkId=0;
		//		contr->uniId=1;
		//
		//		contr->AOV[0]=1000;
		//		contr->AOV[0]=500;
		//		contr->AOV[0]=250;
		//		contr->AOV[0]=0;
		//
		//		FAULTDET_testPoint();
		//
		//		for (int i=0; i<500000; i++) {}
		//
		//		FAULTDET_blockIfFaultDetectedInTask();
		//		xil_printf("end\n");
		outcontainera[outa].ev='e';
		outcontainera[outa].tskId=1;
		outcontainera[outa].job=task1jobidx;
		task1jobidx++;
		vTaskJobEnd(&(outcontainera[outa].timer));
	}
}
/*-----------------------------------------------------------*/

unsigned int task2jobidx=0;
static void prvTaskTwo( void *pvParameters )
{
	for (;;) {
		outb++;
		outcontainerb[outb].timer=get_clock_L();
		outcontainerb[outb].ev='s';
		outcontainerb[outb].tskId=2;
		outcontainerb[outb].job=task2jobidx;
		outb++;
		perf_start_clock();

		if (outb>=outlimit){
			xPortSchedulerDisableIntr();
			printout();
			while(1) {}
		}

		FAULTDETECTOR_controlStr* contr=FAULTDET_initFaultDetection();
		//			perf_stop_clock();
//		volatile uint32_t clk=get_clock_L();
//		volatile uint32_t clku=get_clock_U();
//		volatile uint64_t tot=(uint64_t) clk | ((uint64_t) clku) << 32;

//		if (task2jobidx>0) {
//			outi++;
//		}
////		sprintf(&(outcontainer[outi]), "s2 %" PRIu64 "\n", tot);
//		sprintf(&(outcontainer[outi]), "s1 %u\n", get_clock_L());
//
//		outi++;

		perf_start_clock();

		for(int i=0; i<4000; i++) {}

		//		contr->checkId=0;
		//		contr->uniId=1;
		//
		//		contr->AOV[0]=1000;
		//		contr->AOV[0]=500;
		//		contr->AOV[0]=250;
		//		contr->AOV[0]=0;
		//
		//		FAULTDET_testPoint();
		//
		//		for (int i=0; i<500000; i++) {}
		//
		//		FAULTDET_blockIfFaultDetectedInTask();
		//		xil_printf("end\n");
		outcontainerb[outb].ev='e';
		outcontainerb[outb].tskId=2;
		outcontainerb[outb].job=task2jobidx;
		task2jobidx++;
		vTaskJobEnd(&(outcontainerb[outb].timer));
	}
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
