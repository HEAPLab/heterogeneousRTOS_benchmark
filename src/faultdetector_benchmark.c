#define latnavBench
//#define FFTBench
//#define ANNBench
//#define gaussianBench

#define FAULTDETECTOR_EXECINSW
#define detectionPerformanceMeasurement

#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "simple_random.h"
#include "faultdetector_handler.h"
#include <string.h>

u8 injectingErrors=0;

#ifdef gaussianBench

//at least one of them must be even
#define IMG_HEIGHT 128
#define IMG_WIDTH 128

#define KERNEL_SIZE 5

#define SIGMA 1.0

#ifndef USER_GAUSS_FILTER
static unsigned char mat_in[IMG_HEIGHT][IMG_WIDTH];
static unsigned char mat_out[IMG_HEIGHT][IMG_WIDTH];
#endif

/* KERNEL_SIZExKERNEL_SIZE gaussian filter with origin in (1,1) */
static float kernel[KERNEL_SIZE][KERNEL_SIZE];

/**
 * @brief It generates a KERNEL_SIZE x KERNEL_SIZE gaussian kernel
 *
 */
static void gaussian_kernel_init(){
	int i,j;
	float sum=0;
	for (i = 0; i < KERNEL_SIZE; i++) {
		for (j = 0; j < KERNEL_SIZE; j++) {
			float x = i - (KERNEL_SIZE - 1) / 2.0;
			float y = j - (KERNEL_SIZE - 1) / 2.0;
			kernel[i][j] =  exp(((pow(x, 2) + pow(y, 2)) / ((2 * pow(SIGMA, 2)))) * (-1));
			sum += kernel[i][j];
		}
	}

	for (i = 0; i < KERNEL_SIZE; i++) {
		for (j = 0; j < KERNEL_SIZE; j++) {
			kernel[i][j] /= sum;
		}
	}
}

/**
 * @brief Performs 2D convolution of KERNEL_SIZExKERNEL_SIZE kernel with mat_in
 *
 * @param p_x  center point x coordinate
 * @param p_y center point y coordinate
 * @return int result of 2d convolution of kernel centred in mat_in[p_x][p_y]
 */
char horizontalAccumulate=0x0;
float oldtemp;
float faultdet_vals[KERNEL_SIZE];
char train;
//int uniIdCtr=0;

#define LOOP2TOTAL (8+32*2)
#define LOOP1TOTAL (LOOP2TOTAL*KERNEL_SIZE)
#define CONVOLUTIONTOTAL (LOOP1TOTAL*KERNEL_SIZE/*+8*1+32*3*/)
//static int convolution2D(int p_x, int p_y, int executionId){
//	int k_r,offset_x,offset_y,i,j;
//	float temp;
//
//	/*Kernel radius*/
//	k_r=KERNEL_SIZE/2;
//
//
//	//	FAULTDET_testing_injectFault32(p_x, executionId, 32*0, injectingErrors);
//	//	FAULTDET_testing_injectFault32(p_y, executionId, 32*1, injectingErrors);
//	//	FAULTDET_testing_injectFault32(k_r, executionId, 32*0, injectingErrors);
//
//	/*kernel can be superimposed? if not we are on borders, then we keep the values unchanged*/
//
//		if(p_x-k_r<0 || p_y-k_r<0 || p_x+k_r>=IMG_HEIGHT || p_y+k_r>=IMG_WIDTH){
//			unsigned char res=mat_in[p_x][p_y];
//			//		FAULTDET_testing_injectFault8(res, executionId, 32*1, injectingErrors);
//			return res;
//		}
//
//	/*offset between kernel's indexes and array's ones*/
//	offset_x=p_x-k_r;
//	offset_y=p_y-k_r;
//
//	//	FAULTDET_testing_injectFault32(offset_x, executionId, 32*1+8*1, injectingErrors);
//	//	FAULTDET_testing_injectFault32(offset_y, executionId, 32*2+8*1, injectingErrors);
//
//	temp=0;
//
//
//	/*    for(i=p_x-k_r;i<=p_x+k_r;i++){
//        for(j=p_y-k_r;j<=p_y+k_r;j++){
//        	unsigned char in=mat_in[i+j][j];
//            temp+=kernel[i-offset_x+j-offset_y][j-offset_y] * in;
//            faultdet_vals[ctr]+=in;
//        }
//        ctr++;
//    }*/
//
//	if (horizontalAccumulate) {
//		for (int i=0; i<KERNEL_SIZE; i++) {
//			faultdet_vals[i]=0;
//		}
//
//		int loop1ctr=0;
//		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1TOTAL
//			int loop2ctr=0;
//			for(j=p_y-k_r;j<=p_y+k_r;j++) { //LOOP2TOTAL
//				unsigned char in=mat_in[i][j];
//				float kernelin=kernel[i-offset_x][j-offset_y];
//				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				temp+=kernelin * in;
//				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				faultdet_vals[loop2ctr]+=in;
//				loop2ctr++;
//			}
//			loop1ctr++;
//		}
//
//		float out=temp>oldtemp ? temp-oldtemp : oldtemp-temp;
//
//		int currVecSize=KERNEL_SIZE;
//		while (currVecSize>5) {
//			for (int i=0; i<KERNEL_SIZE-1; i+=2) {
//				faultdet_vals[currVecSize]=faultdet_vals[currVecSize+1];
//				currVecSize--;
//				if (currVecSize<=5)
//					break;
//			}
//		}
//
//		if (train) {
//			FAULTDET_trainPoint(
//					p_x*IMG_HEIGHT+p_y,
//					0,  //checkId
//					6,
//					&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(out));
//		} else {
//			FAULTDET_testPoint(
//#ifndef FAULTDETECTOR_EXECINSW
//					&inst,
//#endif
//					p_x*IMG_HEIGHT+p_y/*p_x*IMG_WIDTH+p_y*/,
//					0, //checkId
//#ifdef detectionPerformanceMeasurement
//					injectingErrors,
//					-1,
//					-1,
//					executionId,
//#endif
//					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
//					&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(out));
//			//			uniIdCtr++;
//		}
//
//
//	} else {
//		int loop1ctr=0;
//		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1
//			int loop2ctr=0;
//			for(j=p_y-k_r;j<=p_y+k_r;j++){ //LOOP2
//				unsigned char in=mat_in[i][j];
//				float kernelin=kernel[i-offset_x][j-offset_y];
//				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				temp+=kernelin * in;
//				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				faultdet_vals[loop1ctr]+=in;
//				loop2ctr++;
//			}
//			loop1ctr++;
//		}
//	}
//
//	if (executionId>=0) {
//		FAULTDET_testing_manual_compare_n_result(offset_x*(IMG_HEIGHT-k_r*2)+offset_y, temp);
//	} else if (executionId==-1) {
//		FAULTDET_testing_manual_result(temp, injectingErrors);
//	}
//
//	horizontalAccumulate=!horizontalAccumulate;
//	oldtemp=temp;
//
//	return temp;
//}


