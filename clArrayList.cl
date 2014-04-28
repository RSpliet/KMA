/**
 * clArrayList.cl
 * A simple arraylist-like "object" for efficient malloc useage
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
#include "clArrayList.h"

__kernel void
clArrayList_init(char __global *arrayList, unsigned int size, void __global *hp) {
	struct clheap __global *heap = (struct clheap __global *)hp;
	clArrayList __global *l = (clArrayList __global *)arrayList;
	clQueue_init(&l->queue);
	l->objSize = size;;
	l->heap = heap;
}

uintptr_t
clArrayList_grow(clArrayList __global *l, unsigned int objs) {
	struct clArrayList_page __global *p;
	size_t size;
	char __global *pc;

	size = (objs * l->objSize) + sizeof(struct clArrayList_page);
	p = (struct clArrayList_page __global *) malloc(l->heap, size);
	if(p == NULL)
		return NULL;
	p->count = objs;
	p->taken = objs;

	enqueue(&l->queue, &p->next);

	pc = (char __global *) p;
	pc += sizeof(struct clArrayList_page);

	return (uintptr_t) pc;
}

//uintptr_t
//clArrayList_grow_global(clArrayList __global *l, unsigned int objs) {
//	const unsigned int pid = get_global_id(0);
//	int that;
//	unsigned int p = 1, off;
//
//	l->reduce_mem[pid] = objs * l->objSize;
//	barrier(CLK_GLOBAL_MEM_FENCE);
//
//	/* Prefix-sum bluff me through this */
//	/* Up-sweep */
//	do {
//		off = p;
//		p <<= 1;
//		if(pid % p == p - 1) {
//			l->reduce_mem[pid] += l->reduce_mem[pid - off];
//		}
//		barrier(CLK_GLOBAL_MEM_FENCE);
//	} while (p < get_global_size(0));
//
//	if(pid == get_global_size(0)-1) {
//		/* In XMalloc, you'd want to allocate with this value! */
//		l->reduce_mem[pid] = clArrayList_grow(l, l->reduce_mem[pid] / l->objSize);
//	}
//
//	/* Down-sweep */
//	do{
//		barrier(CLK_GLOBAL_MEM_FENCE);
//		off = p >> 1;
//		if(pid % p == p - 1) {
//			that = l->reduce_mem[pid - off];
//			l->reduce_mem[pid - off] = l->reduce_mem[pid];
//			l->reduce_mem[pid] += that;
//		}
//		p = off;
//	} while (p > 1);
//
//	if(objs > 0)
//		return l->reduce_mem[pid];
//	else
//		return NULL;
//}

unsigned int
next_pow2(unsigned int value)
{
	/* find MSB */
	unsigned int i = 31;
	while(!(value & (1 << i))) {
		i--;
	}

	if(value & ((1 << i) - 1))
		i++;

	return (1 << i);
}

