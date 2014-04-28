/**
 * clQueue.h
 * Header include for OpenCL queue
 * Roy Spliet, February 5th 2013
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
#ifndef CLQUEUE_H
#define CLQUEUE_H

#ifdef __OPENCL_CL_H
#include <stdbool.h>
#include <stdint.h>

/* silence IDE */
#define __global
#define __kernel
#define __constant
#else
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#if CL_BITNESS == 64
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics : enable
#endif
#endif

typedef struct{
	volatile uintptr_t head; /**< Head ptr of the queue */
	volatile uintptr_t tail; /**< Tail ptr of the queue */
} clqueue;


typedef struct {
	/** Pointer to the next item in the queue. Must be 32-bit aligned, bit 0
	 * is used as "poison" bit when dequeuing.
	 * Under the condition that the "poison" bit 0 remains 1, the clqueue
	 * item can be part of a union. */
	volatile uintptr_t next;
} clqueue_item;

#ifdef __OPENCL_CL_H
typedef volatile uintptr_t vg_uptr_t;
typedef struct  {
	uint32_t head;
	uint32_t free;				 	     /**< Free list */
} clqueue_32;

typedef struct  {
	uint64_t head;
	uint64_t free;
} clqueue_64;

extern cl_mem clQueue_create(cl_device_id, cl_context, cl_command_queue, cl_program);
#else

#define NULL (uintptr_t) 0

#if CL_PLATFORM == 2
/* nVidia */
#define loop_infinite(n) for((n) = 0; (n) < 1048576; (n)++)
#else
#define loop_infinite(n) while(1)
#endif

extern void clQueue_init(clqueue __global *);
extern int enqueue(clqueue __global *, clqueue_item __global *);
extern clqueue_item __global *dequeue(clqueue __global *);
#endif

#endif
