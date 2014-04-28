/**
 * tb_kma.c
 * Benchmark for KMA (and PMA) memory allocators
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
#include "kma.h"

unsigned int *qBack;
unsigned int iters;
unsigned int step;

int
kma_test_sbid(cl_context ctx, cl_command_queue cq, cl_program prg)
{
	cl_int error;
	cl_kernel kernel;
	cl_mem gQ;
	size_t threads = 16;
	unsigned int i;

	/* Now test the size thingy unit */
	qBack = calloc(sizeof(unsigned int), threads);

	/* Test initialisation */
	/* Make me a little buffer */
	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, threads * sizeof(unsigned int), NULL, &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not allocate little buffer on-device\n");
		return -1;
	}

	/* Execute a kernel */
	kernel = clCreateKernel(prg, "kma_test_size_by_sbid", &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not create heap init kernel: %i\n", error);
		return -1;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &gQ);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("KMA_test: Could not execute heap init kernel: %i\n", error);
		return -1;
	}

	error = clEnqueueReadBuffer(cq, gQ, 0, 0, sizeof(unsigned int) * 32, qBack, 0, NULL, NULL);
	if(error != CL_SUCCESS){
		printf("KMA_test: Could not read back from heap: %i\n", error);
		return -1;
	}
	clFinish(cq);

	/* print the contents of the stack */
	for(i = 0; i < threads; i++) {
		printf("  %d\n", qBack[i]);
	}
	printf("\n");
	free(qBack);
	clReleaseMemObject(gQ);

	return 0;
}

int
kma_test_slots(cl_context ctx, cl_command_queue cq, cl_program prg)
{
	cl_int error;
	cl_kernel kernel;
	cl_mem gQ;
	size_t threads = 16;
	unsigned int i;

	/* Now test the size thingy unit */
	qBack = calloc(sizeof(unsigned int), threads);

	/* Test initialisation */
	/* Make me a little buffer */
	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, threads * sizeof(unsigned int), NULL, &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not allocate little buffer on-device\n");
		return -1;
	}

	/* Execute a kernel */
	kernel = clCreateKernel(prg, "kma_test_slots_by_sbid", &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not create heap init kernel: %i\n", error);
		return -1;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &gQ);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("KMA_test: Could not execute heap init kernel: %i\n", error);
		return -1;
	}

	error = clEnqueueReadBuffer(cq, gQ, 0, 0, sizeof(unsigned int) * 16, qBack, 0, NULL, NULL);
	if(error != CL_SUCCESS){
		printf("KMA_test: Could not read back from heap: %i\n", error);
		return -1;
	}
	clFinish(cq);

	/* print the contents of the stack */
	for(i = 0; i < threads; i++) {
		printf("  %d\n", qBack[i]);
	}
	printf("\n");
	free(qBack);
	clReleaseMemObject(gQ);

	return 0;
}

int
kma_test_size(cl_context ctx, cl_command_queue cq, cl_program prg)
{
	cl_int error;
	cl_kernel kernel;
	cl_mem gQ;
	const size_t threads[] = {16,16,16};
	unsigned int i;

	/* Now test the size thingy unit */
	qBack = calloc(sizeof(unsigned int), 4096);

	/* Test initialisation */
	/* Make me a little buffer */
	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, 4096 * sizeof(unsigned int), NULL, &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not allocate little buffer on-device\n");
		return -1;
	}

	/* Execute a kernel */
	kernel = clCreateKernel(prg, "kma_test_sbid_by_size", &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not create size test kernel: %i\n", error);
		return -1;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &gQ);

	error = clEnqueueNDRangeKernel(cq, kernel, 3, NULL, &threads[0], NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("KMA_test: Could not execute size test kernel: %i\n", error);
		return -1;
	}

	error = clEnqueueReadBuffer(cq, gQ, 0, 0, 4096 * sizeof(unsigned int), qBack, 0, NULL, NULL);
	if(error != CL_SUCCESS){
		printf("KMA_test: Could not read back from size test buf: %i\n", error);
		return -1;
	}
	clFinish(cq);

	/* print the contents of the stack */
	for(i = 0; i < 4096; i++) {
		printf("  %d\n", qBack[i]);
	}
	printf("\n");
	free(qBack);
	clReleaseMemObject(gQ);

	return 0;
}

int
kma_test_init(cl_device_id cid, cl_context ctx, cl_command_queue cq, cl_program prg)
{
	cl_int err;
	unsigned int i;
	cl_mem heap;

	qBack = calloc(KMA_SB_SIZE, 32);

	/* Create initialises with a single thread */
	heap = kma_create(cid, ctx, cq, prg, 32);
	err = clEnqueueReadBuffer(cq, heap, 0, 0, KMA_SB_SIZE * 32, qBack, 0, NULL, NULL);
	if(err != CL_SUCCESS){
		printf("KMA_test: Could not read back from heap: %i\n", err);
		return -1;
	}
	clFinish(cq);

	/* print the contents of the stack */
	if(options.print_buffer) {
		for(i = 0; i < (KMA_SB_SIZE * 8); i++) {
			if((i % KMA_SB_SIZE) == 0)
				printf("\n");
			if((i & 0x3) == 0)
				printf("\n%08x: ", i << 2);
			printf("%08x ", qBack[i]);
		}
		printf("\n");
	}
	free(qBack);
	clReleaseMemObject(heap);

	return 0;
}

