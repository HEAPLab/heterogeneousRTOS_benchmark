#include "faultdetector_sw.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

const float FAULTDETECTOR_THRESH=FAULTDETECTOR_THRESH_CONSTANT;
#ifndef dynamicRegions
static FAULTDETECTOR_region_t regionsGlob[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS]; //regions from the distribution
#else
FAULTDETECTOR_region_t* regionsGlob[FAULTDETECTOR_MAX_CHECKS];
#endif
static u8 n_regionsGlob[FAULTDETECTOR_MAX_CHECKS];


char hasRegion(const FAULTDETECTOR_region_t regions[FAULTDETECTOR_MAX_REGIONS], const u8 n_regions, const float d[FAULTDETECTOR_MAX_AOV_DIM]){
	for(int i=0; i < n_regions; i++){

		//		if (i>=n_regions)
		//			break;
		for(int j=0; j < FAULTDETECTOR_MAX_AOV_DIM; j++){

			if((regions[i].min[j] <= d[j] /*|| fabs(regions[i].min[j] - d[j]) < FAULTDETECTOR_THRESH */) && ( regions[i].max[j] >= d[j] /*|| fabs(regions[i].max[j] - d[j]) < FAULTDETECTOR_THRESH */)) {
				if (j==FAULTDETECTOR_MAX_AOV_DIM-1)
					return 0xFF;
			} else break;
		}
	}
	return 0;
}

char is_valid(const float val[FAULTDETECTOR_MAX_AOV_DIM]){

	for(int i=0; i < FAULTDETECTOR_MAX_AOV_DIM;  i++) {

		if(isnan(val[i]) || val[i] == INFINITY || val[i] == -INFINITY)
			return 0;
	}
	return 0xFF;
}

void insert_point(FAULTDETECTOR_region_t regions[FAULTDETECTOR_MAX_REGIONS], u8 *n_regions, const float d[FAULTDETECTOR_MAX_AOV_DIM]) {//, bool is_accept){

	//int id = find_region(regions, n_regions, d);

	if (is_valid(d)) { //&& id<0) {
		//create a new node.
		for(int i=0; i < FAULTDETECTOR_MAX_AOV_DIM; i++){
			regions[(*n_regions)].min[i] = regions[(*n_regions)].max[i] = regions[(*n_regions)].center[i] = d[i];
		}
		(*n_regions)++;

		//if we're full of space, make room for another region.
		if((*n_regions) == FAULTDETECTOR_MAX_REGIONS){ //if we're full.
			//find the region with the most similar dynamics that isn't
			//completely obstructed by another region.
			int merge_1=-1;
			int merge_2=-1;
			float score = 0;

			int i_real=0;
			int k_real=1;

			float tmp_score=0;
			int tmp_other=-1;

			//FAULTDETECTOR_MAX_REGIONS_SUMM
			while (i_real < (*n_regions)-1) {

				//int tmp_other = find_closest_region(regions, n_regions, i, &tmp_score);

				//float bestscore=0.0;




				//				for(int k=i+1; k < FAULTDETECTOR_MAX_REGIONS; k++){

				//					if (k_real>=n_regions)
				//						break;
				//if(k != i) {
				//printf("score [%d,%d]:", i, k);

				float distance = 0;
				float overlap=1.0;

				for(int j=0; j < FAULTDETECTOR_MAX_AOV_DIM; j++){


					float d = (regions[i_real].center[j] - regions[k_real].center[j]);
					distance += d*d;



					float d1 = regions[i_real].max[j] - regions[i_real].min[j];
					float d2 = regions[k_real].max[j] - regions[k_real].min[j];
					float ov;
					if(regions[i_real].min[j] < regions[k_real].min[j])
						ov = d1 - (regions[k_real].min[j] - regions[i_real].min[j]);
					else
						ov = d2 - (regions[i_real].min[j] - regions[k_real].min[j]);
					ov = ov < 0 ? 0 : ov;
					overlap *= ov;
				}
				float sc;
				//printf("b:%f d:%f o:%f = %f\n", behavior, distance, overlap, score);
				//if we are overlapping with another group, merge regardless.
				if(overlap > 0)
					sc = overlap;
				else
					//severely penalize groups where there is an interfering group
					sc = -distance; //negatively impact behavior.



				if(tmp_other == -1 || sc > tmp_score){
					tmp_other = k_real;
					tmp_score = sc;
				}
				//}
				//				}


				if (k_real==(*n_regions)-1) {
					if(merge_1 < 0 || tmp_score > score){
						score = tmp_score;
						merge_1 = i_real;
						merge_2 = tmp_other;
					}
					tmp_score=0;
					tmp_other = -1;

					i_real++;
					k_real=i_real+1;
				} else {
					k_real++;
				}
			}

			//merge_regions(regions, merge_1, merge_2);
			//merge regions inlining
			for(int i=0; i < FAULTDETECTOR_MAX_AOV_DIM; i++){
				if(regions[merge_1].min[i] > regions[merge_2].min[i]){
					regions[merge_1].min[i] = regions[merge_2].min[i];
				}
				if(regions[merge_1].max[i] < regions[merge_2].max[i]){
					regions[merge_1].max[i] = regions[merge_2].max[i];
				}
				regions[merge_1].center[i] = (regions[merge_1].max[i] + regions[merge_1].min[i])/2.0;
			}

			//move everything over
			//			insert_point_label7:for(int i=merge_2; i < FAULTDETECTOR_MAX_REGIONS-1; i++){
			//				//if (i>=merge_2) {
			//					regions[i] = regions[i+1];
			//				//}
			//			}
			//if (merge_2!=(n_regions-1))
			regions[merge_2]=regions[FAULTDETECTOR_MAX_REGIONS-1];
			(*n_regions)--;
		}
	}
}

