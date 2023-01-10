#include "faultdetector_handler.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>


void FAULTDET_hotUpdateRegions(FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions[FAULTDETECTOR_MAX_CHECKS]) {
#ifdef FAULTDETECTOR_EXECINSW
	FAULTDETECTOR_SW_initRegions(trainedRegions, n_regions);
#else
	FAULTDET_StopRunMode();
	FAULTDETECTOR_initRegions(&FAULTDETECTOR_InstancePtr, trainedRegions, n_regions);
	FAULTDET_start ();
#endif
}

#ifndef FAULTDETECTOR_EXECINSW


void FAULTDET_Train(FAULTDETECTOR_controlStr* contr) {
	contr->command=COMMAND_TRAIN;

	while(!FAULTDETECTOR_isReadyForNextControl(&FAULTDETECTOR_InstancePtr)) {}

	controlForFaultDet=*contr;
	FAULTDETECTOR_startCopy(&FAULTDETECTOR_InstancePtr);
}
void FAULTDET_StopRunMode() {
	FAULTDETECTOR_controlStr contr;
	contr.command=1;
	controlForFaultDet=contr;
	FAULTDETECTOR_startCopy(&FAULTDETECTOR_InstancePtr);
	while (!XFaultdetector_IsIdle(&FAULTDETECTOR_InstancePtr)) {};
}

void FAULTDET_Test(FAULTDETECTOR_controlStr* contr) {
	contr->command=COMMAND_TEST;

	while(!FAULTDETECTOR_isReadyForNextControl(&FAULTDETECTOR_InstancePtr)) {}

	controlForFaultDet=*contr;
	FAULTDETECTOR_startCopy(&FAULTDETECTOR_InstancePtr);
}

void FAULTDET_getLastTestedPoint(FAULTDETECTOR_testpointDescriptorStr* dest) {
	FAULTDETECTOR_getLastTestedPoint(&FAULTDETECTOR_InstancePtr, ((*pxCurrentTCB_ptr)->uxTaskNumber)-1, dest);
}

void FAULTDET_resetFault() {
	FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, ((*pxCurrentTCB_ptr)->uxTaskNumber)-1);
}
void FAULTDET_initFaultDetection(FAULTDET_ExecutionDescriptor* instance) {
	//FAULTDET_resetFault(); //not needed, automatically done by the faultdetector when a command from the same check but with different UniId is received
	if ((*pxCurrentTCB_ptr)->executionMode==EXECMODE_FAULT) {
		FAULTDET_getLastTestedPoint(&((*pxCurrentTCB_ptr)->lastError));
	}
	instance->testedOnce=0x0;
}
#endif

#ifndef FAULTDETECTOR_EXECINSW
void FAULTDET_blockIfFaultDetectedInTask (FAULTDET_ExecutionDescriptor* instance) {
	if ((*pxCurrentTCB_ptr)->reExecutions<configMAX_REEXECUTIONS_SET_IN_HW_SCHEDULER) {
		if (instance->testedOnce) {
			u8 taskId=((*pxCurrentTCB_ptr)->uxTaskNumber)-1;

			FAULTDETECTOR_testpointShortDescriptorStr out;
			do {
				FAULTDETECTOR_getLastTestedPointShort(&FAULTDETECTOR_InstancePtr, taskId, &out);
			}
			while(memcmp(&(instance->lastTest), &out, sizeof(FAULTDETECTOR_testpointShortDescriptorStr))!=0);

			if(FAULTDETECTOR_hasFault(&FAULTDETECTOR_InstancePtr, taskId)) {
				while(1) {}
			}
		}
	}
}
#endif

#ifdef detectionPerformanceMeasurement
#ifndef FAULTDETECTOR_EXECINSW

void FAULTDET_testing_blockUntilProcessed (FAULTDET_ExecutionDescriptor* instance) {
	if ((*pxCurrentTCB_ptr)->reExecutions<configMAX_REEXECUTIONS_SET_IN_HW_SCHEDULER) {
		if (instance->testedOnce) {
			u8 taskId=((*pxCurrentTCB_ptr)->uxTaskNumber)-1;

			FAULTDETECTOR_testpointShortDescriptorStr out;
			do {
				FAULTDETECTOR_getLastTestedPointShort(&FAULTDETECTOR_InstancePtr, taskId, &out);
			}
			while(memcmp(&(instance->lastTest), &out, sizeof(FAULTDETECTOR_testpointShortDescriptorStr))!=0);
		}
	}
}
#endif
#define GOLDEN_RESULT_SIZE 98304
#define GOLDEN_MANUAL_RESULT_SIZE GOLDEN_RESULT_SIZE
#define RELATIVE_ERROR_SIZE GOLDEN_RESULT_SIZE

