/**
 * clIndexedQueue.c
 * Implementation of an indexed queue in OpenCL
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "clIndexedQueue.h"


/* XXX: Only works on true 64 bit arch */
cl_mem
_clIndexedQueue_create_64(cl_context ctx, cl_command_queue cq) {
	cl_mem gQ;
	cl_int error;

	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clIndexedQueue_64), NULL, &error);
	if(error) {
		printf("clQueue: Could not allocate queue on-device");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
_clIndexedQueue_create_32(cl_context ctx, cl_command_queue cq) {
	cl_mem gQ;
	cl_int error;

	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clIndexedQueue_32), NULL, &error);
	if(error) {
		printf("clQueue: Could not allocate queue on-device");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
clIndexedQueue_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, cl_mem base, cl_uint stride_l2)
{
	cl_int error;
	cl_uint bits;
	cl_mem q;
	cl_kernel kernel;
	const size_t threads = 1;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error) {
		printf("clIndexedQueue: could not discover device address space\n");
		return NULL;
	}

	if(bits == 32) {
		q = _clIndexedQueue_create_32(ctx, cq);
	} else {
		q = _clIndexedQueue_create_64(ctx, cq);
	}

	if(!q)
		return NULL;

	/* Initialise kernel */
	kernel = clCreateKernel(prg, "clIndexedQueue_init", &error);
	if(error != CL_SUCCESS) {
		printf("clIndexedQueue: Could not create queue init kernel: %i\n", error);
		return NULL;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &q);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &base);
	clSetKernelArg(kernel, 2, sizeof(cl_uint), &stride_l2);
	clSetKernelArg(kernel, 3, sizeof(cl_mem), &base);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("clIndexedQueue: Could not execute queue init kernel: %i\n", error);
		return NULL;
	}
	error = clFinish(cq);
	if (error != CL_SUCCESS) {
		printf("clIndexedQueue: Could not execute queue init kernel: %i\n", error);
		return NULL;
	}
	clReleaseKernel(kernel);

	return q;
}
