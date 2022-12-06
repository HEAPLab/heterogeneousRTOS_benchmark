/*
 ============================================================================
 Name        : faultdetector_benchmark.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
//#define latnavBench
#define ANNBench
#define FAULTDETECTOR_EXECINSW
#define testingCampaign

#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "simple_random.h"
#include "faultdetector_handler.h"
#include <string.h>

u8 injectingErrors=0;
float r1, r2, r3, r4;

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
#define BURST_LENGTH 16

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
	FAULTDET_testing_injectFault32(x, executionId, 32*0, (32*1)-1, injectingErrors);
	float res=1/(1+exp(-x));
	FAULTDET_testing_injectFault32(res, executionId, 32*1, (32*2)-1, injectingErrors);
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
#define LOOP4TOTAL ((LOOP5TOTAL*HIDDEN_NODES)*(HIDDEN_LAYERS-1))

#define LOOP8TOTAL (32*3)
#define LOOP7TOTAL (64+LOOP8TOTAL*HIDDEN_NODES)

#define LOOP1TOTAL (LOOP2TOTAL*HIDDEN_NODES+LOOP4TOTAL*HIDDEN_LAYERS+LOOP7TOTAL*OUT_NODES)

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

			//			FAULTDET_testing_injectFault32(activation, executionId, 32*0+LOOP2TOTAL*h+LOOP1TOTAL*b, (32*1)-1+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);

			for(x=0;x<IN_NODES;x++){ //LOOP3
				float v1=in_weight[x][h];
				float v2=test_in[b][x];
				FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, (32*1)-1+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);
				FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, (32*2)-1+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);

				FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, (32*3)-1+LOOP3TOTAL*x+LOOP2TOTAL*h+LOOP1TOTAL*b, injectingErrors);

				activation+=(v1*v2);
			}
			temp_out[0][h]=sigmoid_fault(activation, executionId - (LOOP3TOTAL*IN_NODES+LOOP2TOTAL*h+LOOP1TOTAL*b)); //64
		}

		for(l=1;l<HIDDEN_LAYERS;l++){ //LOOP4
			for(h=0;h<HIDDEN_NODES;h++){ //LOOP5
				float activation;
				int x;

				activation=hl_bias[l][h];

				//				FAULTDET_testing_injectFault32(activation, executionId, 32*0+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*1-1+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

				for(x=0;x<HIDDEN_NODES;x++){ //LOOP6
					float v1=hl_weight[l][h][x];
					float v2=temp_out[l-1][h];

					FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*1-1+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);
					FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*2-1+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

					FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*3-1+LOOP6TOTAL*x+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

					activation+=(v1*v2);

				}
				temp_out[l][h]=sigmoid_fault(activation, executionId - (LOOP6TOTAL*HIDDEN_NODES+LOOP4TOTAL*l+LOOP5TOTAL*h+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b));
			}
		}

		for(y=0;y<OUT_NODES;y++){ //LOOP7
			float activation;

			activation=out_bias[y];

			//			FAULTDET_testing_injectFault32(activation, executionId, 32*0+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*1-1+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

			for(h=0;h<HIDDEN_NODES;h++){ //LOOP8
				float v1=out_weight[h][y];
				float v2=temp_out[HIDDEN_LAYERS-1][h];
				FAULTDET_testing_injectFault32(v1, executionId, 32*0+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*1-1+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);
				FAULTDET_testing_injectFault32(v2, executionId, 32*1+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*2-1+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

				FAULTDET_testing_injectFault32(activation, executionId, 32*2+LOOP8TOTAL*h+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, 32*3-1+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b, injectingErrors);

				activation+=(v1*v2);
			}
			test_out[b][y]=sigmoid_fault(activation, executionId - (LOOP8TOTAL*HIDDEN_NODES+LOOP7TOTAL*y+LOOP4TOTAL*HIDDEN_LAYERS+LOOP2TOTAL*HIDDEN_NODES+LOOP1TOTAL*b));
		}

		if (executionId<-1) {
			FAULTDET_trainPoint(
					b,
					0,  //ceckId
					5,
					&(test_in[b][0]), &(test_in[b][1]), &(test_in[b][2]), &(test_in[b][3]), &(test_out[b][1]));
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					b, //uniId
					0, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					4,
					4,
					0,
					executionId,
#endif
					5, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					&(test_in[b][0]), &(test_in[b][1]), &(test_in[b][2]), &(test_in[b][3]), &(test_out[b][1]));
		}
	}
	if (executionId>=-1) {
		FAULTDET_testing_commitTmpStatsAndReset(injectingErrors);
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

#define FFT_LENGTH 8
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
	FAULTDET_testing_injectFault32(a.re, executionId, 32*0, (32*1)-1, injectingErrors);
	FAULTDET_testing_injectFault32(a.im, executionId, 32*1, (32*2)-1, injectingErrors);
	FAULTDET_testing_injectFault32(b.re, executionId, 32*2, (32*3)-1, injectingErrors);
	FAULTDET_testing_injectFault32(b.im, executionId, 32*3, (32*4)-1, injectingErrors);

	complex res;
	res.re=a.re + b.re;
	res.im=a.im + b.im;

	FAULTDET_testing_injectFault32(res.re, executionId, 32*4, (32*5)-1, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*5, (32*6)-1, injectingErrors);

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
	FAULTDET_testing_injectFault32(a.re, executionId, 32*0, (32*1)-1, injectingErrors);
	FAULTDET_testing_injectFault32(a.im, executionId, 32*1, (32*2)-1, injectingErrors);
	FAULTDET_testing_injectFault32(b.re, executionId, 32*2, (32*3)-1, injectingErrors);
	FAULTDET_testing_injectFault32(b.im, executionId, 32*3, (32*4)-1, injectingErrors);

	complex res;
	res.re=(a.re * b.re) - (a.im*b.im);
	res.im=(a.im*b.re) + (a.re*b.im);

	FAULTDET_testing_injectFault32(res.re, executionId, 32*4, (32*5)-1, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*5, (32*6)-1, injectingErrors);

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
	FAULTDET_testing_injectFault32(x, executionId, 32*0, (32*1)-1, injectingErrors);

	complex res;
	res.re=cos(x);
	res.im=sin(x);

	FAULTDET_testing_injectFault32(res.re, executionId, 32*1, (32*2)-1, injectingErrors);
	FAULTDET_testing_injectFault32(res.im, executionId, 32*2, (32*3)-1, injectingErrors);

	return res;
}

/**
 * @brief Actual fft implementation using Cooley–Tukey algorithm (radix-2 DIT case)
 *
 * @return Fourier transform for input array
 */