static int FAULTDET_testing_goldenResults_size=0;
static int FAULTDET_testing_goldenResults_idx_tmp=0;
static int FAULTDET_testing_manual_goldenResults_size=0;
static int FAULTDET_testing_manual_currGolden=0;

FAULTDETECTOR_testpointDescriptorStr FAULTDET_testing_goldenResults[GOLDEN_RESULT_SIZE];
float FAULTDET_testing_manual_golden[GOLDEN_RESULT_SIZE];
static float FAULTDET_testing_relativeErrors[RELATIVE_ERROR_SIZE];
static int FAULTDET_testing_relativeErrors_size=0;


static int FAULTDET_testing_total=0;
static int FAULTDET_testing_ok=0;
static int FAULTDET_testing_total_golden=0;
static int FAULTDET_testing_ok_golden=0;
static int FAULTDET_testing_falsePositives_golden=0;
static int FAULTDET_testing_falseNegatives=0;
static int FAULTDET_testing_noeffects=0;
static int FAULTDET_testing_falseNegatives_wtolerance=0;
static int FAULTDET_testing_ok_wtolerance=0;

static char FAULTDET_testing_temp_aovchanged=0;
static char FAULTDET_testing_temp_lastoutputchanged=0;

void FAULTDET_testing_manual_result(float value, char injectingFaults) {
	if (injectingFaults) {
		if (value!=FAULTDET_testing_manual_golden[FAULTDET_testing_manual_currGolden]) {
			float relErr=fabs(value - FAULTDET_testing_manual_golden[FAULTDET_testing_manual_currGolden])/fabs(FAULTDET_testing_manual_golden[FAULTDET_testing_manual_currGolden]);
			FAULTDET_testing_relativeErrors[FAULTDET_testing_relativeErrors_size]=relErr;
			FAULTDET_testing_relativeErrors_size++;

			//			FAULTDET_testing_temp_aovchanged=0xFF;
			FAULTDET_testing_temp_lastoutputchanged=0xFF;
		}
		FAULTDET_testing_manual_currGolden++;
	} else {
		if (FAULTDET_testing_manual_goldenResults_size<=GOLDEN_MANUAL_RESULT_SIZE) {
			FAULTDET_testing_manual_golden[FAULTDET_testing_manual_goldenResults_size]=value;
			FAULTDET_testing_manual_goldenResults_size++;
		} else {
			printf("ERROR: max manual golden results size exceeded");
		}
	}
}

void FAULTDET_testing_manual_compare_n_result (int n, float value) {
	if (value!=FAULTDET_testing_manual_golden[n]) {
		float relErr=fabs(value - FAULTDET_testing_manual_golden[n])/fabs(FAULTDET_testing_manual_golden[n]);
		FAULTDET_testing_relativeErrors[FAULTDET_testing_relativeErrors_size]=relErr;
		FAULTDET_testing_relativeErrors_size++;

		//			FAULTDET_testing_temp_aovchanged=0xFF;
		FAULTDET_testing_temp_lastoutputchanged=0xFF;
	}
}

void FAULTDET_testing_resetGoldens () {
	FAULTDET_testing_goldenResults_size=0;
	FAULTDET_testing_relativeErrors_size=0;
	FAULTDET_testing_goldenResults_idx_tmp=0;

	FAULTDET_testing_manual_goldenResults_size=0;
	FAULTDET_testing_manual_currGolden=0;
}