static int convolution2D(int p_x, int p_y, int executionId){
	int k_r,offset_x,offset_y,i,j;
	float temp;

	/*Kernel radius*/
	k_r=KERNEL_SIZE/2;


	//	FAULTDET_testing_injectFault32(p_x, executionId, 32*0, injectingErrors);
	//	FAULTDET_testing_injectFault32(p_y, executionId, 32*1, injectingErrors);
	//	FAULTDET_testing_injectFault32(k_r, executionId, 32*0, injectingErrors);

	/*kernel can be superimposed? if not we are on borders, then we keep the values unchanged*/

	if(p_x-k_r<0 || p_y-k_r<0 || p_x+k_r>=IMG_HEIGHT || p_y+k_r>=IMG_WIDTH){
		unsigned char res=mat_in[p_x][p_y];
		//		FAULTDET_testing_injectFault8(res, executionId, 32*1, injectingErrors);
		return res;
	}

	/*offset between kernel's indexes and array's ones*/
	offset_x=p_x-k_r;
	offset_y=p_y-k_r;

	//	FAULTDET_testing_injectFault32(offset_x, executionId, 32*1+8*1, injectingErrors);
	//	FAULTDET_testing_injectFault32(offset_y, executionId, 32*2+8*1, injectingErrors);

	temp=0;


	/*    for(i=p_x-k_r;i<=p_x+k_r;i++){
        for(j=p_y-k_r;j<=p_y+k_r;j++){
        	unsigned char in=mat_in[i+j][j];
            temp+=kernel[i-offset_x+j-offset_y][j-offset_y] * in;
            faultdet_vals[ctr]+=in;
        }
        ctr++;
    }*/

	if (horizontalAccumulate) {
		int loop1ctr=0;
		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1
			int loop2ctr=0;
			for(j=p_y-k_r;j<=p_y+k_r;j++){ //LOOP2
				unsigned char in=mat_in[i][j];
				float kernelin=kernel[i-offset_x][j-offset_y];
				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				temp+=kernelin * in;
				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				faultdet_vals[loop1ctr]-=in;
				loop2ctr++;
			}
			loop1ctr++;
		}

		float diff=temp>oldtemp ? temp-oldtemp : oldtemp-temp;

		int currVecSize=KERNEL_SIZE;
		while (currVecSize>5) {
			for (int i=0; i<KERNEL_SIZE-1; i+=2) {
				faultdet_vals[currVecSize]=faultdet_vals[currVecSize+1];
				currVecSize--;
				if (currVecSize<=5)
					break;
			}
		}

		if (train) {
			FAULTDET_trainPoint(
					p_x*IMG_HEIGHT+p_y,
					0,  //checkId
					6,
					&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(diff));
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					p_x*IMG_HEIGHT+p_y/*p_x*IMG_WIDTH+p_y*/,
					0, //checkId
#ifdef detectionPerformanceMeasurement
					injectingErrors,
					-1,
					-1,
					executionId,
#endif
					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(diff));
			//			uniIdCtr++;
		}


	} else {
		for (int i=0; i<KERNEL_SIZE; i++) {
			faultdet_vals[i]=0;
		}

		int loop1ctr=0;
		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1
			int loop2ctr=0;
			for(j=p_y-k_r;j<=p_y+k_r;j++){ //LOOP2
				unsigned char in=mat_in[i][j];
				float kernelin=kernel[i-offset_x][j-offset_y];
				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				temp+=kernelin * in;
				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
				faultdet_vals[loop1ctr]+=in;
				loop2ctr++;
			}
			loop1ctr++;
		}
	}

	if (executionId>=0) {
		FAULTDET_testing_manual_compare_n_result(offset_x*(IMG_HEIGHT-k_r*2)+offset_y, temp);
	} else if (executionId==-1) {
		FAULTDET_testing_manual_result(temp, injectingErrors);
	}

	horizontalAccumulate=!horizontalAccumulate;
	oldtemp=temp;

	return temp;
}


/**
 * @brief Actual gaussian filter implementation
 *
 */
static void gauss_filter_routine(int executionId, int i, int j){

	//	uniIdCtr=0;

	//	int i,j;
	if (executionId<-1)
		train=0xFF;
	else
		train=0x0;

	if (executionId>=0)
		injectingErrors=0xFF;
	else
		injectingErrors=0x0;

	if (executionId>=0) {
		//		char oldAcc=horizontalAccumulate;
		//		int k_r=KERNEL_SIZE/2;
		//		if(p_x-k_r<0 || p_y-k_r<0 || p_x+k_r>=IMG_HEIGHT || p_y+k_r>=IMG_WIDTH){
		//			return;
		//		}

		if (horizontalAccumulate) {
			int prevj=j-1;
			int previ=i;
			if (prevj<KERNEL_SIZE/2) {
				prevj=IMG_WIDTH-KERNEL_SIZE/2-1;
				previ--;
			}
			if (previ<KERNEL_SIZE/2) {
				printf("ERROR OUT OF BOUND");
			}
			horizontalAccumulate=0x0;
			mat_out[previ][prevj]=convolution2D(previ, prevj, CONVOLUTIONTOTAL+10000); //to avoid inject fault in the prev execution
			mat_out[i][j]=convolution2D(i,j, executionId);
		} else {
			int nextj=j+1;
			int nexti=i;
			if (nextj>=IMG_WIDTH-KERNEL_SIZE/2) {
				nextj=KERNEL_SIZE/2;
				nexti++;
			}
			if (nexti>=IMG_HEIGHT-KERNEL_SIZE/2) {
				printf("ERROR OUT OF BOUND");
			}
			mat_out[i][j]=convolution2D(i,j, executionId);
			mat_out[nexti][nextj]=convolution2D(nexti, nextj, CONVOLUTIONTOTAL+10000); //to avoid inject fault in the prev execution
			horizontalAccumulate=0xFF;
		}
		//		horizontalAccumulate=!oldAcc;
	} else {
		horizontalAccumulate=0x0;

		for(i=KERNEL_SIZE/2;i<IMG_HEIGHT-KERNEL_SIZE/2;i++){
			for(j=KERNEL_SIZE/2;j<IMG_WIDTH-KERNEL_SIZE/2;j++){
				//			if (executionId==95203) {
				//				printf("pippo");
				//				printf("i: %d, j: %d\n", i, j);
				//			}

				mat_out[i][j]=convolution2D(i,j, executionId);
			}
		}

		if (horizontalAccumulate) {
			printf("ERROR, HEIGHT OR WIDTH MUST BE EVEN\n");
		}
	}

	if (!train) {
		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
	}
}



//float oldtemp;
//static int convolution2D(int p_x, int p_y, int executionId){
//	int k_r,offset_x,offset_y,i,j;
//	float temp, diff;
//
//	/*Kernel radius*/
//	k_r=KERNEL_SIZE/2;
//
//
//	//	FAULTDET_testing_injectFault32(p_x, executionId, 32*0, injectingErrors);
//	//	FAULTDET_testing_injectFault32(p_y, executionId, 32*1, injectingErrors);
//	//	FAULTDET_testing_injectFault32(k_r, executionId, 32*0, injectingErrors);
//
//	/*kernel can be superimposed? if not we are on borders, then we keep the values unchanged*/
//
//	//	if(p_x-k_r<0 || p_y-k_r<0 || p_x+k_r>=IMG_HEIGHT || p_y+k_r>=IMG_WIDTH){
//	//		unsigned char res=mat_in[p_x][p_y];
//	//		//		FAULTDET_testing_injectFault8(res, executionId, 32*1, injectingErrors);
//	//		return res;
//	//	}
//
//	/*offset between kernel's indexes and array's ones*/
//	offset_x=p_x-k_r;
//	offset_y=p_y-k_r;
//
//	//	FAULTDET_testing_injectFault32(offset_x, executionId, 32*1+8*1, injectingErrors);
//	//	FAULTDET_testing_injectFault32(offset_y, executionId, 32*2+8*1, injectingErrors);
//
//	temp=0;
//
//	/*    for(i=p_x-k_r;i<=p_x+k_r;i++){
//        for(j=p_y-k_r;j<=p_y+k_r;j++){
//        	unsigned char in=mat_in[i+j][j];
//            temp+=kernel[i-offset_x+j-offset_y][j-offset_y] * in;
//            faultdet_vals[ctr]+=in;
//        }
//        ctr++;
//    }*/
//
//	for (int i=0; i<KERNEL_SIZE; i++) {
//		faultdet_vals[i]=0;
//	}
//	unsigned int checkId;
//	if (horizontalAccumulate) {
////		checkId=1;
//		int loop1ctr=0;
//		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1TOTAL
//			int loop2ctr=0;
//			for(j=p_y-k_r;j<=p_y+k_r;j++) { //LOOP2TOTAL
//				unsigned char in=mat_in[i][j];
//				float kernelin=kernel[i-offset_x][j-offset_y];
//				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				temp+=kernelin * in;
//				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				faultdet_vals[loop2ctr]+=in;
//				loop2ctr++;
//			}
//			loop1ctr++;
//		}
//	} else {
////		checkId=0;
//		int loop1ctr=0;
//		for(i=p_x-k_r;i<=p_x+k_r;i++){ //LOOP1
//			int loop2ctr=0;
//			for(j=p_y-k_r;j<=p_y+k_r;j++){ //LOOP2
//				unsigned char in=mat_in[i][j];
//				float kernelin=kernel[i-offset_x][j-offset_y];
//				FAULTDET_testing_injectFault8(in, executionId, LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				FAULTDET_testing_injectFault32(kernelin, executionId, 8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				temp+=kernelin * in;
//				FAULTDET_testing_injectFault32(temp, executionId, 32*1+8*1+LOOP1TOTAL*loop1ctr+LOOP2TOTAL*loop2ctr, injectingErrors);
//				faultdet_vals[loop1ctr]+=in;
//				loop2ctr++;
//			}
//			loop1ctr++;
//		}
//	}
//
//	int currVecSize=KERNEL_SIZE;
//	while (currVecSize>5) {
//		for (int i=0; i<KERNEL_SIZE-1; i+=2) {
//			faultdet_vals[currVecSize]=faultdet_vals[currVecSize+1];
//			currVecSize--;
//			if (currVecSize<=5)
//				break;
//		}
//	}
//
//	if (offset_x!=0)
//		diff= temp>oldtemp ? temp-oldtemp : oldtemp-temp;
//	else diff=0;
//
//	if (train) {
//		FAULTDET_trainPoint(
//				p_x*IMG_HEIGHT+p_y,
//				0,  //checkId
//				7,
//				&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(temp), &(diff));
//	} else {
//		FAULTDET_testPoint(
//#ifndef FAULTDETECTOR_EXECINSW
//				&inst,
//#endif
//				p_x*IMG_HEIGHT+p_y/*p_x*IMG_WIDTH+p_y*/,
//				0, //checkId
//#ifdef detectionPerformanceMeasurement
//				injectingErrors,
//				5,
//				5,
//				executionId,
//#endif
//				7, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
//				&(faultdet_vals[0]), &(faultdet_vals[1]), &(faultdet_vals[2]), &(faultdet_vals[3]), &(faultdet_vals[4]), &(temp), &(diff));
//		//			uniIdCtr++;
//	}
//
//	oldtemp=temp;
//
//	horizontalAccumulate=!horizontalAccumulate;
//	return temp;
//}
//
//
///**
// * @brief Actual gaussian filter implementation
// *
// */
//static void gauss_filter_routine(int executionId, int i, int j){
//
//	//	uniIdCtr=0;
//
//	//	int i,j;
//	if (executionId<-1)
//		train=0xFF;
//	else
//		train=0x0;
//
//	oldtemp=0;
//
//	if (executionId>=0)
//		injectingErrors=0xFF;
//	else
//		injectingErrors=0x0;
//
//	if (executionId>=0) {
//		//		char oldAcc=horizontalAccumulate;
//		//		int k_r=KERNEL_SIZE/2;
//		//		if(p_x-k_r<0 || p_y-k_r<0 || p_x+k_r>=IMG_HEIGHT || p_y+k_r>=IMG_WIDTH){
//		//			return;
//		//		}
//
//		mat_out[i][j]=convolution2D(i,j, executionId);
//	} else {
//		horizontalAccumulate=0x0;
//
//		for(i=KERNEL_SIZE/2;i<IMG_HEIGHT-KERNEL_SIZE/2;i++){
//			for(j=KERNEL_SIZE/2;j<IMG_WIDTH-KERNEL_SIZE/2;j++){
//				//			if (executionId==95203) {
//				//				printf("pippo");
//				//				printf("i: %d, j: %d\n", i, j);
//				//			}
//
//				mat_out[i][j]=convolution2D(i,j, executionId);
//			}
//		}
//	}
//
//	if (!train) {
//		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
//	}
//}