int
kma_test_malloc(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg, char *krnl)
{
	cl_int error;
	unsigned int i, t, threads, its;
	cl_mem heap;
	cl_kernel kernel, detect_orphans;

	qBack = calloc(KMA_SB_SIZE, 64);

	/* Create initialises with a single thread */
	heap = kma_create(cid, ctx, cq, prg, 127);

	/* Execute test_malloc */
	kernel = clCreateKernel(prg, krnl, &error);
	if(error != CL_SUCCESS) {
		printf("KMA_test: Could not create size test kernel: %i\n", error);
		return -1;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &heap);

/*	detect_orphans = clCreateKernel(prg, "clSBMalloc_test_heap_sbs", &error);
	if(error != CL_SUCCESS) {
		printf("clSBMalloc: Could not create size test kernel: %i\n", error);
		return -1;
	}
	clSetKernelArg(detect_orphans, 0, sizeof(cl_mem), &heap);*/

	printf("-- Executing %s --\n", krnl);
	for(t = 0; t < options.wi_entries; t++) {
		if(step == 0) {
			its = iters;
		} else {
			its = 2;
		}

		for(; its <= iters; its += step) {
			clSetKernelArg(kernel, 1, sizeof(cl_uint), &its);
			for(i = 0; i < tSamples; i++) {
				tStart();
				error = clEnqueueNDRangeKernel(cq, kernel, 3, NULL, &options.wi[t].x, NULL, 0, NULL, NULL);
				if (error != CL_SUCCESS) {
					printf("KMA_test: Could not execute size test kernel: %i\n", error);
					return -1;
				}

				error = clFinish(cq);
				if (error != CL_SUCCESS) {
					printf("KMA_test: Size test kernel did not finish: %i\n", error);
					return -1;
				}
				tEnd(i);

				error = clEnqueueReadBuffer(cq, heap, 0, 0, KMA_SB_SIZE * 15 + sizeof(struct kma_heap_32), qBack, 0, NULL, NULL);
				if(error != CL_SUCCESS){
					printf("KMA_test: Could not read back from size test buf: %i\n", error);
					return -1;
				}
				clFinish(cq);
			}
			threads = options.wi[t].x * options.wi[t].y * options.wi[t].z;
			printf("%-5u threads %-5u iters: ", threads, its);
			tPrint();
			//printf("WG Size: %d\n", qBack[14]);

			if(its == 2 && step > 2)
				its -= 2;
			if(step == 0)
				break;
		}

		/*error = clEnqueueNDRangeKernel(cq, detect_orphans, 1, NULL, &one, NULL, 0, NULL, NULL);
		if (error != CL_SUCCESS) {
			printf("clSBMalloc: Could not execute size test kernel: %i\n", error);
			return -1;
		}

		clFinish(cq);*/
	}

	/* print the contents of the stack */
	if(options.print_buffer) {
		for(i = 0; i < (KMA_SB_SIZE * 63); i++) {
			if((i % KMA_SB_SIZE) == 0)
				printf("\n");
			if((i & 0x3) == 0)
				printf("\n%08x: ", i << 2);
			printf("%08x ", qBack[i]);
		}
		printf("\n");
	}

	free(qBack);
	clReleaseMemObject(heap);
	printf("\n");

	return 0;
}

int
kma_opts(unsigned int i, unsigned int argc, char **argv)
{
	unsigned int lstep, lit;
	int ret;

	if(strncmp(argv[i], "-i", 2) == 0) {
		i++;
		if(i < argc) {
			ret = sscanf(argv[i], "%u,%u", &lstep, &lit);
			switch(ret) {
			case 1:
				iters = lstep;
				step = 0;
				break;
			case 2:
				iters = lit;
				step = lstep;
				break;
			default:
				return -1;
			}
			printf("Step: %d\n", step);
			printf("Iter: %d\n", iters);
			return 1;
		}
	}
	return -1;
}

void
usage()
{
	printf("Usage: kma [options]\n");
	printf("\t-i [step,]iters:Iteration count for kernel (default: 100)\n");
	options_print();
	printf("\n");
	printf("For -i, if step is defined, the program will run each thread config\n");
	printf("with {0, step, 2*step..iters} iterations\n");
	return;
}

int
main(int argc, char** argv) {
	printf("Superblock-based malloc on OpenCL 0.1\n");

	cl_platform_id pid;
	cl_device_id cid;
	cl_context ctx = 0;
	cl_command_queue cq;
	cl_program prg;

	char *src[2];

	iters = 100;
	step  = 0;

	if(options_read(argc, argv, kma_opts)) {
		usage();
		return -1;
	}

	/* Get me a context */
	if(clSetup(&pid, &cid, &ctx, &cq)) {
		return -1;
	}

	/* Compile the program! */
	src[0] = kernel_read("clIndexedQueue.cl");
	src[1] = kernel_read("kma.cl");

	prg = program_compile(pid, ctx, &cid, 2, src);
	if(prg < 0)
		return -1;

	/* Test initialisation */
	//kma_test_init(cid, ctx, cq, prg);

	/* Test the block size generator */
	//kma_test_sbid(ctx, cq, prg);
	//kma_test_slots(ctx, cq, prg);
	//kma_test_size(ctx, cq, prg);

	/* And now the big one */
	kma_test_malloc(cid, ctx, cq, prg, "kma_test_malloc");
	kma_test_malloc(cid, ctx, cq, prg, "kma_test_malloc_lowvar");
	kma_test_malloc(cid, ctx, cq, prg, "kma_test_malloc_highvar");

	free(src[0]);
	free(src[1]);
	return 0;
}