void FAULTDET_testing_commitTmpStatsAndReset(u8 injectingFault) {
#ifndef csvOut
	FAULTDET_testing_loggin_faultdetected=FAULTDET_testing_temp_faultdetected;

	if (injectingFault) {
		FAULTDET_testing_total++;

		if (FAULTDET_testing_temp_aovchanged) {
			if (FAULTDET_testing_temp_faultdetected) {
				FAULTDET_testing_ok++;
				//FAULTDET_testing_ok_wtolerance++;

			} else {
				if (FAULTDET_testing_temp_lastoutputchanged) {
					FAULTDET_testing_falseNegatives++;

					float maxErr=FAULTDET_testing_relativeErrors[0];
					for (int i=0; i<FAULTDET_testing_relativeErrors_size; i++) {
						if (FAULTDET_testing_relativeErrors[i]>maxErr)
							maxErr=FAULTDET_testing_relativeErrors[i];
					}

					union {
						float f;
						uint32_t u;
					} f2u = { .f = maxErr };



					//					if (FAULTDET_testing_relativeErrors[FAULTDET_testing_relativeErrors_size-1]>0.15f)
					//						FAULTDET_testing_falseNegatives_wtolerance++;
					//					else
					//						FAULTDET_testing_ok_wtolerance++;
					//									for (int i=0; i<FAULTDET_testing_relativeErrors_size; i++) {
					//										printf("%f;", FAULTDET_testing_relativeErrors[i]);
					//									}
					if (FAULTDET_testing_falseNegatives==1)
						printf("%u", f2u.u);
					else
						printf(",%u", f2u.u);

					fflush(stdout);
				} else {
					FAULTDET_testing_ok++;
					//FAULTDET_testing_ok_wtolerance++;
				}
			}
		} else {
			FAULTDET_testing_noeffects++;
		}
	} else {
		FAULTDET_testing_total_golden++;
		if (FAULTDET_testing_temp_faultdetected) {
			FAULTDET_testing_falsePositives_golden++;
		} else {
			FAULTDET_testing_ok_golden++;
		}
	}
	FAULTDET_testing_relativeErrors_size=0;
	FAULTDET_testing_manual_currGolden=0;


	FAULTDET_testing_temp_faultdetected=0;
	FAULTDET_testing_temp_aovchanged=0;
	FAULTDET_testing_temp_lastoutputchanged=0;
#endif
}

int FAULTDET_testing_getTotal_golden() {
	return FAULTDET_testing_total_golden;
}

int FAULTDET_testing_getOk_golden() {
	return FAULTDET_testing_ok_golden;
}

int FAULTDET_testing_getTotal() {
	return FAULTDET_testing_total;
}

int FAULTDET_testing_getOk() {
	return FAULTDET_testing_ok;
}

int FAULTDET_testing_getOk_wtolerance() {
	return FAULTDET_testing_ok_wtolerance;
}

int FAULTDET_testing_getFalsePositives_golden() {
	return FAULTDET_testing_falsePositives_golden;
}

int FAULTDET_testing_getFalseNegatives() {
	return FAULTDET_testing_falseNegatives;
}

int FAULTDET_testing_getFalseNegatives_wtolerance() {
	return FAULTDET_testing_falseNegatives_wtolerance;
}

int FAULTDET_testing_getNoEffects() {
	return FAULTDET_testing_noeffects;
}

//void FAULTDET_testing_increaseOk() {
//	FAULTDET_testing_ok++;
//}
//
//void FAULTDET_testing_increaseFalseNegatives() {
//	FAULTDET_testing_falseNegatives++;
//}


FAULTDETECTOR_testpointDescriptorStr* FAULTDET_testing_findGolden (FAULTDETECTOR_testpointDescriptorStr* newRes) {
	for (int i=0; i<FAULTDET_testing_goldenResults_size; i++) {
		if (newRes->checkId==FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_idx_tmp].checkId &&
				newRes->executionId==FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_idx_tmp].executionId &&
				newRes->uniId==FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_idx_tmp].uniId) {
			return &(FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_idx_tmp]);
		}
		if (FAULTDET_testing_goldenResults_idx_tmp==(FAULTDET_testing_goldenResults_size-1)) {
			FAULTDET_testing_goldenResults_idx_tmp=0;
		}
		else {
			FAULTDET_testing_goldenResults_idx_tmp++;
		}
	}
	printf("ERROR: golden not found");
	return 0x0;
}
#include <math.h>
#define GOLDENCOMPARE_THRESH_CONSTANT (1e-10f)