//LOOP1TOTAL=64+LOOP2TOTAL+
//LOOP2TOTAL=192+
#define LOOP2TOTAL (192*2+96)
#define LOOP3TOTAL (LOOP2TOTAL)
#define LOOP1TOTAL (32*4+FFT_LENGTH*LOOP3TOTAL+FFT_LENGTH*LOOP2TOTAL)
static void fft_routine(int executionId){
	int k;

	for(k=0;k<FFT_LENGTH;k++){


		/*X_k=[sum{0,N/2-1} x_2n * e^(i*(-2*pi*2n*k)/N)] + [sum{0,N/2-1} x_(2n+1) * e^(i*(-2*pi*(2n+1)*k)/N)]*/
		int n;
		complex even_sum,odd_sum;

		even_sum.re=0;
		even_sum.im=0;

		for(n=0;n<FFT_LENGTH;n=n+2){
			complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (32*2+n*LOOP2TOTAL+k*LOOP1TOTAL)); //96
			complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (32*2+96+n*LOOP2TOTAL+k*LOOP1TOTAL)); //192

			complex_sum(even_sum,n_term,  executionId - (32*2+96+192+n*LOOP2TOTAL+k*LOOP1TOTAL)); //192
		}
		FAULTDET_testing_injectFault32(even_sum.re, executionId, 32*0+k*LOOP1TOTAL, (32*1)-1+k*LOOP1TOTAL, injectingErrors);
		FAULTDET_testing_injectFault32(even_sum.im, executionId, 32*1+k*LOOP1TOTAL, (32*2)-1+k*LOOP1TOTAL, injectingErrors);

		float v1=array_in[0].re+array_in[2].re;
		float v2=array_in[4].re+array_in[6].re;
		float v3=array_in[0].im+array_in[2].im;
		float v4=array_in[4].im+array_in[6].im;

		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					k,  //checkId
					6,
					&(v1), &(v2), &(v3), &(v4), &(even_sum.re),  &(even_sum.im));
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					k, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					4,
					5,
					0,
					executionId,
#endif
					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					&(v1), &(v2), &(v3), &(v4), &(even_sum.re),  &(even_sum.im));
		}


		odd_sum.re=0;
		odd_sum.im=0;

		for(n=1;n<FFT_LENGTH;n=n+2){
			complex cmplxexp=complex_exp((-2*M_PI*n*k)/FFT_LENGTH, executionId - (32*4+n*LOOP3TOTAL+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL) ); //96
			complex n_term = complex_mult(array_in[n], cmplxexp, executionId - (32*4+96+n*LOOP3TOTAL+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL) ); //192

			complex_sum(odd_sum,n_term,  executionId - (32*4+96+192+n*LOOP3TOTAL+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL)); //192
		}

		FAULTDET_testing_injectFault32(odd_sum.re, executionId, (32*2+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL), (32*3-1+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL), injectingErrors);
		FAULTDET_testing_injectFault32(odd_sum.im, executionId, (32*3+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL), (32*4-1+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL), injectingErrors);


		v1=array_in[1].re+array_in[3].re;
		v2=array_in[5].re+array_in[7].re;
		v3=array_in[1].im+array_in[3].im;
		v4=array_in[5].im+array_in[7].im;

		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					k,  //checkId
					6,
					&(v1), &(v2), &(v3), &(v4), &(odd_sum.re),  &(odd_sum.im));
		} else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					k, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					4,
					5,
					0,
					executionId,
