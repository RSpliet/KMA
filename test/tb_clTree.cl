/*
 * tb_clTree.cl
 *
 *  Created on: 12 mrt. 2013
 *      Author: roy
 */

#include "test/tb_clTree.h"
#include "clheap.h"
#include "clArrayList.h"

struct clGraph_node __global *
clGraph_node_ensure(struct clheap __global *heap, struct clTree __global *tree,
		unsigned int key)
{
	struct clGraph_node __global *node = NULL;

	while(node == NULL) {
		node = (struct clGraph_node __global *) clTree_get(tree, key);
		if(!node) {
			node = (struct clGraph_node __global *)
				malloc(heap, sizeof(struct clGraph_node));
			if(!node)
				continue;
			node->tree.key = key;
			clQueue_init(&node->links);
			mem_fence(CLK_GLOBAL_MEM_FENCE);

			if(!clTree_add(tree, &node->tree)) {
				free(heap, (uintptr_t) node);
				node = NULL;
				mem_fence(CLK_GLOBAL_MEM_FENCE);
			}
		}
	}

	return node;
}

__kernel void
clTree_test(void __global *hp, void __global *pTree,
		struct clTree_link __global *data, unsigned int items)
{
	struct clheap __global *heap = (struct clheap __global *) hp;
	struct clGraph_node __global *source, *sink;
	struct clTree __global *tree = (struct clTree __global *)pTree;
	size_t pid = 0, stride;
	unsigned int i;
	struct clTree_link __global *item;
	struct clGraph_link __global *link;
	unsigned int __global *scratch;
	unsigned int sum;

	for(i = 0, stride = 1; i < get_work_dim(); i++) {
		pid += stride * get_global_id(i);
		stride *= get_global_size(i);
	}

	for(i = pid; i < items; i += stride) {
		item = &data[i];
		source = clGraph_node_ensure(heap, tree, item->source);
		sink = clGraph_node_ensure(heap, tree, item->sink);
		link = (struct clGraph_link __global *)
				malloc(heap, sizeof(struct clGraph_link));
		link->sink = sink;
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		enqueue(&source->links, &link->q);
	}

	/*barrier(CLK_LOCAL_MEM_FENCE);

	if(pid == 0) {
		 scratch = (unsigned int __global *)
				 malloc(heap, 4000); // Get me a scratchpad
		 i = 0;
		 source = clGraph_node_ensure(heap, tree, 55);
		 link = (struct clGraph_link __global *) source->links.head;
		 while(link != NULL) {
			 source = link->sink;
			 scratch[i] = source->tree.key;
			 link = (struct clGraph_link __global *) link->q.next;
			 i++;
		 }
	}*/
}

__kernel void
clTree_test_cache(void __global *hp, void __global *pTree,
		struct clTree_link __global *data, unsigned int items)
{
	struct clheap __global *heap = (struct clheap __global *) hp;
	struct clGraph_node __global *source = NULL, *sink = NULL;
	struct clTree __global *tree = (struct clTree __global *)pTree;
	size_t pid = 0, stride;
	unsigned int i;
	struct clTree_link __global *item;
	struct clGraph_link __global *link;

	struct clGraph_node __global *cache = NULL;

	for(i = 0, stride = 1; i < get_work_dim(); i++) {
		pid += stride * get_global_id(i);
		stride *= get_global_size(i);
	}