void init_img_matrix() {
	for (int i = 0; i < IMG_HEIGHT; i++){
		for (int j = 0; j < IMG_WIDTH; j++){
			mat_in[i][j]=random_get()*256;
		}
	}
}


/**
 * @brief It performs gaussian filtering on a random grayscale image . The execution time is measured through user defined MEASURE_START()/MEASURE_STOP() macros.
 */
//void gauss_filter(int executionId){
//	int i;
//	int j;
//
//	for (i = 0; i < IMG_HEIGHT; i++){
//		for (j = 0; j < IMG_WIDTH; j++){
//			mat_in[i][j]=random_get()*256;
//			mat_in[i+IMG_HEIGHT][j]=mat_in[i][j];
//		}
//	}
//	/*kernel initialization*/
//	gaussian_kernel_init();
//
//	gauss_filter_routine(executionId);
//
//}
#endif

#ifdef ANNBench

#include "simple_random.h"

#include <math.h>

#define IN_NODES 4

#define HIDDEN_NODES 2
#define HIDDEN_LAYERS 4

#define OUT_NODES 1
#define LR 0.1
#define NN_EPOCH 1

#define ARRAY_LENGTH 8
#define BURST_LENGTH 1

static float test_in[BURST_LENGTH][IN_NODES];
static float test_out[BURST_LENGTH][OUT_NODES];

static float train_in[ARRAY_LENGTH][IN_NODES];
static float train_out[ARRAY_LENGTH][OUT_NODES];
static float net_out[OUT_NODES];

static float in_weight[IN_NODES][HIDDEN_NODES];

static float hl_weight[HIDDEN_LAYERS][HIDDEN_NODES][HIDDEN_NODES];
static float hl_bias[HIDDEN_LAYERS][HIDDEN_NODES];

static float out_weight[HIDDEN_NODES][OUT_NODES];
static float out_bias[OUT_NODES];

static float temp_out[HIDDEN_LAYERS][HIDDEN_NODES];
static float delta_out[OUT_NODES];
static float delta_hidden[HIDDEN_LAYERS][HIDDEN_NODES];

static float sigmoid(float x){
	return 1/(1+exp(-x));
}

static float sigmoid_fault(float x, int executionId){
	FAULTDET_testing_injectFault32(x, executionId, 32*0, injectingErrors);
	float res=1/(1+exp(-x));
	FAULTDET_testing_injectFault32(res, executionId, 32*1, injectingErrors);
	return res;
}
static float d_sigmoid(float x){
	return x*(1-x);
}
static void init_train_data(){
	int i;
	for (i = 0; i < ARRAY_LENGTH; i++){
		for (int a=0; a<IN_NODES;a++) {
			train_in[i][a]=random_get();
		}
	}
	for (i = 0; i < ARRAY_LENGTH; i++){
		for (int a=0; a<OUT_NODES;a++) {
			train_out[i][a]=random_get();
		}
	}
}

static void init_weights(){
	int i,h,l;

	for(i=0;i<IN_NODES;i++){
		for ( h = 0; h < HIDDEN_NODES; h++){
			in_weight[i][h]=random_get();
		}

	}
	for(l=0;l<HIDDEN_LAYERS;l++){
		for ( h = 0; h < HIDDEN_NODES; h++){
			hl_bias[l][h]=random_get();
			for(i=0;i<HIDDEN_NODES;i++){
				hl_weight[l][h][i]=random_get();
			}
		}

	}

	for(i=0;i<OUT_NODES;i++){
		out_bias[i]=random_get();
		for ( h = 0; h < HIDDEN_NODES; h++){
			out_weight[h][i]=random_get();
		}
	}

}
static void forward_pass(int train_idx){
	int h,l,y;

	for(h=0;h<HIDDEN_NODES;h++){
		int x;
		float activation;

		activation=hl_bias[0][h];
		for(x=0;x<IN_NODES;x++){
			activation+=(in_weight[x][h]*train_in[train_idx][x]);
		}
		temp_out[0][h]=sigmoid(activation);
	}
	for(l=1;l<HIDDEN_LAYERS;l++){
		for(h=0;h<HIDDEN_NODES;h++){
			float activation;
			int x;

			activation=hl_bias[l][h];
			for(x=0;x<HIDDEN_NODES;x++){
				activation+=(hl_weight[l][h][x]*temp_out[l-1][h]);
			}
			temp_out[l][h]=sigmoid(activation);
		}
	}
	for(y=0;y<OUT_NODES;y++){
		float activation;

		activation=out_bias[y];
		for(h=0;h<HIDDEN_NODES;h++){
			activation+=(out_weight[h][y]*temp_out[HIDDEN_LAYERS-1][h]);
		}
		net_out[y]=sigmoid(activation);
	}
}

#define LOOP3TOTAL (32*3)
#define LOOP2TOTAL (64+LOOP3TOTAL*IN_NODES)

#define LOOP6TOTAL (32*3)
#define LOOP5TOTAL (64+LOOP6TOTAL*HIDDEN_NODES)
#define LOOP4TOTAL (LOOP5TOTAL*HIDDEN_NODES)

#define LOOP8TOTAL (32*3)
#define LOOP7TOTAL (64+LOOP8TOTAL*HIDDEN_NODES)

#define LOOP1TOTAL (LOOP2TOTAL*HIDDEN_NODES+LOOP4TOTAL*(HIDDEN_LAYERS-1)+LOOP7TOTAL*OUT_NODES)

