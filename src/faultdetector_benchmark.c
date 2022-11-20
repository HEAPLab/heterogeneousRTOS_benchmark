/*
 ============================================================================
 Name        : faultdetector_benchmark.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#define latnavBench
#define FAULTDETECTOR_EXECINSW
#define testingCampaign

#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "simple_random.h"
#include "faultdetector_handler.h"

u8 injectingErrors=0;
float r1, r2, r3, r4;



#ifdef latnavBench

#define TIME_STEP 0.001

#define CLAMP(x, minim, maxim) (x < minim ? minim : (x > maxim ? maxim : x))

#define ITERATIONS 100


typedef struct pid_controller_s {

	float p;
	float i;
	float d;
	float b;

	float prev_error;
	float integral_sum;
	float backpropagation;

} pid_controller_t;


float run_pid(pid_controller_t* pid, float error, int executionId) {
	float output;

	FAULTDET_testing_injectFault32(pid->p, executionId, 32*0, (32*1)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->i, executionId, 32*1, (32*2)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->d, executionId, 32*2, (32*3)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->b, executionId, 32*3, (32*4)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->prev_error, executionId, 32*4, (32*5)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->integral_sum, executionId, 32*5, (32*6)-1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->backpropagation, executionId, 32*6, (32*7)-1, injectingErrors);

	output = error * pid->p;

	FAULTDET_testing_injectFault32(output, executionId, 32*7, (32*8)-1, injectingErrors);

	pid->integral_sum += ((error * pid->i) + (pid->b * pid->backpropagation)) * TIME_STEP;

	FAULTDET_testing_injectFault32(pid->integral_sum, executionId, 32*8, (32*9)-1, injectingErrors);

	output += pid->integral_sum;

	FAULTDET_testing_injectFault32(output, executionId, 32*9, (32*10)-1, injectingErrors);

	output += pid->d * (error - pid->prev_error) / TIME_STEP;

	FAULTDET_testing_injectFault32(output, executionId, 32*10, (32*11)-1, injectingErrors);

	pid->prev_error = error;

	return output;
}

float roll_limiter(float desired_roll, float speed, int executionId) {
	float limit_perc,limit;

	FAULTDET_testing_injectFault32(speed, executionId, 32*0, (32*1)-1, injectingErrors);
	FAULTDET_testing_injectFault32(desired_roll, executionId, 32*4, (32*5)-1, injectingErrors);

	if (speed <= 140) {
		return CLAMP(desired_roll, -30, 30);
	}
	if (speed >= 300) {
		return CLAMP(desired_roll, -40, 40);
	}

	FAULTDET_testing_injectFault32(speed, executionId, 32*1, (32*2)-1, injectingErrors);

	limit_perc = (speed < 220) ? (speed-140) / 80 : ((speed-220) / 80);

	FAULTDET_testing_injectFault32(limit_perc, executionId, 32*2, (32*3)-1, injectingErrors);

	limit = (speed < 220) ? (30 + limit_perc * 37) : (40 + (1-limit_perc) * 27);

	FAULTDET_testing_injectFault32(limit, executionId, 32*3, (32*4)-1, injectingErrors);

	return CLAMP (desired_roll, -limit, limit);
}

float roll_rate_limiter(float desired_roll_rate, float roll, int executionId) {
	FAULTDET_testing_injectFault32(roll, executionId, 32*0, (32*1)-1, injectingErrors);
	FAULTDET_testing_injectFault32(desired_roll_rate, executionId, 32*1, (32*2)-1, injectingErrors);


	if (roll < 20 && roll > -20) {
		return CLAMP (desired_roll_rate, -5, 5);
	} else if (roll < 30 && roll > -30) {
		return CLAMP (desired_roll_rate, -3, 3);
	} else {
		return CLAMP (desired_roll_rate, -1, 1);
	}
}

float ailerons_limiter(float aileron, int executionId) {
	FAULTDET_testing_injectFault32(aileron, executionId, 32*0, (32*1)-1, injectingErrors);

	return CLAMP(aileron, -30, 30);
}

float r1, r2, r3, r4;

void latnav(int roundId, int executionId) {

#ifndef FAULTDETECTOR_EXECINSW
	FAULTDET_ExecutionDescriptor inst;
	FAULTDET_initFaultDetection(&inst);
#endif

	pid_controller_t pid_roll_rate,pid_roll,pid_heading;
	float curr_heading,curr_roll,curr_roll_rate;
	int i;


	pid_roll_rate.p = 0.31;
	pid_roll_rate.i = 0.25;
	pid_roll_rate.d = 0.46;
	pid_roll_rate.b = 0.15;


	pid_roll.p = 0.81;
	pid_roll.i = 0.63;
	pid_roll.d = 0.39;
	pid_roll.b = 0.42;

	pid_heading.p = 0.15;
	pid_heading.i = 0.64;
	pid_heading.d = 0.33;
	pid_heading.b = 0.55;

	pid_roll_rate.integral_sum= pid_roll_rate.prev_error= pid_roll_rate.backpropagation= 0;
	pid_roll.integral_sum     = pid_roll.prev_error     = pid_roll.backpropagation     = 0;
	pid_heading.integral_sum  = pid_heading.prev_error  = pid_heading.backpropagation  = 0;


	curr_heading = r1;
	curr_roll = r2;
	curr_roll_rate = r3;




	for(i=0; i<1; i++) {

		float desired_roll,actual_roll,desired_roll_rate,actual_roll_rate,desired_ailerons,actual_ailerons;

		float err=curr_heading-r4;
		float pid_heading_backpropagation_orig=pid_heading.backpropagation;
		float err_orig=err;

		desired_roll = run_pid(&pid_heading, err, executionId);
		actual_roll = roll_limiter(desired_roll, 400, executionId-(32*11));

		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					0,  //checkId
					3,
					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					0, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					2,
					2,
					roundId,
					executionId,
#endif
					3, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
		}

				if (executionId<-1) {
					FAULTDET_trainPoint(
							1,
							3,  //checkId
							2,
							&desired_roll, &actual_roll);
				} else {
					FAULTDET_testPoint(
		#ifndef FAULTDETECTOR_EXECINSW
							&inst,
		#endif
							1, //uniId
							3, //checkId
							0, //BLOCKING OR NON BLOCKING, non blocking
		#ifdef testingCampaign
							injectingErrors,
							1,
							1,
							roundId,
							executionId,
		#endif
							2, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
							&desired_roll, &actual_roll);
				}



		pid_heading.backpropagation = actual_roll - desired_roll;

		float pid_roll_backpropagation_orig=pid_roll.backpropagation;

		float err1=curr_roll - actual_roll;
		float err1_orig=curr_roll - actual_roll;
		desired_roll_rate = run_pid(&pid_roll, err1, executionId-(32*11)-(32*5));
		actual_roll_rate = roll_rate_limiter(desired_roll_rate, curr_roll, executionId-(32*11)-(32*5)-(32*11));

		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					1,  //ceckId
					3,
					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					1, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					2,
					2,
					roundId,
					executionId,
#endif
					3, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
		}

				if (executionId<-1) {
					FAULTDET_trainPoint(
							1,
							4,  //checkId
							3,
							&actual_roll_rate, &curr_roll, &desired_roll_rate);
				} else {
					FAULTDET_testPoint(
		#ifndef FAULTDETECTOR_EXECINSW
							&inst,
		#endif
							1, //uniId
							4, //checkId
							0, //BLOCKING OR NON BLOCKING, non blocking
		#ifdef testingCampaign
							injectingErrors,
							0,
							0,
							roundId,
							executionId,
		#endif
							3, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
							&actual_roll_rate, &curr_roll, &desired_roll_rate);
				}

		pid_roll.backpropagation = actual_roll_rate - desired_roll_rate;
		pid_roll_backpropagation_orig=pid_roll.backpropagation;

		float err2=curr_roll_rate - actual_roll_rate;
		float err2_orig=err2;
		desired_ailerons = run_pid(&pid_roll, err2, executionId-(32*11)-(32*5)-(32*11)-(32*2));
		actual_ailerons = ailerons_limiter(desired_ailerons, executionId-(32*11)-(32*5)-(32*11)-(32*2)-(32*11));

		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					2,  //checkId
					3,
					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					2, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					2,
					2,
					roundId,
					executionId,
#endif
					3, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
		}


				if (executionId<-1) {
					FAULTDET_trainPoint(
							1,
							5,  //checkId
							2,
							&desired_ailerons, &actual_ailerons);
				} else {
					FAULTDET_testPoint(
		#ifndef FAULTDETECTOR_EXECINSW
							&inst,
		#endif
							1, //uniId
							5, //checkId
							0, //BLOCKING OR NON BLOCKING, non blocking
		#ifdef testingCampaign
							injectingErrors,
							1,
							1,
							roundId,
							executionId,
		#endif
							2, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
							&desired_ailerons, &actual_ailerons);
				}

		pid_roll.backpropagation = actual_ailerons - desired_ailerons;


		//				if (executionId<-1) {
		//					FAULTDET_trainPoint(
		//							1,
		//							3,  //checkId
		//							5,
		//							/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &desired_ailerons, &actual_ailerons);
		//				} else {
		//					FAULTDET_testPoint(
		//		#ifndef FAULTDETECTOR_EXECINSW
		//							&inst,
		//		#endif
		//							1, //uniId
		//							3, //checkId
		//							0, //BLOCKING OR NON BLOCKING, non blocking
		//		#ifdef testingCampaign
		//							injectingErrors,
		//							4,
		//							4,
		//							roundId,
		//							executionId,
		//		#endif
		//							5, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
		//							/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &desired_ailerons, &actual_ailerons);
		//				}

		/* Just a random plane model*/
		FAULTDET_testing_injectFault32(curr_roll, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)-1, injectingErrors);
		FAULTDET_testing_injectFault32(curr_roll_rate, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32)-1,  injectingErrors);
		FAULTDET_testing_injectFault32(desired_ailerons, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32)+(32)-1,  injectingErrors);

		//		curr_heading += curr_roll/10 * TIME_STEP;
		//		curr_roll += curr_roll_rate * TIME_STEP;
		//		curr_roll_rate += desired_ailerons / 5;


		FAULTDET_testing_injectFault32(curr_heading, executionId, 1408, 1408+32-1, injectingErrors);
		FAULTDET_testing_injectFault32(curr_roll, executionId, 1408+32, 1408+32+32-1,  injectingErrors);
		FAULTDET_testing_injectFault32(curr_roll_rate, executionId, 1408+32+32, 1408+32+32+32-1 /*1503*/,  injectingErrors);

	}