u8 FAULTDET_testing_isAovEqual(FAULTDETECTOR_testpointDescriptorStr* golden, FAULTDETECTOR_testpointDescriptorStr* toTest, int lobound, int upbound) {
	//	return memcmp(&(desc1->AOV), &(desc2->AOV), sizeof(desc1->AOV))==0;
	u8 equal=0xFF;
	u8 outEqual=0xFF;
	for (int i=0; i<FAULTDETECTOR_MAX_AOV_DIM; i++) {
		//		float tresh=fabs(golden->AOV[i])*0.1;
		if (toTest->AOV[i] != golden->AOV[i]/*fabs(toTest->AOV[i] - golden->AOV[i]) > GOLDENCOMPARE_THRESH_CONSTANT*/) {
			if (i>=lobound && i<=upbound) {
				outEqual=0x0;
				float relErr=fabs(toTest->AOV[i] - golden->AOV[i])/fabs(golden->AOV[i]);
#if csvOut
				printf("0.%f.", relErr);
#else
				if (FAULTDET_testing_relativeErrors_size<=RELATIVE_ERROR_SIZE) {
					FAULTDET_testing_relativeErrors[FAULTDET_testing_relativeErrors_size]=relErr;
					FAULTDET_testing_relativeErrors_size++;
				} else {
					printf("ERROR: max relative errors size exceeded");
				}
#endif
			}
			equal=0x0;
		}
	}
	//	return 0xFF;
#ifdef csvOut
	if (outEqual)
		printf("1.0.");
#else
	FAULTDET_testing_temp_lastoutputchanged=!outEqual;
#endif

#ifdef csvOut
	if (equal)
		printf("1;");
	else
		printf("0;");
#else
	if (!equal)
		FAULTDET_testing_temp_aovchanged=0xFF;
#endif
	return equal;
}

void FAULTDET_testing_resetStats() {
	FAULTDET_testing_total_golden=0;
	FAULTDET_testing_ok_golden=0;
	FAULTDET_testing_total=0;
	FAULTDET_testing_ok=0;
	FAULTDET_testing_falsePositives_golden=0;
	FAULTDET_testing_falseNegatives=0;
	FAULTDET_testing_temp_aovchanged=0;
	FAULTDET_testing_temp_faultdetected=0;
	FAULTDET_testing_relativeErrors_size=0;
	FAULTDET_testing_temp_lastoutputchanged=0;
	FAULTDET_testing_falseNegatives_wtolerance=0;
	FAULTDET_testing_ok_wtolerance=0;
}

#endif


//warning: uniId must start from 1!
void FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
		FAULTDET_ExecutionDescriptor* instance,
#endif
		int uniId, int checkId,
#ifdef detectionPerformanceMeasurement
		u8 injectingErrors,
		int goldenLobound,
		int goldenUpbound,
		int testingExecutionId,