uintptr_t
clArrayList_grow_local(clArrayList __global *l, unsigned int objs,
		uintptr_t __local *rMem) {
	size_t lid = 0, j;
	unsigned int i, total;
	unsigned int p = 1, off = 0, base;
	int that;

	/* Find my id normalised from 3 to 1 dimension */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		lid += j * get_local_id(i);
		j *= get_local_size(i);
	}

	barrier(CLK_LOCAL_MEM_FENCE);
	/* set my desired objects */
	rMem[lid] = objs * l->objSize;
	/* Pad to the next power of 2... and some */
	if((lid + j) < 128)
		rMem[lid + j] = 0;

	i = next_pow2(j);

	/* Up-sweep - right threads omitted */
	while (i > 1) {
		i >>= 1;
		off++;
		barrier(CLK_LOCAL_MEM_FENCE);
		if(lid < i) {
			base = ((lid + 1) << off) - 1;
			rMem[base] += rMem[base - p];
		}
		p <<= 1;
	}

	/* Allocate */
	barrier(CLK_LOCAL_MEM_FENCE);
	if(lid == 0) {
		total = rMem[p - 1] / l->objSize;
		rMem[p - 1] = clArrayList_grow(l, total);
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	if(rMem[p - 1] == 0)
		return NULL;

	i = 1;

	/* Down-sweep */
	while (p > 1) {
		off = p >> 1;
		if(lid < i) {
			base = (lid * p) + (off - 1);
			that = rMem[base];
			rMem[base] = rMem[base + off];
			rMem[base + off] += that;
		}
		i <<= 1;
		p = off;
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	if(objs > 0)
		return rMem[lid];
	else
		return NULL;
}

uintptr_t
clArrayList_get(clArrayList __global *l, size_t i) {
	size_t cml = 0;

	bool found = false;
	uintptr_t ret;
	char __global *cRet = NULL;
	struct clArrayList_page __global *p = (struct clArrayList_page __global *) l->queue.head;

	while(!found && p != NULL) {
		if(cml + p->count > i) {
			cRet = (char __global *) p;
			cRet += sizeof(struct clArrayList_page);
			cRet += l->objSize * (i - cml);
			ret = (uintptr_t) cRet;
			found = true;
		} else {
			cml += p->count;
			p = (struct clArrayList_page __global *) p->next.next;
		}
	}
	return ret;
}

/*
 * One by one dequeue all the elements and free them. Should be safe to call
 * in parallel, as long as nobody is actually going to use the ArrayList.
 */
void
clArrayList_clear(clArrayList __global *l)
{
	uintptr_t p;

	p = (uintptr_t) dequeue(&l->queue);
	while(p != NULL) {
		free(l->heap, p);
		p = (uintptr_t) dequeue(&l->queue);
	}
}

//__kernel void
//clArrayList_test_top (char __global *arrayList, struct clheap __global *heap)
//{
//	clArrayList __global *l = (clArrayList __global *)arrayList;
//	int pid = 0;
//	int amount;
//	int __global *mainptr, *extraptr;
//	int i, j;
//
//	/* Project global ID to 1D */
//	for(i = 0, j = 1; i < get_work_dim(); i++) {
//		pid += j * get_global_id(i);
//		j *= get_global_size(i);
//	}
//
//	amount = 1 << (pid % 5);
//
//	mainptr = (int __global *) clArrayList_grow(l, amount);
//	if(mainptr)
//		mainptr[0] = pid;
//
//	if(pid == 0) {
//		extraptr = (int __global *) clArrayList_grow(l, 1000);
//		if(extraptr)
//			extraptr[8] = 4919;
//	}
//}
//
//__kernel void
//clArrayList_test_top_local (char __global *arrayList, struct clheap __global *heap)
//{
//	clArrayList __global *l = (clArrayList __global *)arrayList;
//	int pid = 0;
//	int amount;
//	int __global *mainptr, *extraptr;
//	int i, j;
//	uintptr_t __local rMem[128]; /* Whoaverprovisioning!!! */
//
//	/* Project global ID to 1D */
//	for(i = 0, j = 1; i < get_work_dim(); i++) {
//		pid += j * get_global_id(i);
//		j *= get_global_size(i);
//	}
//
//	amount = 1 << (pid % 5);
//
//	mainptr = (int __global *) clArrayList_grow_local(l, amount, rMem);
//	if(mainptr)
//		mainptr[0] = pid;
//
//	if(pid == 0) {
//		extraptr = (int __global *) clArrayList_grow(l, 1000);
//		extraptr[8] = 4919;
//	}
//}
//
//__kernel void
//clArrayList_test_bottom (char __global *arrayList, struct clheap __global *heap)
//{
//	clArrayList __global *l = (clArrayList __global *)arrayList;
//	clArrayList_clear(l);
//}

/* GBarrier is too unstable, this _will_ crash your GPU in many cases */
//__kernel void
//clArrayList_test (char __global *arrayList, struct clheap __global *heap,
//		volatile unsigned int __global *barrier)
//{
//	clArrayList __global *l = (clArrayList __global *)arrayList;
//	unsigned int pid = 0;
//	int amount;
//	int __global *mainptr, *extraptr = NULL;
//	int i, j;
//	uintptr_t __local rMem[128]; /* Whoaverprovisioning!!! */
//
//	/* Project global ID to 1D */
//	for(i = 0, j = 1; i < get_work_dim(); i++) {
//		pid += j * get_global_id(i);
//		j *= get_global_size(i);
//	}
//
//	for(i = 0; i < 100; i++) {
//		amount = 1;// << ((pid + i) % 5);
//
//		mainptr = (int __global *) clArrayList_grow_local(l, amount, rMem);
//		if(mainptr)
//			mainptr[0] = pid;
//
//		if(pid == 0) {
//			extraptr = (int __global *) clArrayList_grow(l, 1000);
//			if(extraptr)
//				extraptr[8] = 4919 << 16;
//		}
//
//		gbarrier(barrier);
//
//		clArrayList_clear(l);
//		gbarrier(barrier);
//
//	}
//}

__kernel void
clArrayList_test_justgrow(void __global *hp, char __global *arrayList)
{
	struct clheap __global *heap = (struct clheap __global *)hp;
	clArrayList __global *al = (clArrayList __global *) arrayList;
	uintptr_t __local rMem[128]; /* Whoaverprovisioning!!! */
	size_t pid = 0, j;
	unsigned int i, amount;
	unsigned int __global *ptr;

	/* Project global ID to 1D */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}

	for(i = 0; i < 10; i++) {
		amount = ((pid + i) % 2) ? 1 : 2;
		ptr = (unsigned int __global *)
				clArrayList_grow_local(al, amount, rMem);
		if(ptr)
			ptr[0] = pid;
	}
}

__kernel void
clArrayList_test_justgrow_malloc(void __global *hp)
{
	struct clheap __global *heap = (struct clheap __global *)hp;
	size_t pid = 0, j;
	unsigned int i, amount;
	unsigned int __global *ptr;

	/* Project global ID to 1D */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}

	for(i = 0; i < 10; i++) {
		amount = ((pid + i) % 2) ? 4 : 8;
		ptr = (unsigned int __global *)malloc(heap, amount);
		if(ptr)
			*ptr = pid;
	}
}
