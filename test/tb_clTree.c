/**
 * tb_clTree.c
 * Benchmark for OpenCL collaborative tree building
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
#include <CL/opencl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "timing.h"
#include "kma.h"
#include "pma.h"
#include "clArrayList.h"
#include "cl.h"

#define HEAP_KMA 0
#define HEAP_PM 1

char *dataset;

cl_uint lcount;
static uint32_t *links;

cl_mem
_clTree_init_32(cl_context ctx, cl_command_queue cq)
{
	cl_mem tree;
	cl_int error;
	uint32_t zero = 0;

	tree = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(uint32_t), NULL, &error);
	if(error) {
		printf("clTree: Could not allocate tree on-device\n");
		return 0;
	}

	error = clEnqueueWriteBuffer(cq, tree, 1, 0, sizeof(uint32_t), &zero, 0, NULL, NULL);
	error |= clFinish(cq);
	if(error) {
		printf("clTree: Could not initialise tree to zero: %i", error);
		return 0;
	}

	return tree;
}

cl_mem
_clTree_init_64(cl_context ctx, cl_command_queue cq)
{
	cl_mem tree;
	cl_int error;
	uint64_t zero = 0;

	tree = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(uint64_t), NULL, &error);
	if(error) {
		printf("clTree: Could not allocate tree on-device\n");
		return 0;
	}

	error = clEnqueueWriteBuffer(cq, tree, 1, 0, sizeof(uint64_t), &zero, 0, NULL, NULL);
	error |= clFinish(cq);
	if(error) {
		printf("clTree: Could not initialise tree to zero: %i", error);
		return 0;
	}

	return tree;
}

cl_mem
clTree_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg)
{
	cl_int error;
	cl_uint bits;
	cl_mem tree;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error) {
		printf("clTree: could not discover device address space\n");
		return 0;
	}

	if(bits == 32) {
		tree = _clTree_init_32(ctx, cq);
	} else {
		tree = _clTree_init_64(ctx, cq);
	}

	return tree;
}

cl_mem
clTree_pm_init(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, cl_mem heap)
{
	cl_int error;
	cl_kernel kernel;
	size_t threads = 1;
	cl_mem pm;
	unsigned int size = 4194304;

	pm = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(unsigned int) << 2, NULL, &error);
	if(error) {
		printf("clTree: Could not allocate poormans heap on-device\n");
		return 0;
	}

	kernel = clCreateKernel(prg, "pm_init", &error);
	if(error != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", error);
		return 0;
	}

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &pm);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &heap);
	clSetKernelArg(kernel, 2, sizeof(unsigned int), &size);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	error |= clFinish(cq);
	if(error != CL_SUCCESS) {
		printf("clTree: Could not initialise poormans heap: %i", error);
		return 0;
	}

	clReleaseKernel(kernel);

	return pm;
}

int
clTree_execute(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg, char *kname, unsigned int htype)
{
	cl_int err;
	unsigned int i, s;
	unsigned int threads;
	cl_kernel kernel;
	cl_mem heap, tree, data;

	/* Create correct kernels */
	kernel = clCreateKernel(prg, kname, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}

	data = clCreateBuffer(ctx, CL_MEM_READ_WRITE, lcount*8, NULL, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create dataset buffer: %i\n", err);
		return err;
	}
	err = clEnqueueWriteBuffer(cq, data, 1, 0, lcount*8, (uint32_t *)links, 0, NULL, NULL);
	err |= clFinish(cq);
	if(err != CL_SUCCESS) {
		printf("Error: Could not upload dataset: %i\n", err);
		return err;
	}

	printf("-- Executing %s--\n", kname);
	/* Set up the data structures */
	for(i = 0; i < options.wi_entries; i++) {
		for(s = 0; s < tSamples; s++) {
			tree = clTree_create(cid, ctx, cq, prg);
			if(htype == HEAP_KMA)
				heap = kma_create(cid, ctx, cq, prg, 512);
			else
				heap = pma_create(cid, ctx, cq, prg, 2097152);

			clSetKernelArg(kernel, 0, sizeof(cl_mem), &heap);
			clSetKernelArg(kernel, 1, sizeof(cl_mem), &tree);
			clSetKernelArg(kernel, 2, sizeof(cl_mem), &data);
			clSetKernelArg(kernel, 3, sizeof(unsigned int), &lcount);
			err = clFinish(cq);
			if (err != CL_SUCCESS) {
				printf("Error: Could not pre-execute kernel: %i\n", err);
				return -err;
			}

			tStart();
			/* Go */
			err = clEnqueueNDRangeKernel(cq, kernel, 3, NULL, &options.wi[i].x, NULL, 0, NULL, NULL);
			err |= clFinish(cq);
			if (err != CL_SUCCESS) {
				printf("Error: Could not execute kernel: %i\n", err);
				return -err;
			}
			tEnd(s);

			err = clEnqueueReadBuffer(cq, heap, CL_TRUE, 0, KMA_SB_SIZE * 512, heapBack, 0, NULL, NULL);
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

			err = clReleaseMemObject(tree);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free tree: %i\n", err);
				return -err;
			}
			clFinish(cq);
		}

		threads = options.wi[i].x * options.wi[i].y * options.wi[i].z;
		printf("Threads: %-5d", threads);
		tPrint();
	}

	clReleaseKernel(kernel);
	printf("\n");

	return 0;
}