#endif
		int argCount, ...) {

	va_list ap;
	va_start(ap, argCount);
	if (argCount>FAULTDETECTOR_MAX_AOV_DIM) {
		printf("ERROR: MAX AOV DIM SIZE EXCEEDED\n");
		return; //error
	}

	FAULTDETECTOR_controlStr contr;
#ifndef detectionPerformanceMeasurement
	TCB_t* tcbPtr=*pxCurrentTCB_ptr;
#endif


	contr.uniId=uniId;
	contr.checkId=checkId;
#ifndef detectionPerformanceMeasurement
	contr.taskId=tcbPtr->uxTaskNumber-1;
	contr.executionId=tcbPtr->executionId;
#else
	contr.taskId=0;
	contr.executionId=1;
#endif

	for (int i=0; i<argCount; i++) {
		contr.AOV[i]=*va_arg(ap, float*);
	}
	for (int i=argCount; i<FAULTDETECTOR_MAX_AOV_DIM; i++) {
		contr.AOV[i]=0.0;
	}

#ifndef detectionPerformanceMeasurement
	u16 lastErrorUniId=tcbPtr->lastError.uniId;
	u8 lastErrorCheckId=tcbPtr->lastError.checkId;

	if (tcbPtr->executionMode==EXECMODE_FAULT && tcbPtr->lastError.uniId==uniId && tcbPtr->lastError.checkId==checkId) {
		tcbPtr->lastError.uniId=0xFFFF;
		tcbPtr->lastError.checkId=0xFF;
	}

	if (tcbPtr->executionMode==EXECMODE_FAULT && lastErrorUniId==uniId && lastErrorCheckId==checkId && memcmp(tcbPtr->lastError.AOV, contr.AOV, sizeof(contr.AOV))==0) {
#ifdef FAULTDETECTOR_EXECINSW
		//		printf(" SW FAULT DETECTOR: train");
		FAULTDETECTOR_SW_train(&contr);
#else //!FAULTDETECTOR_EXECINSW
		FAULTDET_Train(&contr);
		//						FAULTDET_Test(&controlForFaultDet);
		//						instance->testedOnce=0xFF;
		//						instance->lastTest.checkId=checkId;
		//						instance->lastTest.executionId=tcbPtr->executionId;
		//						instance->lastTest.uniId=uniId;
		//
		//						if (blocking) {
		//							FAULTDET_blockIfFaultDetectedInTask(instance);
		//						}
#endif //FAULTDETECTOR_EXECINSW
	} else if (tcbPtr->reExecutions<configMAX_REEXECUTIONS_SET_IN_HW_SCHEDULER) {
#endif
#ifdef FAULTDETECTOR_EXECINSW
		char fault=FAULTDETECTOR_SW_test(&contr);
		//		printf(" SW FAULT DETECTOR: fault %x", fault);
#ifndef detectionPerformanceMeasurement

		if (fault) {
			tcbPtr->lastError.uniId=contr.uniId;
			tcbPtr->lastError.checkId=contr.checkId;
			tcbPtr->lastError.executionId=contr.executionId;
			memcpy(&(tcbPtr->lastError.AOV), &(contr.AOV), sizeof(contr.AOV));
		}
#endif
#ifdef detectionPerformanceMeasurement

		if (injectingErrors==0) {
			if (FAULTDET_testing_goldenResults_size<GOLDEN_RESULT_SIZE) {
				if (fault) {
					FAULTDET_testing_temp_faultdetected=0xFF;
#ifdef csvOut
					printf("%d.%d.%d.1.0.0.0;", testingExecutionId, checkId, uniId);
				} else {
					printf("%d.%d.%d.0.0.0.0;", testingExecutionId, checkId, uniId);
#endif
				}

				FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_size].uniId=contr.uniId;
				FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_size].executionId=contr.executionId;
				FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_size].checkId=contr.checkId;
				memcpy(&(FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_size].AOV), &(contr.AOV), sizeof(contr.AOV));

				FAULTDET_testing_goldenResults_size++;
			} else {
				printf("ERROR: reached max number golden result size. Not saved.");
			}
		} else {
			FAULTDETECTOR_testpointDescriptorStr curr;

			curr.uniId=contr.uniId;
			curr.executionId=contr.executionId;
			curr.checkId=contr.checkId;
			memcpy(&(curr.AOV), &(contr.AOV), sizeof(contr.AOV));

			FAULTDETECTOR_testpointDescriptorStr* golden=FAULTDET_testing_findGolden(&curr);

			if (FAULTDET_testing_isAovEqual(golden, &curr, goldenLobound, goldenUpbound)==0) {
				if (fault) {
					//				FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, contr.taskId);
					FAULTDET_testing_temp_faultdetected=fault;
#ifdef csvOut
					printf("%d.%d.%d.1.", testingExecutionId, checkId, uniId);
				} else {
					printf("%d.%d.%d.0.", testingExecutionId, checkId, uniId);
#endif
				}
			}
#ifndef detectionPerformanceMeasurement
		}
#endif

#else //!detectionPerformanceMeasurement
		SCHEDULER_restartFaultyJob((void*) SCHEDULER_BASEADDR, tcbPtr->uxTaskNumber, contr.executionId);
		while(1) {}
	}
#endif //detectionPerformanceMeasurement

#else //!FAULTDETECTOR_EXECINSW
	FAULTDET_Test(&contr);
	instance->testedOnce=0xFF;
	instance->lastTest.checkId=checkId;
	instance->lastTest.executionId=tcbPtr->executionId;
	instance->lastTest.uniId=uniId;

