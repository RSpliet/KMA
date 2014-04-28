/**
 * clheap.h
 * Heap allocation for OpenCL kernels - API
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
#ifndef CLHEAP_H
#define CLHEAP_H

#ifdef __OPENCL_CL_H
#include <stdbool.h>

/* silence IDE */
#define __global
#define __kernel
#define __constant
#else

#define NULL (uintptr_t) 0
typedef volatile uintptr_t __global vg_uptr_t;
struct clheap;

void heap_init(void __global *);
void __global *malloc(__global struct clheap *heap, size_t);
void free(__global struct clheap *heap, uintptr_t);

#endif /* __OPENCL_CL_H */
#endif /* CLHEAP_H */