static void forward_pass_test_burst(int executionId){
	int h,l,y;

	if (executionId>=0)
		injectingErrors=0xFF;
	else
		injectingErrors=0x0;

	for (int b=0; b<BURST_LENGTH; b++) { //LOOP1

		for(h=0;h<HIDDEN_NODES;h++){ //LOOP2
			int x;
			float activation;

			activation=hl_bias[0][h];

			for(x=0;x<IN_NODES;x++){ //LOOP3
				float v1=in_weight[x][h];
				float v2=test_in[b][x];
				FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);
				FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);

				FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);

				activation+=(v1*v2);
			}
			temp_out[0][h]=sigmoid_fault(activation, executionId - (LOOP3TOTAL*IN_NODES+LOOP2TOTAL*h+LOOP1TOTAL*b)); //64
		}

		for(l=1;l<HIDDEN_LAYERS;l++){ //LOOP4
			for(h=0;h<HIDDEN_NODES;h++){ //LOOP5
				float activation;
				int x;

				activation=hl_bias[l][h];

				for(x=0;x<HIDDEN_NODES;x++){ //LOOP6
					float v1=hl_weight[l][h][x];
					float v2=temp_out[l-1][h];

					FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP6TOTAL*x+LOOP4TOTAL*(l-1)+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);
					FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP6TOTAL*x+LOOP4TOTAL*(l-1)+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

					FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP6TOTAL*x+LOOP4TOTAL*(l-1)+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

					activation+=(v1*v2);

				}
				temp_out[l][h]=sigmoid_fault(activation, executionId - (LOOP6TOTAL*HIDDEN_NODES+LOOP4TOTAL*(l-1)+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b));
			}
		}

		for(y=0;y<OUT_NODES;y++){ //LOOP7
			float activation;

			activation=out_bias[y];

			for(h=0;h<HIDDEN_NODES;h++){ //LOOP8
				float v1=out_weight[h][y];
				float v2=temp_out[HIDDEN_LAYERS-1][h];
				FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*(HIDDEN_LAYERS-1)+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);
				FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*(HIDDEN_LAYERS-1)+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

				FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*(HIDDEN_LAYERS-1)+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

				activation+=(v1*v2);
			}
			test_out[b][y]=sigmoid_fault(activation, executionId - (LOOP8TOTAL*HIDDEN_NODES+LOOP7TOTAL*y+LOOP4TOTAL*(HIDDEN_LAYERS-1)+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b));
		}

		if (executionId<-1) {
			FAULTDET_trainPoint(
					b,
					0,  //ceckId
					5,
					&(test_in[b][0]), &(test_in[b][1]), &(test_in[b][2]), &(test_in[b][3]), &(test_out[b][0]));
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					b, //uniId
					0, //checkId
#ifdef detectionPerformanceMeasurement
					injectingErrors,
					4,
					4,
					executionId,
#endif
					5, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					&(test_in[b][0]), &(test_in[b][1]), &(test_in[b][2]), &(test_in[b][3]), &(test_out[b][0]));
		}

		if (executionId>=-1) {
			FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
		}
	}
}

static void back_propagation(int train_idx){
	int y,h,l,x;
	/*Compute deltas for OUTPUT LAYER*/
	for(y=0;y<OUT_NODES;y++){
		delta_out[y] = (train_out[train_idx][y]-net_out[y])*d_sigmoid(net_out[y]);
	}
	/* Compute deltas for HIDDEN LAYER */
	for(h=0;h<HIDDEN_NODES;h++){
		float d_error;

		d_error=0;
		for(y=0;y<OUT_NODES;y++){
			d_error+=delta_out[y]*out_weight[h][y];
		}
		delta_hidden[HIDDEN_LAYERS-1][h]=d_error*sigmoid(temp_out[HIDDEN_LAYERS-1][h]);
	}
	for(l=HIDDEN_NODES-2;l>=0;l--){
		for(h=0;h<HIDDEN_NODES;h++){
			float d_error;

			d_error=0;
			for(y=0;y<HIDDEN_NODES;y++){
				d_error+=delta_hidden[l+1][y]*hl_weight[l][h][y];
			}
			delta_hidden[l][h]=d_error*sigmoid(temp_out[l][h]);
		}
	}

	/*Update weights*/
	for(y=0;y<OUT_NODES;y++){
		out_bias[y]+=delta_out[y]*LR;
		for(h=0;h<HIDDEN_NODES;h++){
			out_weight[h][y]+=temp_out[HIDDEN_LAYERS-1][h]*delta_out[y]*LR;
		}
	}
	for(l=HIDDEN_NODES-2;l>0;l--){
		for(h=0;h<HIDDEN_NODES;h++){
			hl_bias[l][h]+=delta_hidden[l][h]*LR;
			for(x=0;x<IN_NODES;x++){
				hl_weight[l][h][x]+=temp_out[l-1][x]*delta_hidden[l][h]*LR;
			}
		}
	}

	for(h=0;h<HIDDEN_NODES;h++){
		hl_bias[0][h]+=delta_hidden[0][h]*LR;
		for(x=0;x<IN_NODES;x++){
			in_weight[x][h]+=train_in[train_idx][x]*delta_hidden[0][h]*LR;
		}
	}

}

static void init_test_data(){
	for (int i=0; i<BURST_LENGTH; i++) {
		for (int a=0; a<IN_NODES;a++) {
			test_in[i][a]=random_get();
		}
	}
}

static void train_ann_routine(){
	int i,j;

	init_weights();
	for(i=0; i<NN_EPOCH;i++){

		for(j=0; j<ARRAY_LENGTH; j++){
			forward_pass(j);

			back_propagation(j);
		}

	}
}
#endif

#ifdef FFTBench

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FFT_LENGTH 512
#define CHECKPERIODICITY 16

typedef struct{
	float re,im;
} complex;

static complex array_in[FFT_LENGTH];
static complex array_out[FFT_LENGTH];



/**
 * @brief It performs sum between two complex numbers
 *
 * @param a first complex number
 * @param b second complex number
 * @return complex sum of a and b
 */

//192
static complex complex_sum(complex a, complex b, int executionId){
	FAULTDET_testing_injectFault32(a.re, executionId, 32*0, injectingErrors);
	FAULTDET_testing_injectFault32(a.im, executionId, 32*1, injectingErrors);
	FAULTDET_testing_injectFault32(b.re, executionId, 32*2, injectingErrors);
	FAULTDET_testing_injectFault32(b.im, executionId, 32*3, injectingErrors);

	complex res;
	res.re=a.re + b.re;
	res.im=a.im + b.im;

	FAULTDET_testing_injectFault32(res.re, executionId, 32*4, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*5, injectingErrors);

	return res;
}
/**
 * @brief It performs multiplication between two complex numbers
 *
 * @param a first complex number
 * @param b second complex number
 * @return complex mult of a and b
 */

//192
static complex complex_mult(complex a, complex b, int executionId){
	FAULTDET_testing_injectFault32(a.re, executionId, 32*0, injectingErrors);
	FAULTDET_testing_injectFault32(a.im, executionId, 32*1, injectingErrors);
	FAULTDET_testing_injectFault32(b.re, executionId, 32*2, injectingErrors);
	FAULTDET_testing_injectFault32(b.im, executionId, 32*3, injectingErrors);

	complex res;
	res.re=(a.re * b.re) - (a.im*b.im);
	res.im=(a.im*b.re) + (a.re*b.im);

	FAULTDET_testing_injectFault32(res.re, executionId, 32*4, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*5, injectingErrors);

	return res;
}
/**
 * @brief It translates exponential form to Re and Im components
 *
 * @param x exponent
 * @return Re and Im components
 */

//96
static complex complex_exp(float x, int executionId){
	/* e^(i*x)=cos(x) + i*sin(x)*/
	FAULTDET_testing_injectFault32(x, executionId, 32*0, injectingErrors);

	complex res;
	res.re=cos(x);
	res.im=sin(x);

	FAULTDET_testing_injectFault32(res.re, executionId, 32*1, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*2, injectingErrors);

	return res;
}

/**
 * @brief Actual fft implementation using Cooley–Tukey algorithm (radix-2 DIT case)
 *
 * @return Fourier transform for input array
 */
#define checksNum FAULTDETECTOR_MAX_CHECKS
int checks_idx[checksNum-1];

#define LOOP2TOTAL (192*2+96+32)
#define LOOP3TOTAL (LOOP2TOTAL)
#define LOOP1TOTAL (((int)((FFT_LENGTH-1)/2))*LOOP3TOTAL+((int)(FFT_LENGTH/2))*LOOP2TOTAL)

complex odd_sum_golden[FFT_LENGTH];
complex even_sum_golden[FFT_LENGTH];

