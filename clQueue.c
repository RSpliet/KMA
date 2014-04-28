/**
 * clQueue.c
 * Implementation of a queue in OpenCL
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

#include "clQueue.h"

cl_mem
_clQueue_create_64(cl_context ctx, cl_command_queue cq) {
	cl_mem gQ;
	cl_int error;

	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clqueue_64), NULL, &error);
	if(error) {
		printf("clQueue: Could not allocate queue on-device");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
_clQueue_create_32(cl_context ctx, cl_command_queue cq) {
	cl_mem gQ;
	cl_int error;

	gQ = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clqueue_32), NULL, &error);
	if(error) {
		printf("clQueue: Could not allocate queue on-device");
		return (cl_mem) 0;
	}

	clFinish(cq);

	return gQ;
}

cl_mem
clQueue_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg)
{
	cl_int error;
	cl_uint bits;
	cl_mem q;
	cl_kernel kernel;
	const size_t threads = 1;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error) {
		printf("clQueue: could not discover device address space\n");
		return NULL;
	}

	if(bits == 32) {
		q = _clQueue_create_32(ctx, cq);
	} else {
		q = _clQueue_create_64(ctx, cq);
	}

	if(!q)
		return NULL;

	/* Initialise kernel */
	kernel = clCreateKernel(prg, "clQueue_init", &error);
	if(error != CL_SUCCESS) {
		printf("clQueue: Could not create queue init kernel: %i\n", error);
		return NULL;
	}
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &q);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("clQueue: Could not execute queue init kernel: %i\n", error);
		return NULL;
	}
	clFinish(cq);
	clReleaseKernel(kernel);

	return q;
}
