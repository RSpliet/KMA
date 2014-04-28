/**
 * tb_clArrayList.c
 * Benchmark program for clArrayList
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
#include <time.h>

#include "test/cl.h"
#include "test/timing.h"
#include "clArrayList.h"
#include "pma.h"
#include "kma.h"

#define HEAP_KMA 0
#define HEAP_PM 1

int
clArrayList_execute(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg, const char *krnl, unsigned int backend)
{
	cl_int err;
	unsigned int i;
	unsigned int t, threads;
	cl_kernel kernel_top, kernel_bottom;
	cl_mem al, heap;

	/* Create correct kernels */
	kernel_top = clCreateKernel(prg, krnl, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}
/*
	kernel_bottom = clCreateKernel(prg, "clArrayList_test_bottom", &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}*/

	/* Go */
	printf("-- Executing %s --\n", krnl);
	for(t = 0; t < options.wi_entries; t++) {
		for(i = 0; i < tSamples; i++) {
			/* Set up the data structures */
			if(backend == HEAP_KMA)
				heap = kma_create(cid, ctx, cq, prg, 2048);
			else
				heap = pma_create(cid, ctx, cq, prg, 8388608);
			al = clArrayList_create(cid, ctx, cq, prg, sizeof(cl_uint), heap);
			clSetKernelArg(kernel_top, 0, sizeof(cl_mem), &heap);
			clSetKernelArg(kernel_top, 1, sizeof(cl_mem), &al);
			//clSetKernelArg(kernel_bottom, 1, sizeof(cl_mem), &al.list);
			//clSetKernelArg(kernel_bottom, 0, sizeof(cl_mem), &al.heap);

			/* Go 100 times */
			tStart();
			//for(j = 0; j < 100; j++) {
				err = clEnqueueNDRangeKernel(cq, kernel_top, 3, NULL, &options.wi[t].x, NULL, 0, NULL, NULL);
				if (err != CL_SUCCESS) {
					printf("Error: Could not execute kernel top: %i\n", err);
					return -err;
				}
				err = clFinish(cq);
				if (err != CL_SUCCESS) {
					printf("Error: Could not execute kernel: %i\n", err);
					return -err;
				}

				/*err = clEnqueueNDRangeKernel(cq, kernel_bottom, 1, NULL, &one, NULL, 0, NULL, NULL);
				if (err != CL_SUCCESS) {
					printf("Error: Could not execute kernel bottom : %i\n", err);
					return -err;
				}
				err = clFinish(cq);
			}*/
			tEnd(i);

			err = clEnqueueReadBuffer(cq, heap, CL_TRUE, 0, 393216, heapBack, 0, NULL, NULL);
			if (err != CL_SUCCESS) {
				printf("Error: Could not read from kernel: %i\n", err);
				return -err;
			}

			err = clFinish(cq);
			if(err != CL_SUCCESS) {
				printf("Error: Execution failed: %i\n", err);
				return -err;
			}
			err = clReleaseMemObject(heap);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free heap: %i\n", err);
				return -err;
			}

			err = clReleaseMemObject(al);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free arraylist: %i\n", err);
				return -err;
			}
			clFinish(cq);
		}

		threads = options.wi[t].x * options.wi[t].y * options.wi[t].z;
		printf("%-5u threads: ", threads);
		tPrint();
	}

	clReleaseKernel(kernel_top);
	clReleaseKernel(kernel_bottom);

	return 0;
}

int
clArrayList_opts(unsigned int i, unsigned int argc, char **argv)
{
	return -1;
}

void
usage()
{
	printf("Usage: clArrayList [options]\n");
	options_print();
	return;
}

int
main(int argc, char** argv)
{
	printf("ArrayList structure allocator on OpenCL 0.1\n");

	cl_platform_id pid;
	cl_device_id cid;
	cl_context ctx;
	cl_command_queue cq;
	cl_program prg;

	char *src[4];

	unsigned int i, *heap;

	if(options_read(argc, argv, clArrayList_opts)) {
		usage();
		return -1;
	}

	/* Get me a context */
	if(clSetup(&pid, &cid, &ctx, &cq)) {
		return -1;
	}

	/* Compile the program! */
	src[0] = kernel_read("clQueue.cl");
	src[1] = kernel_read("clIndexedQueue.cl");
	src[2] = kernel_read("pma.cl");
	src[3] = kernel_read("clArrayList.cl");

	prg = program_compile(pid, ctx, &cid, 4, src);
	if(prg < 0)
		return -1;

	srand(time(0));
	/*heapBack = malloc(GLOBAL_SIZE * 0x80);

	clheap_execute(cid, ctx, cq, xKernel, GLOBAL_SIZE);

	for(i = 0; i < GLOBAL_SIZE * 0x80; i++) {
		if((i & 0xf) == 0)
			printf("\n%08x: ", i);
		printf("%02hhx ", heapBack[i]);
	}
	printf("\n");

	free(heapBack); */

	/* Then arraylist */
	heapBack = malloc(393216);
	printf("Poormans heap:\n");
	clArrayList_execute(cid, ctx, cq, prg, "clArrayList_test_justgrow", HEAP_PM);

	if(options.print_buffer) {
		heap = (unsigned int *)heapBack;
		for(i = 0; i < 98304; i++) {
			if((i & 0x3) == 0)
				printf("\n%08x: ", i);
			printf("%08x ", heap[i]);
		}
		printf("\n");
	}

	clArrayList_execute(cid, ctx, cq, prg, "clArrayList_test_justgrow_malloc", HEAP_PM);
	if(options.print_buffer) {
		heap = (unsigned int *)heapBack;
		for(i = 0; i < 98304; i++) {
			if((i & 0x3) == 0)
				printf("\n%08x: ", i);
			printf("%08x ", heap[i]);
		}
		printf("\n");
	}
	printf("\n");

	/* And KMA */
	free((void *)src[2]);
	src[2] = kernel_read("kma.cl");
	prg = program_compile(pid, ctx, &cid, 4, src);
	if(prg < 0)
		return -1;

	printf("KMA01 heap:\n");
	clArrayList_execute(cid, ctx, cq, prg, "clArrayList_test_justgrow", HEAP_KMA);

	if(options.print_buffer) {
		heap = (unsigned int *)heapBack;
		for(i = 0; i < 98304; i++) {
			if((i & 0x3) == 0)
				printf("\n%08x: ", i);
			printf("%08x ", heap[i]);
		}
		printf("\n");
	}

	clArrayList_execute(cid, ctx, cq, prg, "clArrayList_test_justgrow_malloc", HEAP_KMA);
	if(options.print_buffer) {
		heap = (unsigned int *)heapBack;
		for(i = 0; i < 98304; i++) {
			if((i & 0x3) == 0)
				printf("\n%08x: ", i);
			printf("%08x ", heap[i]);
		}
		printf("\n");
	}

	free(heapBack);

	free((void *)src[0]);
	free((void *)src[1]);
	free((void *)src[2]);
	free((void *)src[3]);
	return 0;
}
