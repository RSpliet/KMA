/**
 * clIndexedQueue.cl
 * Indexed queue implementation in OpenCL
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
#include "clIndexedQueue.h"

/* Lines get too long... */
#define uptr uintptr_t
#define POISON 1

struct mem_item {
	clIndexedQueue_item q;
	uint32_t filler;
	uintptr_t thisobj;
};

/**
 * clIndexedQueue_ptr2idx() - Convert a pointer and tag to an index value
 * @q: Indexed queue object
 * @ptr: Pointer to convert
 * This helper function produces a uint32_t variable with the following bit-layout:
 * 0     Poison
 * 21:1  Index (base + stride * (n-1)
 * 32:22 Tag
 *
 * Returns 0 on failure.
 */
uint32_t
clIndexedQueue_ptr2idx(clIndexedQueue __global *q, void __global *ptr)
{
	uptr idx = (uptr) ptr;

	/* Before the base isn't possible */
	if(idx < (uptr) q->base)
		return 0;

	idx -= (uptr) q->base;
	/* Does it align to a stride boundary nicely? */
	if(idx & ((1 << q->stride) - 1))
		return 0;

	idx >>= q->stride;
	idx++;
	/* Does the index still fit? */
	if(idx > ((1 << 20) - 1))
		return 0;

	return (uint32_t) idx << 1;
}

/**
 * clIndexedQueue_idx2ptr() - Convert a pointer and tag to an index value
 * @q: Indexed queue object
 * @idx: Index to convert
 */
inline void __global*
clIndexedQueue_idx2ptr(clIndexedQueue __global *q, uint32_t idx)
{
	size_t i = idx;
	idx >>= 1;
	idx &= 0xfffff;
	if(idx == 0)
		return 0;

	idx--;

	i = idx;
	i <<= q->stride;

	return &q->base[i];
}

#define PTR(i,t) ((i & 0x1fffe) | ((((t)+1) & 0x7ff) << 22))
#define TAG(i) ((i >> 22) & 0x7ff)
#define IDX(i) ((i & 0x1fffe) >> 1)

__kernel void
clIndexedQueue_init(void __global *queue, void __global *base,
		uint32_t stride_l2, void __global *i)
{
	clIndexedQueue __global *q = (clIndexedQueue __global *) queue;
	uint32_t idx, tag;
	clIndexedQueue_item __global *item = (clIndexedQueue_item __global *)i;

	q->base = base;
	q->stride = stride_l2;

	tag = TAG(item->next);
	item->next = PTR(0, tag);
	idx = clIndexedQueue_ptr2idx(q, item);

	q->head = PTR(idx, tag);
	q->tail = PTR(idx, tag);
	mem_fence(CLK_GLOBAL_MEM_FENCE);
}

/**
 * idxd_enqueue() - Add an item to the indexed queue
 * @q: Queue to add the item to
 * @item: Item to add to the queue
 * @return 1 iff enqueuing succeeded, 0 otherwise
 */
int
idxd_enqueue(clIndexedQueue __global *q, clIndexedQueue_item __global *item)
{
	clIndexedQueue_item __global *tail;
	unsigned int ret = 0;
	uint32_t idx, tag,
	tailidx,
	nextidx;

	if(item == NULL)
		return 0;

	tag = TAG(item->next);
	item->next = PTR(0, tag-1);
	tag++;
	idx = clIndexedQueue_ptr2idx(q, item);

	while(1) {
		tailidx = q->tail;
		mem_fence(CLK_GLOBAL_MEM_FENCE);

		tail = (clIndexedQueue_item __global *)
				clIndexedQueue_idx2ptr(q, tailidx);
		nextidx = tail->next;
		mem_fence(CLK_GLOBAL_MEM_FENCE);

		/* Did I read a consistent state? */
		if(q->tail == tailidx) {
			if(IDX(nextidx) == 0) {
				tag = TAG(nextidx);
				if(atom_cmpxchg(&tail->next, nextidx, PTR(idx, tag)) == nextidx) {
					mem_fence(CLK_GLOBAL_MEM_FENCE);
					ret = 1;
					break;
				}
			} else {
				tag = TAG(tailidx);
				atom_cmpxchg(&q->tail, tailidx, PTR(nextidx, tag));
				mem_fence(CLK_GLOBAL_MEM_FENCE);
			}
		}
	}
	tag = TAG(tailidx);
	atom_cmpxchg(&q->tail, tailidx, PTR(idx, tag));
	mem_fence(CLK_GLOBAL_MEM_FENCE);

	return ret;
}

/**
 * idxd_dequeue() - Remove and return the next item in the queue
 * @q: Queue to get the item from
 * @return The next queue item, NULL on failure.
 */
clIndexedQueue_item __global *
idxd_dequeue(clIndexedQueue __global *q)
{
	clIndexedQueue_item __global *head;
	uint32_t 	tag,
			nextidx,
			tailidx,
			headidx;

	while(1) {
		headidx = q->head;
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		head = (clIndexedQueue_item __global *)
				clIndexedQueue_idx2ptr(q, headidx);
		tailidx = q->tail;
		nextidx = head->next;
		mem_fence(CLK_GLOBAL_MEM_FENCE);

		if(headidx == q->head) {
			if(IDX(headidx) == IDX(tailidx)) {
				if(IDX(nextidx) == 0) {
					return NULL;
				}
				tag = TAG(tailidx);
				atom_cmpxchg(&q->tail, tailidx, PTR(nextidx, tag));
				mem_fence(CLK_GLOBAL_MEM_FENCE);
			} else {
				tag = TAG(headidx);
				if(atom_cmpxchg(&q->head, headidx, PTR(nextidx, tag)) == headidx)
					break;
			}
		}

	}
	mem_fence(CLK_GLOBAL_MEM_FENCE);
	return head;
}

/* Test enqueueing. */
__kernel void
clIndexedQueue_test_enqueue(void __global *queue, unsigned int __global *mem)
{
	clIndexedQueue __global *q = (clIndexedQueue __global *) queue;
	size_t pid = 0, j;
	unsigned int i;
	struct mem_item __global *item, *prev;

	/* First find global unique ID */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}
	pid++;

	item = (struct mem_item __global *) &mem[pid * 4];

	idxd_enqueue(q, &item->q);

	if(pid == 1) {
		prev = (struct mem_item __global *) &mem[0];
		prev->thisobj = (uintptr_t) &mem[0];
	}
	item->thisobj = (uintptr_t) item;
}

/* Test enqueueing, dequeueing. */
__kernel void
clIndexedQueue_test_dequeue(void __global *queue, unsigned int __global *mem)
{
	clIndexedQueue __global *q = (clIndexedQueue __global *) queue;
	size_t pid = 0, j;
	unsigned int i;
	struct mem_item __global *item, *prev;

	/* First find global unique ID */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}
	pid++;

	item = (struct mem_item __global *) &mem[pid * 4];
	item->thisobj = (uintptr_t) item;
	if(pid == 1) {
		prev = (struct mem_item __global *) &mem[0];
		prev->thisobj = (uintptr_t) &mem[0];
	}
	idxd_enqueue(q, (clIndexedQueue_item __global *) item);

	/* Do the shuffle */
	for(i = 0; i < 10; i++) {
		item = (struct mem_item __global *) idxd_dequeue(q);
		if(item != NULL) {
			if(!idxd_enqueue(q, (clIndexedQueue_item __global *) item)) {
			}
		} else {
		}
	}
}