int
clTree_execute_al(cl_device_id cid, cl_context ctx, cl_command_queue cq,
		cl_program prg)
{
	cl_int err;
	unsigned int i, s;
	unsigned int threads;
	cl_kernel kernel;
	cl_mem heap, tree, data, al, allink;

	/* Create correct kernels */
	kernel = clCreateKernel(prg, "clTree_test_al", &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create kernel: %i\n", err);
		return err;
	}

	data = clCreateBuffer(ctx, CL_MEM_READ_WRITE, lcount*8, NULL, &err);
	if(err != CL_SUCCESS) {
		printf("Error: Could not create dataset buffer: %i\n", err);
		return err;
	}
	err = clEnqueueWriteBuffer(cq, data, 1, 0, lcount*8, (uint32_t *)links, 0, NULL, NULL);
	err |= clFinish(cq);
	if(err != CL_SUCCESS) {
		printf("Error: Could not upload dataset: %i\n", err);
		return err;
	}

	printf("-- Executing clTree_test_al --\n");
	/* Set up the data structures */
	for(i = 0; i < options.wi_entries; i++) {
		for(s = 0; s < tSamples; s++) {
			tree = clTree_create(cid, ctx, cq, prg);
			heap = kma_create(cid, ctx, cq, prg, 1024);
			al = clArrayList_create(cid, ctx, cq, prg, 36, heap);
			allink = clArrayList_create(cid, ctx, cq, prg, 16, heap);

			clSetKernelArg(kernel, 0, sizeof(cl_mem), &al);
			clSetKernelArg(kernel, 1, sizeof(cl_mem), &allink);
			clSetKernelArg(kernel, 2, sizeof(cl_mem), &tree);
			clSetKernelArg(kernel, 3, sizeof(cl_mem), &data);
			clSetKernelArg(kernel, 4, sizeof(unsigned int), &lcount);
			err = clFinish(cq);
			if (err != CL_SUCCESS) {
				printf("Error: Could not pre-execute kernel: %i\n", err);
				return -err;
			}

			tStart();
			/* Go */
			err = clEnqueueNDRangeKernel(cq, kernel, 3, NULL, &options.wi[i].x, NULL, 0, NULL, NULL);
			err |= clFinish(cq);
			if (err != CL_SUCCESS) {
				printf("Error: Could not execute kernel: %i\n", err);
				return -err;
			}
			tEnd(s);

			err = clEnqueueReadBuffer(cq, heap, CL_TRUE, 0, KMA_SB_SIZE * 512, heapBack, 0, NULL, NULL);
			if (err != CL_SUCCESS) {
				printf("Error: Could not read from kernel: %i\n", err);
				return -err;
			}

			err = clFinish(cq);
			if(err != CL_SUCCESS) {
				printf("Error: Execution failed: %i\n", err);
				return -err;
			}
			err = clReleaseMemObject(al);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free heap: %i\n", err);
				return -err;
			}
			err = clReleaseMemObject(allink);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free heap: %i\n", err);
				return -err;
			}
			err = clReleaseMemObject(heap);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free heap: %i\n", err);
				return -err;
			}

			err = clReleaseMemObject(tree);
			if(err != CL_SUCCESS) {
				printf("Error: Could not free tree: %i\n", err);
				return -err;
			}
			clFinish(cq);
		}

		threads = options.wi[i].x * options.wi[i].y * options.wi[i].z;
		printf("Threads: %-5d", threads);
		tPrint();
	}

	clReleaseKernel(kernel);
	printf("\n");

	return 0;
}