#ifdef testingCampaign
	if (executionId>=-1) {
		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
	}

	if (executionId==-1) {

		injectingErrors=0xFF;
	}

#endif
}

#endif






int main(void)
{
	printf("start\n");
	random_set_seed(1);

	//	for (int i=0; i<1000; i++) {
	//		injectingErrors=0x0;


	for (int executionId=-8096 ;executionId<-1/*960*/; executionId++) {
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		latnav(0, executionId);
	}

	for (int i=0; i<4000; i++) {
		FAULTDET_testing_resetGoldens();
		injectingErrors=0x0;
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		for (int executionId=-1 ;executionId<1503/*960*/; executionId++) {
			latnav(i, executionId);
		}
		//		}
		//#ifdef trainMode
		//		printf("Training done. ");
		//		FAULTDET_dumpRegions();
		//		while(1) {
		//			//printf("done train");
		//
		//		}
		//#endif
		//			FAULTDET_hotUpdateRegions(trainedRegions, n_regions);
	}
	printf("\ntotal for fp: %d\n", FAULTDET_testing_getTotal_golden());
	printf("ok for fp: %d\n", FAULTDET_testing_getOk_golden());
	printf("fp: %d\n", FAULTDET_testing_getFalsePositives_golden());

	printf("total for fn: %d\n", FAULTDET_testing_getTotal());
	printf("ok for fn: %d\n", FAULTDET_testing_getOk());
	printf("fn: %d\n", FAULTDET_testing_getFalseNegatives());
	printf("ok for fn with tolerance: %d\n", FAULTDET_testing_getOk_wtolerance());
	printf("fn with tolerance: %d\n", FAULTDET_testing_getFalseNegatives_wtolerance());
	printf("no effects: %d\n", FAULTDET_testing_getNoEffects());
}