static void fft_routine(int executionId, int k){
	FAULTDET_testing_temp_faultdetected=0;


	if (executionId>=0)
		injectingErrors=0xFF;
	else
		injectingErrors=0x0;

	if (executionId>=0) {

		//		for(k=0;k<FFT_LENGTH;k++){ //LOOP1TOTAL

		/*X_k=[sum{0,N/2-1} x_2n * e^(i*(-2*pi*2n*k)/N)] + [sum{0,N/2-1} x_(2n+1) * e^(i*(-2*pi*(2n+1)*k)/N)]*/
		int n;
		complex even_sum,odd_sum;

		float v1=0;
		float v2=0;
		float v3=0;
		float v4=0;

		even_sum.re=0;
		even_sum.im=0;

		int idx=0;
		int mul=0;
		complex tmp;
		tmp.im=0;
		tmp.re=0;

		if (executionId<LOOP1TOTAL/2) {
			for(n=0;n<FFT_LENGTH && !FAULTDET_testing_temp_faultdetected;n=n+2){ //LOOP2TOTAL
				complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //96
				complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				tmp=complex_sum(tmp, n_term,  executionId - (96+192+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				FAULTDET_testing_injectFault32(tmp, executionId, 32*0+(96+192+192+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/), injectingErrors);

				//			if (isnan(v2))
				//				printf("isnan");

				if (idx<CHECKPERIODICITY/2) {
					v1+=array_in[n].re;
					v3+=array_in[n].im;
					idx++;
				} else if (idx==CHECKPERIODICITY-1) {
					v2+=array_in[n].re;
					v4+=array_in[n].im;

					mul=n*k;
					int chkid=checksNum-1;
					for (int i=0; i<checksNum-1; i++) {
						if (mul<=checks_idx[i]){
							chkid=i;
							break;
						}
					}

					//				int out0=((n+2)>=FFT_LENGTH) ? 4 : -1;
					//				int out1=((n+2)>=FFT_LENGTH) ? 5 : -1;

					//					if (isnan(v2))
					//						printf("isnan");

					if (executionId<-1) {
						FAULTDET_trainPoint(
								FFT_LENGTH*k+n,
								chkid,  //checkId
								6,
								&(v1), &(v2), &(v3), &(v4), &(tmp.re),  &(tmp.im));
					} else {
						FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
								&inst,
#endif
								FFT_LENGTH*k+n, //uniId
								chkid, //checkId
#ifdef detectionPerformanceMeasurement
								injectingErrors,
								-1,
								-1,
								executionId,
#endif
								6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
								&(v1), &(v2), &(v3), &(v4), &(tmp.re), &(tmp.im));
					}

					even_sum=complex_sum(even_sum,tmp, -1); //192

					idx=0;
					tmp.im=0;
					tmp.re=0;

					v1=0;
					v2=0;
					v3=0;
					v4=0;
				} else {
					v2+=array_in[n].re;
					v4+=array_in[n].im;
					idx++;
				}
			}
		} else {
			even_sum=even_sum_golden[k];
		}

		odd_sum.re=0;
		odd_sum.im=0;

		idx=0;
		mul=0;

		tmp.im=0;
		tmp.re=0;
		if (executionId>=LOOP1TOTAL/2) {
			for(n=1;n<FFT_LENGTH && !FAULTDET_testing_temp_faultdetected;n=n+2){ //LOOP3TOTAL
				complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/) ); //96
				complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/) ); //192

				tmp=complex_sum(tmp,n_term,  executionId - (192+96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				FAULTDET_testing_injectFault32(tmp, executionId, (192+192+96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/), injectingErrors);



				if (idx<CHECKPERIODICITY/2) {
					v1+=array_in[n].re;
					v3+=array_in[n].im;
					idx++;
				} else if (idx==CHECKPERIODICITY-1) {
					v2+=array_in[n].re;
					v4+=array_in[n].im;

					mul=n*k;
					int chkid=checksNum-1;
					for (int i=0; i<checksNum-1; i++) {
						if (mul<=checks_idx[i]){
							chkid=i;
							break;
						}
					}

					//				if (isnan(v2))
					//					printf("isnan");

					//				int out0=((n+2)>=FFT_LENGTH) ? 4 : -1;
					//				int out1=((n+2)>=FFT_LENGTH) ? 5 : -1;

					if (executionId<-1) {
						FAULTDET_trainPoint(
								FFT_LENGTH*k+n,
								chkid,  //checkId
								6,
								&(v1), &(v2), &(v3), &(v4), &(tmp.re),  &(tmp.im));
					} else {
						FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
								&inst,
#endif
								FFT_LENGTH*k+n, //uniId
								chkid, //checkId
#ifdef detectionPerformanceMeasurement
								injectingErrors,
								-1,
								-1,
								executionId,
#endif
								6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
								&(v1), &(v2), &(v3), &(v4), &(tmp.re), &(tmp.im));
					}

					odd_sum=complex_sum(odd_sum,tmp, -1); //192

					idx=0;
					tmp.im=0;
					tmp.re=0;

					v1=0;
					v2=0;
					v3=0;
					v4=0;
				} else {
					v2+=array_in[n].re;
					v4+=array_in[n].im;
					idx++;
				}
			}
		} else {
			odd_sum=odd_sum_golden[k];
		}

		complex out=complex_sum(even_sum,odd_sum, -10);

		int idxGolden=k*2;
		FAULTDET_testing_manual_compare_n_result(idxGolden, out.re);
		FAULTDET_testing_manual_compare_n_result(idxGolden+1, out.im);

		array_out[k] = out;
		//		}

	} else {


		for(k=0;k<FFT_LENGTH;k++){ //LOOP1TOTAL



			/*X_k=[sum{0,N/2-1} x_2n * e^(i*(-2*pi*2n*k)/N)] + [sum{0,N/2-1} x_(2n+1) * e^(i*(-2*pi*(2n+1)*k)/N)]*/
			int n;
			complex even_sum,odd_sum;

			float v1=0;
			float v2=0;
			float v3=0;
			float v4=0;

			even_sum.re=0;
			even_sum.im=0;


			int idx=0;
			int mul=0;
			complex tmp;
			tmp.im=0;
			tmp.re=0;

			for(n=0;n<FFT_LENGTH;n=n+2){ //LOOP2TOTAL
				complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //96
				complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				tmp=complex_sum(tmp, n_term,  executionId - (96+192+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				FAULTDET_testing_injectFault32(tmp, executionId, 32*0+(96+192+192+((int)(n/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/), injectingErrors);

				//			if (isnan(v2))
				//				printf("isnan");

				if (idx<CHECKPERIODICITY/2) {
					v1+=array_in[n].re;
					v3+=array_in[n].im;
					idx++;
				} else if (idx==CHECKPERIODICITY-1) {
					v2+=array_in[n].re;
					v4+=array_in[n].im;

					mul=n*k;
					int chkid=checksNum-1;
					for (int i=0; i<checksNum-1; i++) {
						if (mul<=checks_idx[i]){
							chkid=i;
							break;
						}
					}

					//				int out0=((n+2)>=FFT_LENGTH) ? 4 : -1;
					//				int out1=((n+2)>=FFT_LENGTH) ? 5 : -1;

					//					if (isnan(v2))
					//						printf("isnan");


					if (executionId<-1) {
						FAULTDET_trainPoint(
								FFT_LENGTH*k+n,
								chkid,  //checkId
								6,
								&(v1), &(v2), &(v3), &(v4), &(tmp.re),  &(tmp.im));
					} else {
						FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
								&inst,
#endif
								k*FFT_LENGTH+n, //uniId
								chkid, //checkId
#ifdef detectionPerformanceMeasurement
								injectingErrors,
								-1,
								-1,
								executionId,
#endif
								6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
								&(v1), &(v2), &(v3), &(v4), &(tmp.re), &(tmp.im));
					}

					even_sum=complex_sum(even_sum,tmp, -1); //192

					idx=0;
					tmp.im=0;
					tmp.re=0;

					v1=0;
					v2=0;
					v3=0;
					v4=0;
				} else {
					v2+=array_in[n].re;
					v4+=array_in[n].im;
					idx++;
				}
			}

			even_sum_golden[k]=even_sum;

			odd_sum.re=0;
			odd_sum.im=0;

			idx=0;
			mul=0;

			tmp.im=0;
			tmp.re=0;
			for(n=1;n<FFT_LENGTH;n=n+2){ //LOOP3TOTAL
				complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/) ); //96
				complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/) ); //192

				tmp=complex_sum(tmp,n_term,  executionId - (192+96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/)); //192

				FAULTDET_testing_injectFault32(tmp, executionId, (192+192+96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL/*+k*LOOP1TOTAL*/), injectingErrors);



				if (idx<CHECKPERIODICITY/2) {
					v1+=array_in[n].re;
					v3+=array_in[n].im;
					idx++;
				} else if (idx==CHECKPERIODICITY-1) {
					v2+=array_in[n].re;
					v4+=array_in[n].im;

					mul=n*k;
					int chkid=checksNum-1;
					for (int i=0; i<checksNum-1; i++) {
						if (mul<=checks_idx[i]){
							chkid=i;
							break;
						}
					}

					//				if (isnan(v2))
					//					printf("isnan");

					//				int out0=((n+2)>=FFT_LENGTH) ? 4 : -1;
					//				int out1=((n+2)>=FFT_LENGTH) ? 5 : -1;

					if (executionId<-1) {
						FAULTDET_trainPoint(
								FFT_LENGTH*k+n,
								chkid,  //checkId
								6,
								&(v1), &(v2), &(v3), &(v4), &(tmp.re),  &(tmp.im));
					} else {
						FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
								&inst,
#endif
								FFT_LENGTH*k+n, //uniId
								chkid, //checkId
#ifdef detectionPerformanceMeasurement
								injectingErrors,
								-1,
								-1,
								executionId,
#endif
								6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
								&(v1), &(v2), &(v3), &(v4), &(tmp.re), &(tmp.im));
					}

					odd_sum=complex_sum(odd_sum,tmp, -1); //192

					idx=0;
					tmp.im=0;
					tmp.re=0;

					v1=0;
					v2=0;
					v3=0;
					v4=0;
				} else {
					v2+=array_in[n].re;
					v4+=array_in[n].im;
					idx++;
				}
			}

			odd_sum_golden[k]=odd_sum;

			complex out=complex_sum(even_sum,odd_sum, -10);

			if (executionId>=-1) {
				FAULTDET_testing_manual_result(out.re, injectingErrors);
				FAULTDET_testing_manual_result(out.im, injectingErrors);
			}

			array_out[k] = out;
		}


	}

	if (executionId>=-1) {
		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
	}
}