#ifdef detectionPerformanceMeasurement

	if (injectingErrors==0) {
		if (FAULTDET_testing_goldenResults_size<GOLDEN_RESULT_SIZE) {
			FAULTDET_testing_blockUntilProcessed(instance);
			if (FAULTDETECTOR_hasFault(&FAULTDETECTOR_InstancePtr, contr.taskId)) {
				FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, contr.taskId);
				FAULTDET_testing_temp_faultdetected=0xFF;
#ifdef csvOut
				printf("%d.%d.%d.1.0.0.0;",testingExecutionId, checkId, uniId);
			} else {
				printf("%d.%d.%d.0.0.0.0;",testingExecutionId, checkId, uniId);
#endif
			}

			FAULTDETECTOR_getLastTestedPoint(&FAULTDETECTOR_InstancePtr, contr.taskId, &(FAULTDET_testing_goldenResults[FAULTDET_testing_goldenResults_size]));
			FAULTDET_testing_goldenResults_size++;
		} else {
			printf("ERROR: reached max number golden result size. Not saved.");
		}
	} else {
		FAULTDET_testing_blockUntilProcessed(instance);
		FAULTDETECTOR_testpointDescriptorStr curr;
		FAULTDETECTOR_getLastTestedPoint(&FAULTDETECTOR_InstancePtr, contr.taskId, &curr);

		FAULTDETECTOR_testpointDescriptorStr* golden=FAULTDET_testing_findGolden(&curr);

		if (FAULTDET_testing_isAovEqual(golden, &curr, goldenLobound, goldenUpbound)==0) {
			char fault=FAULTDETECTOR_hasFault(&FAULTDETECTOR_InstancePtr, contr.taskId);
			if (fault) {
				FAULTDET_testing_temp_faultdetected=0xFF;
				//				FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, contr.taskId);
#ifdef csvOut
				printf("%d.%d.%d.1.", testingExecutionId, checkId, uniId);


			} else {
				printf("%d.%d.%d.0.", testingExecutionId, checkId, uniId);
#endif
			}
		}
		FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, contr.taskId);

		//		FAULTDET_testing_temp_changed = FAULTDET_testing_temp_changed || FAULTDET_testing_isAovEqual(&curr, golden, goldenLobound, goldenUpbound)==0;
		//		FAULTDET_testing_temp_faultdetected=FAULTDET_testing_temp_faultdetected || fault;
	}
#endif //detectionPerformanceMeasurement
#endif //FAULTDETECTOR_EXECINSW
}

va_end(ap);
}

void FAULTDET_trainPoint(int uniId, int checkId, int argCount, ...) {
	va_list ap;
	va_start(ap, argCount);
	if (argCount>FAULTDETECTOR_MAX_AOV_DIM) //MAX_AOV_DIM
		return; //error

	FAULTDETECTOR_controlStr contr;

#ifndef detectionPerformanceMeasurement
	TCB_t* tcbPtr=*pxCurrentTCB_ptr;
#endif

	contr.uniId=uniId;
	contr.checkId=checkId;
#ifndef detectionPerformanceMeasurement
	contr.taskId=tcbPtr->uxTaskNumber-1;
	contr.executionId=tcbPtr->executionId;
#else
	contr.taskId=0;
	contr.executionId=1;
#endif

	for (int i=0; i<argCount; i++) {
		contr.AOV[i]=*va_arg(ap, float*);
	}
	for (int i=argCount; i<FAULTDETECTOR_MAX_AOV_DIM; i++) {
		contr.AOV[i]=0.0;
	}

#ifdef FAULTDETECTOR_EXECINSW

	char fault=FAULTDETECTOR_SW_test(&contr);
	if (fault) {
		FAULTDETECTOR_SW_train(&contr);
		fault=FAULTDETECTOR_SW_test(&contr);
		if (fault) {
			printf("Train failed, checkId %d, uniId %d", checkId, uniId);
		}
		//		 else
		//			 printf("Train ok, checkId %d, uniId %d", checkId, uniId);

	}
#else
	FAULTDET_Test(&contr);

	FAULTDET_ExecutionDescriptor instance;
	instance.testedOnce=0xFF;
	instance.lastTest.checkId=checkId;
	instance.lastTest.executionId=tcbPtr->executionId;
	instance.lastTest.uniId=uniId;

	FAULTDET_testing_blockUntilProcessed(&instance);
	if (FAULTDETECTOR_hasFault(&FAULTDETECTOR_InstancePtr, contr.taskId)) {
		FAULTDETECTOR_resetFault(&FAULTDETECTOR_InstancePtr, contr.taskId);
		FAULTDET_Train(&contr);
	}
#endif
	va_end(ap);
}
