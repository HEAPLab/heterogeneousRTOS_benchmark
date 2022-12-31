#ifndef FAULTDETECTOR_HANDLER_HEADER
#define FAULTDETECTOR_HANDLER_HEADER

#include "xil_types.h"

#define FAULTDETECTOR_EXECINSW
#define detectionPerformanceMeasurement

#ifdef FAULTDETECTOR_EXECINSW
#include "faultdetector_sw.h"
#else
#include "xfaultdetector.h"
#endif

#define PRIVILEGED_FUNCTION

typedef struct {
	FAULTDETECTOR_testpointDescriptorStr lastTest;
	char testedOnce;
} FAULTDET_ExecutionDescriptor;
void FAULTDET_dumpRegions() PRIVILEGED_FUNCTION;
void FAULTDET_init(u8 restoreTrainDataFromSd, FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions[FAULTDETECTOR_MAX_CHECKS]) PRIVILEGED_FUNCTION;
void FAULTDET_start() PRIVILEGED_FUNCTION;
//void FAULTDET_dumpRegions(region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions[FAULTDETECTOR_MAX_CHECKS]) PRIVILEGED_FUNCTION;
void FAULTDET_Train(FAULTDETECTOR_controlStr* contr) PRIVILEGED_FUNCTION;
void FAULTDET_Test(FAULTDETECTOR_controlStr* contr) PRIVILEGED_FUNCTION;
void FAULTDET_hotUpdateRegions(FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions[FAULTDETECTOR_MAX_CHECKS]) PRIVILEGED_FUNCTION;
void FAULTDET_blockIfFaultDetectedInTask (FAULTDET_ExecutionDescriptor* instance) PRIVILEGED_FUNCTION;
void FAULTDET_getLastTestedPoint(FAULTDETECTOR_testpointDescriptorStr* dest) PRIVILEGED_FUNCTION;
void FAULTDET_initFaultDetection(FAULTDET_ExecutionDescriptor* instance) PRIVILEGED_FUNCTION;
//void FAULTDET_endFaultDetection() PRIVILEGED_FUNCTION;
void FAULTDET_trainPoint(int uniId, int checkId, int argCount, ...) PRIVILEGED_FUNCTION;
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
		int argCount, ...) PRIVILEGED_FUNCTION;
//char FAULTDET_hasFault()  PRIVILEGED_FUNCTION;
void FAULTDET_resetFault() PRIVILEGED_FUNCTION;

#ifdef detectionPerformanceMeasurement

//float FAULTDET_testing_manual_getNextGolden() PRIVILEGED_FUNCTION;
//void FAULTDET_testing_manual_setNextGolden(float golden) PRIVILEGED_FUNCTION;
//void FAULTDET_testing_manual_resetGoldens();
//void FAULTDET_testing_manual_compareNextGolden(float toTest);
void FAULTDET_testing_manual_result (float value, char injectingFaults) PRIVILEGED_FUNCTION;
void FAULTDET_testing_manual_compare_n_result (int n, float value) PRIVILEGED_FUNCTION;
unsigned char FAULTDET_testing_loggin_faultdetected;
unsigned char FAULTDET_testing_temp_faultdetected;
void FAULTDET_testing_resetGoldens () PRIVILEGED_FUNCTION;
int FAULTDET_testing_getTotal() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getOk() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getTotal_golden() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getOk_golden() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getFalsePositives_golden() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getFalseNegatives() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getFalseNegatives_wtolerance() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getOk_wtolerance() PRIVILEGED_FUNCTION;
int FAULTDET_testing_getNoEffects() PRIVILEGED_FUNCTION;
void FAULTDET_testing_resetStats() PRIVILEGED_FUNCTION;
void FAULTDET_testing_commitTmpStatsAndReset(u8 injectingFault) PRIVILEGED_FUNCTION;
//void FAULTDET_testing_increaseOk() PRIVILEGED_FUNCTION;
//void FAULTDET_testing_increaseFalseNegatives() PRIVILEGED_FUNCTION;

//#define FAULTDET_testing_initTesting()\
//	FAULTDET_testing_injectingErrors=0

#define FAULTDET_testing_injectFault32(var, execId, lobound, inject)\
		if ( inject && execId >= lobound && execId <= ( lobound+31 ) ) {\
			(*((u32*)(&var))) = ( (*((u32*)(&var))) & (~(0x1 << ( lobound == 0 ? execId : ( execId % lobound ) ) ))) | ((~ (*((u32*)(&var))) ) & (0x1 << ( lobound == 0 ? execId : ( execId % lobound ) )));\
		}

#define FAULTDET_testing_injectFault8(var, execId, lobound, inject)\
		if ( inject && execId >= lobound && execId <= ( lobound+7 ) ) {\
			(*((u8*)(&var))) = ( (*((u8*)(&var))) & (~(0x1 << ( lobound == 0 ? execId : ( execId % lobound ) ) ))) | ((~ (*((u8*)(&var))) ) & (0x1 << ( lobound == 0 ? execId : ( execId % lobound ) )));\
		}
#endif

//#define FAULTDET_testing_injectFault32(var, execId, lobound, upbound, inject, offset, offseti)\
//		if ( inject && execId >= ( lobound + ( offset * offseti ) ) = && execId <= ( upbound +  ( offset * offseti ) ) ) {\
//			(*((u32*)(&var))) = ( (*((u32*)(&var))) & (~(0x1 << ( ( lobound + ( offset * offseti ) == 0 ? execId : ( execId % ( lobound + ( offset * offseti ) ) ) ))) | ((~ (*((u32*)(&var))) ) & (0x1 << ( ( lobound + ( offset * offseti ) == 0 ? execId : ( execId % ( lobound + ( offset * offseti ) ) )));\
//		}
//#endif

#endif