#ifdef dynamicRegions
void FAULTDETECTOR_SW_allocRegions(int number_of_regions) {
	FAULTDETECTOR_MAX_REGIONS=number_of_regions;
	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
		n_regionsGlob[i]=0;
		regionsGlob[i]=(FAULTDETECTOR_region_t*)calloc(number_of_regions, sizeof(FAULTDETECTOR_region_t));
		//		for (int j=0; j<number_of_regions; j++) {
		//			for (int k=0; k<FAULTDETECTOR_MAX_AOV_DIM; k++) {
		//				regionsGlob[i][j].center[k]=0.0;
		//				regionsGlob[i][j].max[k]=0.0;
		//				regionsGlob[i][j].min[k]=0.0;
		//			}
		//		}
	}
}

void FAULTDETECTOR_SW_freeRegions() {
	for (int i=0; i<FAULTDETECTOR_MAX_CHECKS; i++) {
		free(regionsGlob[i]);
	}
}
#endif

void FAULTDETECTOR_SW_initRegions(FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions_in[FAULTDETECTOR_MAX_CHECKS]) {
	memcpy(&regionsGlob, trainedRegions, sizeof(regionsGlob));
	memcpy(&n_regionsGlob, n_regions_in, sizeof(n_regionsGlob));
}

void FAULTDETECTOR_SW_dumpRegions(FAULTDETECTOR_region_t trainedRegions[FAULTDETECTOR_MAX_CHECKS][FAULTDETECTOR_MAX_REGIONS], u8 n_regions_out[FAULTDETECTOR_MAX_CHECKS]) {
	memcpy(trainedRegions, &regionsGlob, sizeof(regionsGlob));
	memcpy(n_regions_out, &n_regionsGlob, sizeof(n_regionsGlob));
}

char FAULTDETECTOR_SW_test(FAULTDETECTOR_controlStr* in) {
	return !(is_valid(in->AOV) && hasRegion(regionsGlob[in->checkId], n_regionsGlob[in->checkId], in->AOV));
}

void FAULTDETECTOR_SW_train(FAULTDETECTOR_controlStr* in) {
	insert_point(regionsGlob[in->checkId],
			&(n_regionsGlob[in->checkId]),
			in->AOV);
}
