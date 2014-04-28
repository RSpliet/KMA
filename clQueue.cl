/**
 * clQueue.cl
 * Queue implementation in OpenCL
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
#include "clQueue.h"

/* Lines get too long... */
#define uptr uintptr_t
#define POISON 1

__kernel void
clQueue_init(clqueue __global *q)
{
	q->head = NULL;
	q->tail = NULL;
}

/**
 * enqueue() - Add an item to the queue
 * @q: Queue to add the item to
 * @item: Item to add to the queue
 * @return 1 iff enqueuing succeeded, 0 otherwise
 */
int
enqueue(clqueue __global *q, clqueue_item __global *item)
{
	clqueue_item __global *tail;
	unsigned int i = 0, ret = 0;

	if(item == NULL)
		return 0;

	item->next = NULL;
	mem_fence(CLK_GLOBAL_MEM_FENCE);
	loop_infinite(i) {
		/* If tail == NULL, CAS me into there. */
		tail = (clqueue_item __global *)
				atom_cmpxchg(&q->tail, NULL, (uptr) item);
		mem_fence(CLK_GLOBAL_MEM_FENCE);

		if(tail == NULL) {
			atom_cmpxchg(&q->head, NULL, (uptr) item);
			ret = 1;
			break;
		}

		/* If not, add me to the tail of the list.
		 * on failure someone else beat me to it: retry! */
		if(atom_cmpxchg(&tail->next, NULL, (uptr) item) == NULL) {
			mem_fence(CLK_GLOBAL_MEM_FENCE);
			while(atom_cmpxchg(&q->tail, (uptr) tail, (uptr) item) != (uptr) tail);
			ret = 1;
			break;
		}
	}

	mem_fence(CLK_GLOBAL_MEM_FENCE);
	return ret;
}

/**
 * dequeue() - Remove and return the next item in the queue
 * @q: Queue to get the item from
 * @return The next queue item, NULL on failure.
 */
clqueue_item __global *
dequeue(clqueue __global *q)
{
	clqueue_item __global *item, *next;
	unsigned int i = 0;

	loop_infinite(i) {
		item = (clqueue_item __global *) q->head;
		if(item == NULL)
			return NULL;

		/* On GPGPU (sort-of-fixed memory) &item still exists even if
		 * no longer in the list. The following is safe here, but not
		 * on a CPU. If next actually contains sth else than a next
		 * ptr, head can't be in q->head thus atom_cmpxchg fails.*/
		next = (clqueue_item __global *)atom_xchg(&item->next, POISON);
		mem_fence(CLK_GLOBAL_MEM_FENCE);

		/* First garble the next pointer, keeps others hands off it */
		if((uptr)next == POISON)
			continue;

		if(atom_cmpxchg(&q->head, (uptr) item, (uptr) next) == (uptr) item) {
			mem_fence(CLK_GLOBAL_MEM_FENCE);

			/* If I'm head *and* tail I must be the only block.*/
			atom_cmpxchg(&q->tail, (uptr) item, NULL);
			mem_fence(CLK_GLOBAL_MEM_FENCE);
			return item;
		} else {
			/* Restore next pointer and pretend nothing happened... */
			atom_cmpxchg(&item->next, POISON, (uptr) next);
			mem_fence(CLK_GLOBAL_MEM_FENCE);
		}
	}
	return NULL;
}

/* Test enqueueing. */
__kernel void
clQueue_test_enqueue(clqueue __global *q, unsigned int __global *mem)
{
	unsigned int pid = 0, i, j;
	clqueue_item __global *item;

	/* First find global unique ID */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}

	item = (clqueue_item __global *) &mem[pid * 2];

	enqueue(q, item);
	mem[pid * 2 + 1] = (uintptr_t) item;
}

/* Test enqueueing, dequeueing. */
__kernel void
clQueue_test_dequeue(clqueue __global *q, unsigned int __global *mem)
{
	unsigned int pid = 0, i, j;
	clqueue_item __global *item;

	/* First find global unique ID */
	for(i = 0, j = 1; i < get_work_dim(); i++) {
		pid += j * get_global_id(i);
		j *= get_global_size(i);
	}

	item = (clqueue_item __global *) &mem[pid * 2];
	mem[pid * 2 + 1] = (uintptr_t) item;
	enqueue(q, item);

	/* Do the shuffle */
	for(i = 0; i < 10; i++) {
		item = (clqueue_item __global *) dequeue(q);
		if(item != NULL) {
			//mychunk[1] = pid;
			enqueue(q, item);
		}
	}
}