//
//#define LOOP2TOTAL (192*2+96)
//#define LOOP3TOTAL (LOOP2TOTAL)
//#define LOOP1TOTAL (32*4+((int)((FFT_LENGTH-1)/2))*LOOP3TOTAL+((int)(FFT_LENGTH/2))*LOOP2TOTAL)
//
//static void fft_routine(int executionId){
//	int k;
//
//	if (executionId>=0)
//		injectingErrors=0xFF;
//	else
//		injectingErrors=0x0;
//
//	for(k=0;k<FFT_LENGTH;k++){ //LOOP1TOTAL
//
//
//		/*X_k=[sum{0,N/2-1} x_2n * e^(i*(-2*pi*2n*k)/N)] + [sum{0,N/2-1} x_(2n+1) * e^(i*(-2*pi*(2n+1)*k)/N)]*/
//		int n;
//		complex even_sum,odd_sum;
//
//		even_sum.re=0;
//		even_sum.im=0;
//
//		for(n=0;n<FFT_LENGTH;n=n+2){ //LOOP2TOTAL
//			complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)(n/2))*LOOP2TOTAL+k*LOOP1TOTAL)); //96
//			complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)(n/2))*LOOP2TOTAL+k*LOOP1TOTAL)); //192
//
//			complex_sum(even_sum,n_term,  executionId - (96+192+((int)(n/2))*LOOP2TOTAL+k*LOOP1TOTAL)); //192
//		}
//		FAULTDET_testing_injectFault32(even_sum.re, executionId, 32*0+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL, injectingErrors);
//		FAULTDET_testing_injectFault32(even_sum.im, executionId, 32*1+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL, injectingErrors);
//
//		float v1=0;
//		float v2=0;
//		float v3=0;
//		float v4=0;
//		for (int i=0; i<FFT_LENGTH; i+=8) {
//			int i1=i+2;
//			int i2=i+4;
//			int i3=i+6;
//			v1+=array_in[i].re+array_in[i1].re;
//			v2+=array_in[i2].re+array_in[i3].re;
//			v3+=array_in[i].im+array_in[i1].im;
//			v4+=array_in[i2].im+array_in[i3].im;
//		}
//
//		if (executionId<-1) {
//			FAULTDET_trainPoint(
//					1,
//					k,  //checkId
//					6,
//					&(v1), &(v2), &(v3), &(v4), &(even_sum.re),  &(even_sum.im));
//		} else {
//			FAULTDET_testPoint(
//#ifndef FAULTDETECTOR_EXECINSW
//					&inst,
//#endif
//					1, //uniId
//					k, //checkId
//#ifdef detectionPerformanceMeasurement
//					injectingErrors,
//					4,
//					5,
//					executionId,
//#endif
//					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
//					&(v1), &(v2), &(v3), &(v4), &(even_sum.re),  &(even_sum.im));
//		}
//
//
//		odd_sum.re=0;
//		odd_sum.im=0;
//
//		for(n=1;n<FFT_LENGTH;n=n+2){ //LOOP3TOTAL
//			complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL) ); //96
//			complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL) ); //192
//
//			complex_sum(odd_sum,n_term,  executionId - (192+96+((int)((n-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL)); //192
//		}
//
//		FAULTDET_testing_injectFault32(odd_sum.re, executionId, (((int)((FFT_LENGTH-1)/2))*LOOP3TOTAL+32*2+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL), injectingErrors);
//		FAULTDET_testing_injectFault32(odd_sum.im, executionId, (((int)((FFT_LENGTH-1)/2))*LOOP3TOTAL+32*3+((int)(FFT_LENGTH/2))*LOOP2TOTAL+k*LOOP1TOTAL), injectingErrors);
//
//		v1=0;
//		v2=0;
//		v3=0;
//		v4=0;
//		for (int i=1; i<FFT_LENGTH; i+=8) {
//			int i1=i+2;
//			int i2=i+4;
//			int i3=i+6;
//			v1+=array_in[i].re+array_in[i1].re;
//			v2+=array_in[i2].re+array_in[i3].re;
//			v3+=array_in[i].im+array_in[i1].im;
//			v4+=array_in[i2].im+array_in[i3].im;
//		}
//
//		if (executionId<-1) {
//			FAULTDET_trainPoint(
//					1,
//					k,  //checkId
//					6,
//					&(v1), &(v2), &(v3), &(v4), &(odd_sum.re),  &(odd_sum.im));
//		} else {
//			FAULTDET_testPoint(
//#ifndef FAULTDETECTOR_EXECINSW
//					&inst,
//#endif
//					1, //uniId
//					k, //checkId
//#ifdef detectionPerformanceMeasurement
//					injectingErrors,
//					4,
//					5,
//					executionId,
//#endif
//					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
//					&(v1), &(v2), &(v3), &(v4), &(odd_sum.re),  &(odd_sum.im));
//		}
//
//		complex out=complex_sum(even_sum,odd_sum, -10);
//
//		array_out[k] = out;
//	}
//	if (executionId>=-1) {
//		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
//	}
//}

#endif

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

	FAULTDET_testing_injectFault32(pid->p, executionId, 32*0, injectingErrors);
	FAULTDET_testing_injectFault32(pid->i, executionId, 32*1, injectingErrors);
	FAULTDET_testing_injectFault32(pid->d, executionId, 32*2, injectingErrors);
	FAULTDET_testing_injectFault32(pid->b, executionId, 32*3, injectingErrors);
	FAULTDET_testing_injectFault32(pid->prev_error, executionId, 32*4, injectingErrors);
	FAULTDET_testing_injectFault32(pid->integral_sum, executionId, 32*5, injectingErrors);
	FAULTDET_testing_injectFault32(pid->backpropagation, executionId, 32*6, injectingErrors);

	output = error * pid->p;

	FAULTDET_testing_injectFault32(output, executionId, 32*7, injectingErrors);

	pid->integral_sum += ((error * pid->i) + (pid->b * pid->backpropagation)) * TIME_STEP;

	FAULTDET_testing_injectFault32(pid->integral_sum, executionId, 32*8, injectingErrors);

	output += pid->integral_sum;

	FAULTDET_testing_injectFault32(output, executionId, 32*9, injectingErrors);

	output += pid->d * (error - pid->prev_error) / TIME_STEP;

	FAULTDET_testing_injectFault32(output, executionId, 32*10, injectingErrors);

	pid->prev_error = error;

	return output;
}

