/**
 * tb_clQueue.c
 * Test program for OpenCL queue implementation
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
#include <stdio.h>

#include "test/cl.h"
#include "test/timing.h"
#include "clQueue.h"

unsigned int *qBack;
unsigned int *qPtrBack;

/* Defines for the CLQueue testcase */
#define GLOBAL_DIM_X 8
#define GLOBAL_DIM_Y 8
#define GLOBAL_DIM_Z 8
#define GLOBAL_SIZE (GLOBAL_DIM_X * GLOBAL_DIM_Y * GLOBAL_DIM_Z)
#define DIMS 3

#define LOCAL_DIM_X 4
#define LOCAL_DIM_Y 4
#define LOCAL_DIM_Z 2
#define LOCAL_SIZE (LOCAL_DIM_X * LOCAL_DIM_Y * LOCAL_DIM_Z)

static const size_t __constant elems[DIMS] = {
	GLOBAL_DIM_X, GLOBAL_DIM_Y, GLOBAL_DIM_Z
};

static const size_t __constant elems_local[DIMS] = {
	LOCAL_DIM_X, LOCAL_DIM_Y, LOCAL_DIM_Z
};

int
clQueue_execute(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg, size_t threads, char *krnl)
{
	cl_int err;
	unsigned int i, start;
	cl_mem q, qData;
	cl_kernel kernel;

	/* Create the right kernel */
	kernel = clCreateKernel(prg, krnl, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}

	/* And go execute N times */
	for(i = 0; i < tSamples; i++) {
		q = clQueue_create(cid, ctx, cq, prg);
		qPtrBack = malloc(sizeof(clqueue));

		clSetKernelArg(kernel, 0, sizeof(cl_mem), &q);

		qData = clCreateBuffer(ctx, CL_MEM_READ_WRITE, threads * 8, NULL, &err);
		clSetKernelArg(kernel, 1, sizeof(cl_mem), &qData);
		tStart();

		err = clEnqueueNDRangeKernel(cq, kernel, DIMS, NULL, elems, elems_local,
				0, NULL, NULL);
		err |= clFinish(cq);
		if (err != CL_SUCCESS) {
			printf("clQueue: Could not execute kernel: %i\n", err);
			return -err;
		}
		tEnd(i);

		err = clEnqueueReadBuffer(cq, qData, CL_TRUE, 0, threads * 0x8, qBack,
				0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("clQueue: Could not read from kernel: %i\n", err);
			return -err;
		}

		err = clEnqueueReadBuffer(cq, q, CL_TRUE, 0, sizeof(clqueue), qPtrBack,
				0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("clQueue: Could not read from kernel: %i\n", err);
			return -err;
		}

		clFinish(cq);
		clReleaseMemObject(q);
		clReleaseMemObject(qData);
		clFinish(cq);
	}

	printf("-- Executing %s --\n", krnl);
	printf("Time: ");
	tPrint();

	/* Print buffer if requested */
	if(options.print_buffer) {
		start = qBack[1];
		for(i = 0; i < (threads * 2); i++) {
			if(i % 4 == 0)
				printf("\n%08x: ", start + (i<<2));

			printf("%08x ", qBack[i]);
		}
	}

	clReleaseKernel(kernel);

	return 0;
}

void
clQueue_validate(unsigned int *queue, unsigned int head, unsigned int tail)
{
	unsigned int start = queue[1]; /* Offset from index */
	unsigned int index, count;

	printf("\nValidating:\n");
	printf("Head: %08x\n", head);
	printf("Tail: %08x\n", tail);
	printf("Start: %08x\n\n", start);

	/* Define first index */
	index = (head - start) >> 2;
	for(count = 0; count <= GLOBAL_SIZE; count++) {
		if(queue[index] == 0) {
			if(queue[index + 1] == tail) {
				printf("Queue valid.\n");
			} else {
				printf("Queue invalid: Tail != last item\n");
				printf("Tail: %08x\n", tail);
				printf("Item: %08x\n", queue[index + 1]);
			}
			printf("Items: %u\n\n", count + 1);
			return;
		}

		index = (queue[index] - start) >> 2;
	}

	printf("Queue invalid: circular dependency\n\n");

}

int
clQueue_opts(unsigned int i, unsigned int argc, char **argv)
{
	return -1;
}

void
usage()
{
	printf("Usage: clQueue [options]\n");
	options_print();
	return;
}

int
main(int argc, char** argv) {
	printf("Queue on OpenCL 0.1\n");

	cl_platform_id pid;
	cl_device_id cid;
	cl_context ctx;
	cl_command_queue cq;
	cl_program prg;

	char *src[1];

	if(options_read(argc, argv, clQueue_opts)) {
		usage();
		return -1;
	}

	/* Get me a context */
	if(clSetup(&pid, &cid, &ctx, &cq)) {
		return -1;
	}

	src[0] = kernel_read("clQueue.cl");
	if(!src[0])
		return -1;

	prg = program_compile(pid, ctx, &cid, 1, src);
	if(prg < 0)
		return -1;

	/* Make me a global queue for debugging purposes */
	qBack = calloc(8, GLOBAL_SIZE);

	/* Go */
	clQueue_execute(cid, ctx, cq, prg, GLOBAL_SIZE, "clQueue_test_enqueue");
	clQueue_validate(qBack, qPtrBack[0], qPtrBack[1]);

	clQueue_execute(cid, ctx, cq, prg, GLOBAL_SIZE, "clQueue_test_dequeue");
	clQueue_validate(qBack, qPtrBack[0], qPtrBack[1]);

	free((void *)src[0]);
	clReleaseProgram(prg);

	return 0;
}
