/**
 * clIndexedQueue.h
 * Header definitions for indexed queue in OpenCL
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
#ifndef CLINDEXEDQUEUE_H
#define CLINDEXEDQUEUE_H

#ifdef __OPENCL_CL_H
#include <stdbool.h>
#include <stdint.h>

/* silence IDE */
#define __global
#define __kernel
#define __constant
#else
#define uint32_t unsigned int
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#endif

typedef struct{
	char __global *base;
	size_t stride;
	volatile uint32_t head; /**< Head ptr of the queue */
	volatile uint32_t tail; /**< Tail ptr of the queue */
} clIndexedQueue;

typedef struct {
	/** Pointer to the next item in the queue. Must be 32-bit aligned, bit 0
	 * is used as "poison" bit when dequeuing.
	 * Under the condition that the "poison" bit 0 remains 1, the clqueue
	 * item can be part of a union. */
	volatile uint32_t next;
} clIndexedQueue_item;

#ifdef __OPENCL_CL_H
typedef volatile uintptr_t vg_uptr_t;
typedef struct  {
	uint32_t base;
	uint32_t stride;
	uint32_t head;
	uint32_t tail;
} clIndexedQueue_32;

typedef struct  {
	uint64_t base;
	uint64_t stride;
	uint32_t head;
	uint32_t tail;
} clIndexedQueue_64;

extern cl_mem clIndexedQueue_create(cl_device_id, cl_context, cl_command_queue,
		cl_program, cl_mem base, cl_uint stride_l2);
#else

#define NULL (uintptr_t) 0

extern void clIndexedQueue_init(void __global *, void __global *,
		uint32_t, void __global *);
extern int idxd_enqueue(clIndexedQueue __global *, clIndexedQueue_item __global *);
extern clIndexedQueue_item __global *idxd_dequeue(clIndexedQueue __global *);
#endif

#endif
