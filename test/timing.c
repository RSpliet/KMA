/**
 * timing.c
 * Library for obtaining wall-clock time and calculating averages
 * Copyright (C) 2013-2014 Roy Spliet, Delft University of Technology
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include "timing.h"

struct timeval tstart, tend;
uint64_t tSample[tSamples];
int tStep;

uint64_t
tMin(uint64_t *pTList, int pLen) {
	int i;
	uint64_t retval = pTList[0];

	for(i = 0; i < pLen; i++) {
		if(retval > pTList[i])
			retval = pTList[i];
	}
	return retval;
}

uint64_t
tMax(uint64_t *pTList, int pLen) {
	int i;
	uint64_t retval = pTList[0];

	for(i = 0; i < pLen; i++) {
		if(retval < pTList[i])
			retval = pTList[i];
	}
	return retval;
}

uint64_t
tAvg(uint64_t *pTList, int pLen) {
	// Reduce by 10 every iteration
	int i,j,jsteps;
	uint64_t curSample;
	uint64_t* mySamples;

	mySamples = malloc(tSamples*sizeof(uint64_t));
	memcpy(mySamples,pTList,pLen*sizeof(uint64_t));

	for(i = 0; i < tSamples_baseten; i++) {
		jsteps = tSamples / pow(10,i+1);
		for(j = 0; j < jsteps; j++) {
			curSample = mySamples[j*10];
			curSample +=	mySamples[(j*10)+1];
			curSample +=	mySamples[(j*10)+2];
			curSample +=	mySamples[(j*10)+3];
			curSample +=	mySamples[(j*10)+4];
			curSample +=	mySamples[(j*10)+5];
			curSample +=	mySamples[(j*10)+6];
			curSample +=	mySamples[(j*10)+7];
			curSample +=	mySamples[(j*10)+8];
			curSample +=	mySamples[(j*10)+9];
			mySamples[j] = curSample / 10;
		}
	}

	curSample = mySamples[0];
	free(mySamples);
	return curSample;
}

/* If it fits... */
uint64_t
tTotal(uint64_t* pTList, int pLen) {
	int i;
	uint64_t retval = 0;

	for(i = 0; i < pLen; i++) {
		retval += pTList[i];
	}
	return retval;
}

void
tStart() {
	gettimeofday(&tstart,NULL);
}

void
tEnd(int pI) {
	gettimeofday(&tend,NULL);
	tSample[pI] = (tend.tv_sec * 1000000 + tend.tv_usec) - (tstart.tv_sec * 1000000 + tstart.tv_usec);
}

void
tPrint() {
	uint64_t avg,min,max,total;

	avg = tAvg(tSample,tSamples);
	min = tMin(tSample,tSamples);
	max = tMax(tSample,tSamples);
	total = tTotal(tSample,tSamples);
	printf("Avg: %"PRIu64".%06"PRIu64" ( %"PRIu64".%06"PRIu64" - %"PRIu64".%06"PRIu64" ) Total: %"PRIu64".%06"PRIu64"\n",avg/1000000,avg%1000000,min/1000000,min%1000000,max/1000000,max%1000000,total/1000000,total%1000000);
}
