/**
 * clArrayList.h
 * Collaborative memory allocator using prefix-sum reduction, C frontend
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

#include "clArrayList.h"

cl_mem
_clArrayList_init_64(cl_context ctx, cl_command_queue cq)
{
	cl_mem al;
	cl_int error;

	al = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clArrayList_64), NULL, &error);
	if(error) {
		printf("CLArrayList: Could not allocate arraylist on-device");
		return (cl_mem) 0;
	}

	return al;
}

cl_mem
_clArrayList_init_32(cl_context ctx, cl_command_queue cq)
{
	cl_mem al;
	cl_int error;

	al = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(clArrayList_32), NULL, &error);
	if(error) {
		printf("CLArrayList: Could not allocate arraylist on-device");
		return (cl_mem) 0;
	}

	return al;
}

cl_mem
clArrayList_create(cl_device_id dev, cl_context ctx, cl_command_queue cq,
		cl_program prg, cl_uint objsize, cl_mem heap)
{
	cl_int error;
	cl_uint bits;
	cl_kernel kernel;
	size_t threads = 1;
	cl_mem al;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error) {
		printf("CLArrayList: could not discover device address space\n");
		return (cl_mem) 0;
	}

	if(bits == 32) {
		al = _clArrayList_init_32(ctx, cq);
	} else {
		al = _clArrayList_init_64(ctx, cq);
	}

	/* Initialise kernel */
	kernel = clCreateKernel(prg, "clArrayList_init", &error);
	if(error != CL_SUCCESS) {
		printf("clArrayList: Could not create arraylist init kernel: %i\n", error);
		return (cl_mem) 0;
	}

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &al);
	clSetKernelArg(kernel, 1, sizeof(cl_uint), &objsize);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &heap);

	error = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &threads, NULL, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		printf("clArrayList: Could not execute heap init kernel: %i\n", error);
		return (cl_mem) 0;
	}
	error = clFinish(cq);
	if(error != CL_SUCCESS) {
		printf("clArrayList: failed to initialise: %i\n", error);
		return (cl_mem) 0;
	}
	clReleaseKernel(kernel);

	return al;
}

