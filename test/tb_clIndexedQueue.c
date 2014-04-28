/**
 * tb_clIndexedQueue.c
 * Test program for indexed queue
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
#include <stdint.h>

#include "test/cl.h"
#include "test/timing.h"
#include "clIndexedQueue.h"

unsigned int *qBack;
unsigned int *qPtrBack;

/* Defines for the CLIndexedQueue testcase */
#define GLOBAL_DIM_X 32
#define GLOBAL_DIM_Y 32
#define GLOBAL_DIM_Z 32
#define GLOBAL_SIZE (GLOBAL_DIM_X * GLOBAL_DIM_Y * GLOBAL_DIM_Z)
#define DIMS 3

#define LOCAL_DIM_X 32
#define LOCAL_DIM_Y 2
#define LOCAL_DIM_Z 2
#define LOCAL_SIZE (LOCAL_DIM_X * LOCAL_DIM_Y * LOCAL_DIM_Z)

static const size_t __constant elems[DIMS] = {
	GLOBAL_DIM_X, GLOBAL_DIM_Y, GLOBAL_DIM_Z
};

static const size_t __constant elems_local[DIMS] = {
	LOCAL_DIM_X, LOCAL_DIM_Y, LOCAL_DIM_Z
};

int
clIndexedQueue_execute(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg, size_t threads, char *krnl)
{
	cl_int err;
	unsigned int i, start;
	cl_mem q, qData;
	cl_kernel kernel;
	size_t qsize;

	qsize = (platform_bits(cid) == 32) ?
			sizeof(clIndexedQueue_32) : sizeof(clIndexedQueue_64);

	/* Create the right kernel */
	kernel = clCreateKernel(prg, krnl, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}

	/* And go execute N times */
	printf("-- Executing %s --\n", krnl);
	for(i = 0; i < 1; i++) {
		qData = clCreateBuffer(ctx, CL_MEM_READ_WRITE, (threads+1)*16, NULL, &err);

		q = clIndexedQueue_create(cid, ctx, cq, prg, qData, 4);
		qPtrBack = malloc(qsize);

		clSetKernelArg(kernel, 0, sizeof(cl_mem), &q);
		clSetKernelArg(kernel, 1, sizeof(cl_mem), &qData);
		tStart();

		err = clEnqueueNDRangeKernel(cq, kernel, 3, NULL, &elems, NULL,
				0, NULL, NULL);
		err |= clFinish(cq);
		if (err != CL_SUCCESS) {
			printf("clQueue: Could not execute kernel: %i\n", err);
			return -err;
		}
		tEnd(i);

		err = clEnqueueReadBuffer(cq, qData, CL_TRUE, 0, (threads+1)*16, qBack,
				0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("clQueue: Could not read from kernel: %i\n", err);
			return -err;
		}

		err = clEnqueueReadBuffer(cq, q, CL_TRUE, 0, qsize, qPtrBack,
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
	printf("Time: ");
	tPrint();

	/* Print buffer if requested */
	if(options.print_buffer) {
		start = qBack[2];
		for(i = 0; i < ((threads + 1) << 2); i++) {
			if(i % 4 == 0)
				printf("\n%08x: ", start + (i<<2));

			printf("%08x ", qBack[i]);
		}
		printf("\n\n");
	}

	clReleaseKernel(kernel);

	return 0;
}

void
clIndexedQueue_validate(unsigned int *queue, unsigned int head, unsigned int tail)
{
	struct {
		uint32_t idx;
		uint32_t filler;
		uint32_t ptr;
		uint32_t filler2;
	} *q = (void *)queue;
	uint32_t start = q[0].ptr; /* Offset from index */
	unsigned int index, count;
	uint32_t abshead, abstail;


	printf("\nValidating:\n");
	abshead = ((head & 0x1fffe) << 3) + start - 16;
	abstail = ((tail & 0x1fffe) << 3) + start - 16;
	printf("Head: %08x (%08x)\n", abshead, head);
	printf("Tail: %08x (%08x)\n", abstail, tail);
	printf("Start: %08x\n\n", start);

	/* Define first index */
	index = ((head & 0x1ffffe) >> 1) - 1;
	for(count = 0; count <= GLOBAL_SIZE+1; count++) {
		if((q[index].idx & 0x1ffffe) == 0) {
			if(q[index].ptr == abstail) {
				printf("Queue valid.\n");
			} else {
				printf("Queue invalid: Tail != last item\n");
				printf("Tail: %08x\n", tail);
				printf("Item: %08x\n", q[index].ptr);
			}
			printf("Items: %u\n\n", count + 1);
			return;
		}

		index = ((q[index].idx & 0x1ffffe) >> 1) - 1;
	}
	printf("Queue invalid: circular dependency\n\n");
}

void
clIndexedQueue_validate64(unsigned int *queue, unsigned int head, unsigned int tail)
{
	struct {
		uint32_t idx;
		uint32_t filler;
		uint64_t ptr;
	} *q = (void *)queue;
	uint64_t start = q[0].ptr; /* Offset from index */
	unsigned int index, count;
	uint64_t abshead, abstail;


	printf("\nValidating:\n");
	abshead = ((head & 0x1fffe) << 3) + start - 16;
	abstail = ((tail & 0x1fffe) << 3) + start - 16;
	printf("Head: %016lx (%08x)\n", abshead, head);
	printf("Tail: %016lx (%08x)\n", abstail, tail);
	printf("Start: %016lx\n\n", start);

	/* Define first index */
	index = ((head & 0x1ffffe) >> 1) - 1;
	for(count = 0; count <= GLOBAL_SIZE+1; count++) {
		if((q[index].idx & 0x1ffffe) == 0) {
			if(q[index].ptr == abstail) {
				printf("Queue valid.\n");
			} else {
				printf("Queue invalid: Tail != last item\n");
				printf("Tail: %08x\n", tail);
				printf("Item: %016lx\n", q[index].ptr);
			}
			printf("Items: %u\n\n", count + 1);
			return;
		}

		index = ((q[index].idx & 0x1ffffe) >> 1) - 1;
	}
	printf("Queue invalid: circular dependency\n\n");
}

int
clIndexedQueue_opts(unsigned int i, unsigned int argc, char **argv)
{
	return -1;
}

void
usage()
{
	printf("Usage: clIndexedQueue [options]\n");
	options_print();
	return;
}

int
main(int argc, char** argv) {
	printf("Index-based queue on OpenCL 0.1\n");

	cl_platform_id pid;
	cl_device_id cid;
	cl_context ctx;
	cl_command_queue cq;
	cl_program prg;

	char *src[1];
	unsigned int bits;

	if(options_read(argc, argv, clIndexedQueue_opts)) {
		usage();
		return -1;
	}

	/* Get me a context */
	if(clSetup(&pid, &cid, &ctx, &cq)) {
		return -1;
	}

	src[0] = kernel_read("clIndexedQueue.cl");
	if(!src[0])
		return -1;

	prg = program_compile(pid, ctx, &cid, 1, src);
	if(prg < 0)
		return -1;
	bits = platform_bits(cid);

	/* Make me a global queue for debugging purposes */
	qBack = calloc(16, GLOBAL_SIZE+1);

	/* Go */
	clIndexedQueue_execute(cid, ctx, cq, prg, GLOBAL_SIZE, "clIndexedQueue_test_enqueue");
	if(bits == 32)
		clIndexedQueue_validate(qBack, qPtrBack[2], qPtrBack[3]);
	if(bits == 64)
		clIndexedQueue_validate64(qBack, qPtrBack[4], qPtrBack[5]);

	clIndexedQueue_execute(cid, ctx, cq, prg, GLOBAL_SIZE, "clIndexedQueue_test_dequeue");
	if(bits == 32)
		clIndexedQueue_validate(qBack, qPtrBack[2], qPtrBack[3]);
	if(bits == 64)
		clIndexedQueue_validate64(qBack, qPtrBack[4], qPtrBack[5]);

	free((void *)src[0]);
	clReleaseProgram(prg);

	return 0;
}