#endif
					6, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					&(v1), &(v2), &(v3), &(v4), &(odd_sum.re),  &(odd_sum.im));
		}

		complex out=complex_sum(even_sum,odd_sum, -10/*executionId - (32*4+FFT_LENGTH*LOOP3TOTAL+FFT_LENGTH*LOOP2TOTAL+k*LOOP1TOTAL)*/);


		array_out[k] = out; //192
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

#ifndef FAULTDETECTOR_EXECINSW
		FAULTDET_blockIfFaultDetectedInTask(&inst);
#endif


		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					0,  //checkId
					4,
					//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
					/*&(pid_heading.b),*/ &(pid_heading.backpropagation), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err, &desired_roll, &actual_roll);//, &actual_roll);

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
					3,
					3,
					roundId,
					executionId,
#endif
					4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					//					/*&(pid_heading.b),*/ &(pid_heading_backpropagation_orig), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err_orig, &desired_roll);//, &actual_roll);
					/*&(pid_heading.b),*/ &(pid_heading.backpropagation), /*&(pid_heading.d), &(pid_heading.i), &(pid_heading.p),*/ /*&(pid_heading.prev_error),*/ &err, &desired_roll, &actual_roll);//, &actual_roll);
		}



		//		if (executionId<-1) {
		//			FAULTDET_trainPoint(
		//					1,
		//					3,  //checkId
		//					2,
		//					&desired_roll, &actual_roll);
		//		} else {
		//			FAULTDET_testPoint(
		//#ifndef FAULTDETECTOR_EXECINSW
		//					&inst,
		//#endif
		//					1, //uniId
		//					3, //checkId
		//					0, //BLOCKING OR NON BLOCKING, non blocking
		//#ifdef testingCampaign
		//					injectingErrors,
		//					1,
		//					1,
		//					roundId,
		//					executionId,
		//#endif
		//					2, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
		//					&desired_roll, &actual_roll);
		//		}



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
					5,
					//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
					/*&(pid_roll.b),*/ &(pid_roll.backpropagation), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1, &desired_roll_rate, &actual_roll_rate, &curr_roll);//, &actual_roll_rate);

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
					3,
					3,
					roundId,
					executionId,
#endif
					5, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1_orig, &desired_roll_rate);//, &actual_roll_rate);
					/*&(pid_roll.b),*/ &(pid_roll.backpropagation), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err1, &desired_roll_rate, &actual_roll_rate, &curr_roll);//, &actual_roll_rate);
		}


		//		if (executionId<-1) {
		//			FAULTDET_trainPoint(
		//					1,
		//					4,  //checkId
		//					3,
		//					&actual_roll_rate, &curr_roll, &desired_roll_rate);
		//		} else {
		//			FAULTDET_testPoint(
		//#ifndef FAULTDETECTOR_EXECINSW
		//					&inst,
		//#endif
		//					1, //uniId
		//					4, //checkId
		//					0, //BLOCKING OR NON BLOCKING, non blocking
		//#ifdef testingCampaign
		//					injectingErrors,
		//					0,
		//					0,
		//					roundId,
		//					executionId,
		//#endif
		//					3, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
		//					&actual_roll_rate, &curr_roll, &desired_roll_rate);
		//		}

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
					4,
					//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
					/*&(pid_roll.b),*/ &(pid_roll.backpropagation), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2, &desired_ailerons, &actual_ailerons);//, &actual_ailerons);

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
					3,
					3,
					roundId,
					executionId,