float roll_limiter(float desired_roll, float speed, int executionId) {
	float limit_perc,limit;

	FAULTDET_testing_injectFault32(speed, executionId, 32*0, injectingErrors);
	FAULTDET_testing_injectFault32(desired_roll, executionId, 32*1, injectingErrors);

	if (speed <= 140) {
		return CLAMP(desired_roll, -30, 30);
	}
	if (speed >= 300) {
		return CLAMP(desired_roll, -40, 40);
	}

	FAULTDET_testing_injectFault32(speed, executionId, 32*2, injectingErrors);

	limit_perc = (speed < 220) ? (speed-140) / 80 : ((speed-220) / 80);

	FAULTDET_testing_injectFault32(limit_perc, executionId, 32*3, injectingErrors);

	limit = (speed < 220) ? (30 + limit_perc * 37) : (40 + (1-limit_perc) * 27);

	FAULTDET_testing_injectFault32(limit, executionId, 32*4, injectingErrors);

	return CLAMP (desired_roll, -limit, limit);
}

float roll_rate_limiter(float desired_roll_rate, float roll, int executionId) {
	FAULTDET_testing_injectFault32(roll, executionId, 32*0, injectingErrors);
	FAULTDET_testing_injectFault32(desired_roll_rate, executionId, 32*1, injectingErrors);


	if (roll < 20 && roll > -20) {
		return CLAMP (desired_roll_rate, -5, 5);
	} else if (roll < 30 && roll > -30) {
		return CLAMP (desired_roll_rate, -3, 3);
	} else {
		return CLAMP (desired_roll_rate, -1, 1);
	}
}

float ailerons_limiter(float aileron, int executionId) {
	FAULTDET_testing_injectFault32(aileron, executionId, 32*0, injectingErrors);

	return CLAMP(aileron, -30, 30);
}

float r1, r2, r3, r4;

void latnav(int executionId) {

#ifndef FAULTDETECTOR_EXECINSW
	FAULTDET_ExecutionDescriptor inst;
	FAULTDET_initFaultDetection(&inst);
#endif

	if (executionId>=0)
		injectingErrors=0xFF;
	else
		injectingErrors=0x0;

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


	//	for(i=0; i<1; i++) {

	float desired_roll,actual_roll,desired_roll_rate,actual_roll_rate,desired_ailerons,actual_ailerons;

	float err=curr_heading-r4;
	float pid_heading_backpropagation_orig=pid_heading.backpropagation;
	float err_orig=err;

	desired_roll = run_pid(&pid_heading, err, executionId);
	actual_roll = roll_limiter(desired_roll, 400, executionId-(32*11));

	FAULTDET_testing_injectFault32(actual_roll, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32*0), injectingErrors);

	//	if (executionId<-1) {
	//		FAULTDET_trainPoint(
	//				1,
	//				0,  //checkId
	//				4,
	//				//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
	//				&(pid_heading.backpropagation), &err, &desired_roll, &actual_roll);//, &actual_roll);
	//
	//	} else {
	//		FAULTDET_testPoint(
	//#ifndef FAULTDETECTOR_EXECINSW
	//				&inst,
	//#endif
	//				1, //uniId
	//				0, //checkId
	//#ifdef detectionPerformanceMeasurement
	//				injectingErrors,
	//				-1,
	//				-1,
	//				executionId,
	//#endif
	//				4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
	//				//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
	//				&(pid_heading.backpropagation), &err, &desired_roll, &actual_roll);//, &actual_roll);
	//	}

	if (executionId<-1) {
		FAULTDET_trainPoint(
				1,
				0,  //checkId
				4,
				//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
				&(curr_heading), &r4, &desired_roll, &actual_roll);//, &actual_roll);

	} else {
		FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
				&inst,
#endif
				1, //uniId
				0, //checkId
#ifdef detectionPerformanceMeasurement
				injectingErrors,
				-1,
				-1,
				executionId,
#endif
				4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
				//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
				&(curr_heading), &r4, &desired_roll, &actual_roll);//, &actual_roll);
	}

	pid_heading.backpropagation = actual_roll - desired_roll;

	float pid_roll_backpropagation_orig=pid_roll.backpropagation;

	float err1=curr_roll - actual_roll;
	float err1_orig=curr_roll - actual_roll;
	desired_roll_rate = run_pid(&pid_roll, err1, executionId-(32*11)-(32*5));

	actual_roll_rate = roll_rate_limiter(desired_roll_rate, curr_roll, executionId-(32*11)-(32*5)-(32*11));
	FAULTDET_testing_injectFault32(actual_roll_rate, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32*1), injectingErrors);


	//	if (executionId<-1) {
	//		FAULTDET_trainPoint(
	//				1,
	//				1,  //ceckId
	//				5,
	//				//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
	//				&(pid_roll.backpropagation), &err1, &desired_roll_rate, &actual_roll_rate, &curr_roll);//, &actual_roll_rate);
	//
	//	} else {
	//		FAULTDET_testPoint(
	//#ifndef FAULTDETECTOR_EXECINSW
	//				&inst,
	//#endif
	//				1, //uniId
	//				1, //checkId
	//#ifdef detectionPerformanceMeasurement
	//				injectingErrors,
	//				-1,
	//				-1,
	//				executionId,
	//#endif
	//				5, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
	//				//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
	//				&(pid_roll.backpropagation), &err1, &desired_roll_rate, &actual_roll_rate, &curr_roll);//, &actual_roll_rate);
	//	}

	pid_roll.backpropagation = actual_roll_rate - desired_roll_rate;
	pid_roll_backpropagation_orig=pid_roll.backpropagation;

	float err2=curr_roll_rate - actual_roll_rate;
	float err2_orig=err2;
	desired_ailerons = run_pid(&pid_roll, err2, executionId-(32*11)-(32*5)-(32*11)-(32*2));
	actual_ailerons = ailerons_limiter(desired_ailerons, executionId-(32*11)-(32*5)-(32*11)-(32*2)-(32*11));

	//	if (executionId<-1) {
	//		FAULTDET_trainPoint(
	//				1,
	//				2,  //checkId
	//				4,
	//				//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
	//				&(pid_roll.backpropagation), &err2, &desired_ailerons, &actual_ailerons);//, &actual_ailerons);
	//
	//	} else {
	//		FAULTDET_testPoint(
	//#ifndef FAULTDETECTOR_EXECINSW
	//				&inst,
	//#endif
	//				1, //uniId
	//				2, //checkId
	//#ifdef detectionPerformanceMeasurement
	//				injectingErrors,
	//				-1,
	//				-1,
	//				executionId,
	//#endif
	//				4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
	//				//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
	//				&(pid_roll.backpropagation), &err2, &desired_ailerons, &actual_ailerons);//, &actual_ailerons);
	//	}

	pid_roll.backpropagation = actual_ailerons - desired_ailerons;


	//	if (executionId<-1) {
	//		FAULTDET_trainPoint(
	//				1,
	//				3,  //checkId
	//				4,
	//				&(curr_heading), &(curr_roll), &curr_roll_rate, &desired_ailerons);
	//	}  else {
	//		FAULTDET_testPoint(
	//#ifndef FAULTDETECTOR_EXECINSW
	//				&inst,
	//#endif
	//				1, //uniId
	//				3, //checkId
	//#ifdef detectionPerformanceMeasurement
	//				injectingErrors,
	//				3,
	//				3,
	//				executionId,
	//#endif
	//				4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
	//				&(curr_heading), &(curr_roll), &curr_roll_rate, &desired_ailerons);
	//	}
	if (executionId<-1) {
		FAULTDET_trainPoint(
				1,
				1,  //checkId
				4,
				&(curr_roll), &(actual_roll_rate), &curr_roll_rate, &desired_ailerons);
	}  else {
		FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
				&inst,
#endif
				1, //uniId
				1, //checkId
#ifdef detectionPerformanceMeasurement
				injectingErrors,
				3,
				3,
				executionId,
#endif
				4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
				&(curr_roll), &(actual_roll_rate), &curr_roll_rate, &desired_ailerons);
	}

	pid_roll.backpropagation = actual_ailerons - desired_ailerons;


	/* Just a random plane model*/
	//		FAULTDET_testing_injectFault32(curr_roll, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)-1, injectingErrors);
	//		FAULTDET_testing_injectFault32(curr_roll_rate, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32)-1,  injectingErrors);
	//		FAULTDET_testing_injectFault32(desired_ailerons, executionId, (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32), (32*11)+(32*5)+(32*11)+(32*2)+(32*11)+(32*1)+(32)+(32)+(32)-1,  injectingErrors);

	//				curr_heading += curr_roll/10 * TIME_STEP;
	//				curr_roll += curr_roll_rate * TIME_STEP;
	//				curr_roll_rate += desired_ailerons / 5;


	//		FAULTDET_testing_injectFault32(curr_heading, executionId, 1408, 1408+32-1, injectingErrors);
	//		FAULTDET_testing_injectFault32(curr_roll, executionId, 1408+32, 1408+32+32-1,  injectingErrors);
	//		FAULTDET_testing_injectFault32(curr_roll_rate, executionId, 1408+32+32, 1408+32+32+32-1 /*1503*/,  injectingErrors);

	//	}

	if (executionId>=-1) {
		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
	}
}