	for(i = pid; i < items; i += stride) {
		item = &data[i];
		sink = NULL;
		source = NULL;

		/* First ensure the source exists
		 * Yes this is a copy paste from node_ensure... can't use static
		 * var as cache so had to bring it top-level */

		while(!source) {
			source = (struct clGraph_node __global *) clTree_get(tree, item->source);
			if(!source){

				if(cache) {
					source = cache;
					cache = NULL;
				} else {
					source = (struct clGraph_node __global *)
						malloc(heap, sizeof(struct clGraph_node));
					if(!source)
						return;
				}
				source->tree.key = item->source;
				clQueue_init(&source->links);
				mem_fence(CLK_GLOBAL_MEM_FENCE);
				if(!clTree_add(tree, &source->tree)) {
					cache = source;
					source = NULL;
				}
			}
		}

		/* Then sink */
		while(!sink) {
			sink = (struct clGraph_node __global *) clTree_get(tree, item->sink);

			if(!sink) {
				if(cache) {
					sink = cache;
					cache = NULL;
				} else {
					sink = (struct clGraph_node __global *)
						malloc(heap, sizeof(struct clGraph_node));
					if(!sink)
						return;
				}
				sink->tree.key = item->sink;
				clQueue_init(&sink->links);
				mem_fence(CLK_GLOBAL_MEM_FENCE);
				if(!clTree_add(tree, &sink->tree)) {
					cache = sink;
					sink = NULL;
				}
			}
		}

		/* Linking is easier */
		link = (struct clGraph_link __global *)
				malloc(heap, sizeof(struct clGraph_link));
		if(!link)
			return;
		link->sink = sink;
		enqueue(&source->links, &link->q);
	}

	if(cache)
		free(heap, cache);
}

__kernel void
clTree_test_al(void __global *al, void __global *alLink, void __global *pTree,
		struct clTree_link __global *data, unsigned int items)
{
	struct clArrayList __global *atree = (struct clArrayList __global *) al;
	struct clArrayList __global *alinks = (struct clArrayList __global *) alLink;
	struct clGraph_node __global *source = NULL, *sink = NULL;
	struct clTree __global *tree = (struct clTree __global *)pTree;
	size_t pid = 0, stride;
	unsigned int i, alloc, itemsCeil;
	uintptr_t __local rMem[128];
	uintptr_t node;
	struct clTree_link __global *item;
	struct clGraph_link __global *link;

	struct clGraph_node __global *cache = NULL;

	for(i = 0, stride = 1; i < get_work_dim(); i++) {
		pid += stride * get_global_id(i);
		stride *= get_global_size(i);
	}

	itemsCeil = items % stride;
	if(itemsCeil)
		itemsCeil = stride - itemsCeil;
	itemsCeil += items;

	for(i = pid; i < itemsCeil; i += stride) {
		if(i < items) {
			item = &data[i];
			source = (struct clGraph_node __global *) clTree_get(tree, item->source);
			if(!source && !cache)
				alloc = 1;
			else
				alloc = 0;
		} else {
			item = NULL;
			source = 0xFFFFFFFF;
			sink = 0xFFFFFFFF;
			alloc = 0;
		}

		/* First ensure the source exists
		 * Yes this is a copy paste from node_ensure... can't use static
		 * var as cache so had to bring it top-level */
		node = clArrayList_grow_local(atree, alloc, rMem);
		if(!cache)
			cache = (struct clGraph_node __global *) node;

		while(!source) {
			source = cache;
			cache = NULL;

			source->tree.key = item->source;
			clQueue_init(&source->links);
			mem_fence(CLK_GLOBAL_MEM_FENCE);
			if(!clTree_add(tree, &source->tree)) {
				cache = source;
				source = (struct clGraph_node __global *) clTree_get(tree, item->source);
			}
		}

		/* Then sink */
		if(item) {
			sink = (struct clGraph_node __global *) clTree_get(tree, item->sink);
			if(!sink && !cache)
				alloc = 1;
			else
				alloc = 0;
		}

		node = clArrayList_grow_local(atree, alloc, rMem);
		if(!cache)
			cache = (struct clGraph_node __global *) node;

		while(!sink) {
			sink = cache;
			cache = NULL;

			sink->tree.key = item->sink;
			clQueue_init(&sink->links);
			mem_fence(CLK_GLOBAL_MEM_FENCE);
			if(!clTree_add(tree, &sink->tree)) {
				cache = sink;
				sink = (struct clGraph_node __global *) clTree_get(tree, item->sink);
			}
		}

		/* Linking is easier */
		link = (struct clGraph_link __global *)
				clArrayList_grow_local(alinks, item ? 1 : 0, rMem);
		if(!item)
			continue;
		if(!link)
			return;
		link->sink = sink;
		enqueue(&source->links, &link->q);
	}
}