#endif
					4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					//					/*&(pid_roll.b),*/ &(pid_roll_backpropagation_orig), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2_orig, &desired_ailerons);//, &actual_ailerons);
					/*&(pid_roll.b),*/ &(pid_roll.backpropagation), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ /*&(pid_roll.prev_error),*/ &err2, &desired_ailerons, &actual_ailerons);//, &actual_ailerons);
		}

		//		if (executionId<-1) {
		//			FAULTDET_trainPoint(
		//					1,
		//					5,  //checkId
		//					2,
		//					&desired_ailerons, &actual_ailerons);
		//		} else {
		//			FAULTDET_testPoint(
		//#ifndef FAULTDETECTOR_EXECINSW
		//					&inst,
		//#endif
		//					1, //uniId
		//					5, //checkId
		//					0, //BLOCKING OR NON BLOCKING, non blocking
		//#ifdef testingCampaign
		//					injectingErrors,
		//					1,
		//					1,
		//					roundId,
		//					executionId,
		//#endif
		//					2, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
		//					&desired_ailerons, &actual_ailerons);
		//		}

		pid_roll.backpropagation = actual_ailerons - desired_ailerons;


		if (executionId<-1) {
			FAULTDET_trainPoint(
					1,
					3,  //checkId
					4,
					/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &desired_ailerons);
		}  else {
			FAULTDET_testPoint(
#ifndef FAULTDETECTOR_EXECINSW
					&inst,
#endif
					1, //uniId
					3, //checkId
					0, //BLOCKING OR NON BLOCKING, non blocking
#ifdef testingCampaign
					injectingErrors,
					3,
					3,
					roundId,
					executionId,
#endif
					4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
					/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &desired_ailerons);
		}

		pid_roll.backpropagation = actual_ailerons - desired_ailerons;


		//		if (executionId<-1) {
		//			FAULTDET_trainPoint(
		//					1,
		//					6,  //checkId
		//					4,
		//					/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &actual_ailerons);
		//		}  else {
		//			FAULTDET_testPoint(
		//#ifndef FAULTDETECTOR_EXECINSW
		//					&inst,
		//#endif
		//					1, //uniId
		//					6, //checkId
		//					0, //BLOCKING OR NON BLOCKING, non blocking
		//#ifdef testingCampaign
		//					injectingErrors,
		//					3,
		//					3,
		//					roundId,
		//					executionId,
		//#endif
		//					4, //SIZE OF THIS SPECIFIC AOV (<=FAULTDETECTOR_MAX_AOV_DIM , unused elements will be initialised to 0)
		//					/*&(pid_roll.b),*/ &(curr_heading), /*&(pid_roll.d), &(pid_roll.i), &(pid_roll.p),*/ &(curr_roll), &curr_roll_rate, &actual_ailerons);
		//		}

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

	//	if (executions==0)
	//		executions=4000;
	//	if (regs==0)
	//		regs=16;
	//	if (trainIter==0)
	//		trainIter=8096;

	if (executions==0)
		executions=4000;
	if (regs==0)
		regs=8;
	if (trainIter==0)
		trainIter=200;

	setlocale(LC_NUMERIC,"en_US.UTF-8");

	printf("{\"regions\":%d, \"trainIterations\":%d, \"testIterations\":%d,  \"relerr\": [", regs, trainIter, executions);

#ifndef dynamicRegions
	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
		n_regions[i]=0;
		for (int j=0; j<FAULTDETECTOR_MAX_REGIONS; j++) {
			for (int k=0; k<FAULTDETECTOR_MAX_AOV_DIM; k++) {
				trainedRegions[i][j].center[k]=0.0;
				//				trainedRegions[i][j].max[k]=100.0;
				//				trainedRegions[i][j].min[k]=-100.0;
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

#ifdef ANNBench

	init_train_data();
	train_ann_routine();

	for (int i=-5000; i<-1; i++) {
		init_test_data();
		forward_pass_test_burst(i);
	}

	for (int i=0; i<50; i++) {
		init_test_data();
		injectingErrors=0x0;

		for (int executionId=-1 ;executionId<LOOP1TOTAL*BURST_LENGTH/*1503*//*960*/; executionId++) {
			forward_pass_test_burst(executionId);
		}
		FAULTDET_testing_resetGoldens();
	}
#endif

#ifdef FFTBench
	for (int executionId=-10000; executionId<-1; executionId++) {
		for(int i=0; i<FFT_LENGTH;i++){
			complex x;
			x.re=random_get();
			x.im=random_get();

			array_in[i]=x;
		}
		fft_routine(executionId);

	}

	for (int i=0; i<10000; i++) {
		for(int i=0; i<FFT_LENGTH;i++){
			complex x;
			x.re=random_get();
			x.im=random_get();

			array_in[i]=x;
		}
		injectingErrors=0x0;
		for (int executionId=-1 ;executionId<LOOP1TOTAL*FFT_LENGTH/*1503*//*960*/; executionId++) {
			fft_routine(executionId);
		}
		FAULTDET_testing_resetGoldens();
		injectingErrors=0;
	}
#endif

#ifdef latnavBench

	//	for (int i=0; i<1000; i++) {
	//		injectingErrors=0x0;


	for (int executionId=-trainIter-1 ;executionId<-1/*960*/; executionId++) {
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		latnav(0, executionId);
	}

	for (int i=0; i<executions; i++) {
		FAULTDET_testing_resetGoldens();
		injectingErrors=0x0;
		r1=random_get();
		r2=random_get();
		r3=random_get();
		r4=random_get();
		for (int executionId=-1 ;executionId<1312/*1503*//*960*/; executionId++) {
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