#endif

#include <locale.h>
int main(int argc, char * const argv[])
{
	int executions, regs, trainIter;

	executions=0;
	regs=0;
	trainIter=0;

	char regNext=0x0;
	char trainNext=0x0;
	char executionsNext=0x0;

	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "-r")==0) {
			regNext=0xFF;
		}
		else if (strcmp(argv[i], "-t")==0) {
			trainNext=0xFF;
		}
		else if (strcmp(argv[i], "-e")==0) {
			executionsNext=0xFF;
		}
		else if (regNext) {
			regs=atoi(argv[i]);
			regNext=0x0;
		}
		else if (trainNext) {
			trainIter=atoi(argv[i]);
			trainNext=0x0;
		}
		else if (executionsNext) {
			executions=atoi(argv[i]);
			executionsNext=0x0;
		}
	}

	if (executions==0)
		executions=50000;
	if (regs==0)
		regs=16;
	if (trainIter==0)
		trainIter=50000;

	setlocale(LC_NUMERIC,"en_US.UTF-8");

	printf("{\"regions\":%d, \"trainIterations\":%d, \"testIterations\":%d,  \"relerr\": [", regs, trainIter, executions);

#ifndef dynamicRegions
	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
		n_regions[i]=0;
		for (int j=0; j<FAULTDETECTOR_MAX_REGIONS; j++) {
			for (int k=0; k<FAULTDETECTOR_MAX_AOV_DIM; k++) {
				trainedRegions[i][j].center[k]=0.0;
				trainedRegions[i][j].max[k]=0.0;
				trainedRegions[i][j].min[k]=0.0;
			}
		}
	}
	FAULTDETECTOR_SW_initRegions(trainedRegions, n_regions);
#else
	FAULTDETECTOR_SW_allocRegions(regs);
#endif

	random_set_seed(1);


#ifdef gaussianBench
	gaussian_kernel_init();

	for (int i=-trainIter-1; i<-1; i++) {
		init_img_matrix();
		gauss_filter_routine(i, -1, -1);
	}

	//32*32=1024, ceil(50000/1024)=49
	int NoFp=0;
	for (int iter=0; iter<50000; iter++) {
		init_img_matrix();
		gauss_filter_routine(-1, -1, -1);
		if (!FAULTDET_testing_loggin_faultdetected) {
			while (NoFp<15) {
				for (int executionId=0 ;executionId<CONVOLUTIONTOTAL; executionId++) {
					horizontalAccumulate=0x0;
					for(int i=KERNEL_SIZE/2;i<IMG_HEIGHT-KERNEL_SIZE/2;i++){
						for(int j=KERNEL_SIZE/2;j<IMG_WIDTH-KERNEL_SIZE/2;j++){
							gauss_filter_routine(executionId, i, j);
						}
					}
				}
				NoFp++;
			}
		}

		FAULTDET_testing_resetGoldens();
	}


	//	char pass=0x0;
	//	int i=0;
	//	while(!pass) {
	////	for (int i=0; i<executions; i++) {
	//		init_img_matrix();
	//		gauss_filter_routine(-1);
	//		if (!FAULTDET_testing_loggin_faultdetected) {
	//			pass=0xFF;
	//			for (int executionId=0 ;executionId<CONVOLUTIONTOTAL*IMG_HEIGHT*IMG_WIDTH; executionId++) {
	//				gauss_filter_routine(executionId);
	//			}
	//		}
	//
	//		fflush(stdout);
	//
	//		FAULTDET_testing_resetGoldens();
	//		i++;
	//	}
	//
	//	while (i<CONVOLUTIONTOTAL*IMG_HEIGHT*IMG_WIDTH) {
	//		init_img_matrix();
	//		gauss_filter_routine(-1);
	//		FAULTDET_testing_resetGoldens();
	//		i++;
	//	}

#endif

#ifdef ANNBench

	//	init_test_data();
	//	forward_pass_test_burst(-2);
	init_train_data();
	train_ann_routine();

	for (int i=-trainIter-1; i<-1; i++) {
		init_test_data();
		forward_pass_test_burst(i);
	}

	for (int i=0; i<executions; i++) {
		init_test_data();
		forward_pass_test_burst(-1);
		if (!FAULTDET_testing_loggin_faultdetected) {
			for (int executionId=0 ;executionId<LOOP1TOTAL*BURST_LENGTH; executionId++) {
				forward_pass_test_burst(executionId);
			}
		}
		FAULTDET_testing_resetGoldens();
	}
#endif

#ifdef FFTBench
	int step=FFT_LENGTH*FFT_LENGTH/checksNum;
	int part=0;
	for (int i=0; i<checksNum-1; i++) {
		part+=step;
		checks_idx[i]=part;
	}

	for (int executionId=-trainIter-1; executionId<-1; executionId++) {
		for(int i=0; i<FFT_LENGTH;i++){
			complex x;
			x.re=random_get();
			x.im=random_get();

			array_in[i]=x;
		}
		fft_routine(executionId, 0);
	}

	int NoFp=0;
	for (int i=0; i<50000; i++) {
		//	for (int i=0; i<50; i++) {
		for(int i=0; i<FFT_LENGTH;i++){
			complex x;
			x.re=random_get();
			x.im=random_get();

			array_in[i]=x;
		}
		fft_routine(-1,0);
		if (!FAULTDET_testing_loggin_faultdetected) {
			if (NoFp<3) {
				//start to inject faults
				for (int k=0; k<FFT_LENGTH; k++) {
					for (int executionId=0; executionId<LOOP1TOTAL; executionId++) {
						fft_routine(executionId, k);
					}
				}
				NoFp++;
			}
		}
		FAULTDET_testing_resetGoldens();
	}
#endif

#ifdef latnavBench

	for (int executionId=-trainIter-1 ;executionId<-1/*960*/; executionId++) {
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		latnav(executionId);
	}

	for (int i=0; i<executions; i++) {
		FAULTDET_testing_resetGoldens();
		injectingErrors=0x0;
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		latnav(-1);
		if (!FAULTDET_testing_loggin_faultdetected) {
			for (int executionId=0 ;executionId<1312+32*2; executionId++) {
				latnav(executionId);
			}
		}
	}
#endif
	printf("], ");
	printf("\"total_pos\": %d, ", FAULTDET_testing_getTotal_golden());
	printf("\"true_pos\": %d, ", FAULTDET_testing_getOk_golden());
	printf("\"false_pos\": %d, ", FAULTDET_testing_getFalsePositives_golden());

	printf("\"total_bitflips\":%d, ", FAULTDET_testing_getTotal());
	printf("\"true_neg\": %d, ", FAULTDET_testing_getOk());
	printf("\"false_neg\":%d, ", FAULTDET_testing_getFalseNegatives());
	//	printf("%d|", FAULTDET_testing_getOk_wtolerance());
	//	printf("%d|", FAULTDET_testing_getFalseNegatives_wtolerance());
	printf("\"no_effect_bitflips\": %d", FAULTDET_testing_getNoEffects());
	printf("}");
	fflush(stdout);
	//	printf("\ntotal for fp: %d\n", FAULTDET_testing_getTotal_golden());
	//	printf("ok for fp: %d\n", FAULTDET_testing_getOk_golden());
	//	printf("fp: %d\n", FAULTDET_testing_getFalsePositives_golden());
	//
	//	printf("total for fn: %d\n", FAULTDET_testing_getTotal());
	//	printf("ok for fn: %d\n", FAULTDET_testing_getOk());
	//	printf("fn: %d\n", FAULTDET_testing_getFalseNegatives());
	//	printf("ok for fn with tolerance: %d\n", FAULTDET_testing_getOk_wtolerance());
	//	printf("fn with tolerance: %d\n", FAULTDET_testing_getFalseNegatives_wtolerance());
	//	printf("no effects: %d\n", FAULTDET_testing_getNoEffects());
	FAULTDETECTOR_SW_freeRegions();
}