int
clTree_read_file()
{
	unsigned int entries, i;
	struct {unsigned int source, sink;} *dt;
	int ret;
	FILE *fp;

	if(dataset == NULL) {
		printf("Error: No data set file specified\n");
		return -1;
	}

	fp = fopen(dataset, "r");
	if(!fp) {
		printf("Error: specified file cannot be opened (%s)\n", dataset);
		return -1;
	}

	ret = fscanf(fp, "%u\n", &entries);
	if(ret < 1) {
		printf("Error: Dataset file malformed\n");
		return -1;
	}

	dt = malloc(entries*sizeof(*dt));
	for(i = 0; i < entries; i++) {
		ret = fscanf(fp, "{%u,%u}\n", &dt[i].source, &dt[i].sink);
		if(ret < 2){
			printf("Error: Dataset file malformed\n");
			return -1;
		}
	}

	lcount = entries;
	links = (void *)dt;

	fclose(fp);
	return 0;
}

int
clTree_opts(unsigned int i, unsigned int argc, char **argv)
{
	if(argv[i][0] != '-') {
		/* Filename, let's assume this is the dataset */
		dataset = argv[i];
		return 0;
	}
	return -1;
}

void
usage()
{
	printf("Usage: clTree [options] [data file]\n");
	options_print();
	printf("The data file begins with the number of entries, followed by\n");
	printf("a series of {source, sink} tuples, each on a new line.\n");
	return;
}

int
main(int argc, char** argv)
{
	printf("Tree test using in-kernel malloc on OpenCL 0.1\n");

	cl_platform_id pid;
	cl_device_id cid;
	cl_context ctx;
	cl_command_queue cq;
	cl_program prg;

	char *src[6];

	dataset = NULL;
	if(options_read(argc, argv, clTree_opts)) {
		usage();
		return -1;
	}

	/* Get me a context */
	if(clSetup(&pid, &cid, &ctx, &cq)) {
		return -1;
	}

	/* Go and read the dataset */
	if(clTree_read_file() < 0)
		return -1;

	/* Compile the program! */
	src[0] = kernel_read("clQueue.cl");
	src[1] = kernel_read("clIndexedQueue.cl");
	src[2] = kernel_read("kma.cl");
	src[3] = kernel_read("clTree.cl");
	src[4] = kernel_read("clArrayList.cl");
	src[5] = kernel_read("test/tb_clTree.cl");

	prg = program_compile(pid, ctx, &cid, 6, src);
	if(prg < 0)
		return -1;

	/* Then tree */
	printf("KMA-1:\n");
	heapBack = malloc(KMA_SB_SIZE * 512);
	clTree_execute(cid, ctx, cq, prg, "clTree_test", HEAP_KMA);

	//heap = (unsigned int *)heapBack;
	/*for(i = 0; i < CLSBM_SB_SIZE * 4; i++) {
		if((i & 0x3) == 0)
			printf("\n%08x: ", i);
		printf("%08x ", heap[i]);
	}
	printf("\n");*/
	clTree_execute(cid, ctx, cq, prg, "clTree_test_cache", HEAP_KMA);

	/* Now with ArrayList */
	clTree_execute_al(cid, ctx, cq, prg);

	/*heap = (unsigned int *)heapBack;
	for(i = 0; i < (49152 >> 2); i++) {
		if((i & 0x3) == 0)
			printf("\n%08x: ", i);
		printf("%08x ", heap[i]);
	}
	printf("\n");*/

	/* Now with poormans heap */
	free((void *)src[2]);
	src[2] = kernel_read("pma.cl");
	prg = program_compile(pid, ctx, &cid, 6, src);
	if(prg < 0)
		return -1;

	printf("Poormans heap:\n");
	clTree_execute(cid, ctx, cq, prg, "clTree_test_cache", HEAP_PM);

	if(links)
		free(links);

	free((void *)src[0]);
	free((void *)src[1]);
	free((void *)src[2]);
	free((void *)src[3]);
	return 0;
}
