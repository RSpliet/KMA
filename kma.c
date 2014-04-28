/**
 * kma.c
 * Implementation of a Superblock-based malloc routine in OpenCL
 * Copyright (C) 2013-2014 Roy Spliet
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "kma.h"

/* XXX: Only works on true 64 bit arch */
cl_mem
_kma_create_64(cl_context ctx, cl_command_queue cq, unsigned int sblocks) {
	cl_mem gQ;
	cl_int error;
	struct kma_heap_64 localHeap;

	localHeap.bytes = (sblocks * KMA_SB_SIZE) + (sizeof(struct kma_heap_64));
	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, localHeap.bytes, NULL, &error);
	if(error != CL_SUCCESS) {
		printf("KMA: Could not allocate heap on-device\n");
		return (cl_mem) 0;
	}

	error = clEnqueueWriteBuffer(cq, gQ, 0, 0, sizeof(struct kma_heap_64), &localHeap, 0, NULL, NULL);
	if(error != CL_SUCCESS) {
		printf("KMA: Could not setup heap on device\n");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
_kma_create_32(cl_context ctx, cl_command_queue cq, unsigned int sblocks) {
	cl_mem gQ;
	cl_int error;
	struct kma_heap_32 localHeap;

	localHeap.bytes = (sblocks * KMA_SB_SIZE) + (sizeof(struct kma_heap_32));
	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, localHeap.bytes, NULL, &error);
	if(error != CL_SUCCESS) {
		printf("KMA: Could not allocate heap on-device\n");
		return (cl_mem) 0;
	}

	error = clEnqueueWriteBuffer(cq, gQ, 0, 0, sizeof(struct kma_heap_32), &localHeap, 0, NULL, NULL);
	if(error != CL_SUCCESS) {
		printf("KMA: Could not setup heap on device\n");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
kma_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, unsigned int sblocks)
{
	cl_int error;
	cl_uint bits;
	cl_mem q;
	cl_kernel kernel;
	const size_t threads = 1;

	if(sblocks == 0)
			return NULL;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error) {
		printf("KMA: could not discover device address space\n");
		return NULL;
	}

	if(bits == 32) {
		q = _kma_create_32(ctx, cq, sblocks);
	} else {
		q = _kma_create_64(ctx, cq, sblocks);
	}

	if(!q) {
		printf("KMA: No heap!\n");
		return NULL;
	}

	/* Initialise kernel */
	kernel = clCreateKernel(prg, "clheap_init", &error);
	if(error != CL_SUCCESS) {
		printf("KMA: Could not create heap init kernel: %i\n", error);
		return NULL;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &q);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	error |= clFinish(cq);
	if (error != CL_SUCCESS) {
		printf("KMA: Could not execute heap init kernel: %i\n", error);
		return NULL;
	}
	clReleaseKernel(kernel);

	return q;
}
