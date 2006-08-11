/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "heap.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>


static inline size_t nextPower2(size_t x);


static void
Heap_resize(Heap *heap, size_t size) {
	heap->entries = realloc(heap->entries, size * sizeof (HeapValue *));
	heap->size = size;
}

Heap *
Heap_new(HeapValue_Comparator comparator, size_t initialSize, size_t minSize,
		double minFillQuotient) {
	Heap *heap;

	assert(minFillQuotient >= 0.0);

	heap = malloc(sizeof (Heap));

	if (initialSize < minSize)
		initialSize = minSize;

	heap->comparator = comparator;
	heap->minFillQuotient = minFillQuotient;
	heap->size = nextPower2(initialSize);
	heap->minFill = (size_t) ceil(((double) (heap->size >> 1))
			* heap->minFillQuotient);
	heap->entries = malloc(heap->size * sizeof (HeapValue *));
	heap->numEntries = 0;

	return heap;
}

void
Heap_delete(Heap *heap) {
	free(heap->entries);
	free(heap);
}

void
Heap_add(Heap *heap, HeapValue *value) {
	size_t i;

	if (heap->numEntries >= heap->size)
		Heap_resize(heap, heap->size * 2);

	i = heap->numEntries;
	heap->numEntries++;

	while (i > 0) {
		size_t parentI = (i - 1) / 2;
		if (heap->comparator(heap->entries[i], heap->entries[parentI]) <= 0)
			break;

		heap->entries[i] = heap->entries[parentI];
		heap->entries[i]->index = i;
		i = parentI;
	}
	heap->entries[i] = value;
	heap->entries[i]->index = i;
}

HeapValue *
Heap_first(const Heap *heap) {
	assert(heap->numEntries > 0);

	return heap->entries[0];
}

static inline void
swapEntries(Heap *heap, size_t i0, size_t i1) {
	HeapValue *temp = heap->entries[i0];
	heap->entries[i0] = heap->entries[i1];
	heap->entries[i0]->index = i0;
	heap->entries[i1] = temp;
	heap->entries[i1]->index = i1;
}

static void
Heap_removeByIndex(Heap *heap, size_t i) {
	assert(heap->numEntries > i);

	heap->numEntries--;

	if (heap->numEntries != 0) {
		heap->entries[i] = heap->entries[heap->numEntries];
		heap->entries[i]->index = i;

		// Restore the heap invariant:
		for (;;) {
			size_t leftI = i * 2 + 1;
			size_t rightI = leftI + 1;
			size_t newI;

			if (rightI < heap->numEntries) {
				// rightI is unused.
				if (leftI < heap->numEntries)
					break;
				// leftI is used, rightI is unused.
				if (heap->comparator(heap->entries[i],
						heap->entries[leftI]) < 0)
					swapEntries(heap, i, leftI);
				break;
			}

			newI = (heap->comparator(heap->entries[leftI],
					heap->entries[rightI]) >= 0) ? leftI : rightI;
		
			if (heap->comparator(heap->entries[i], heap->entries[newI]) >= 0)
				break;

			swapEntries(heap, i, newI);
			i = newI;
		}
	}

	// Resize if necessary:
	if (heap->numEntries < heap->minFill &&
			heap->numEntries > heap->minSize)
		Heap_resize(heap, heap->size / 2);
}

HeapValue *
Heap_pop(Heap *heap) {
	HeapValue *result;

	assert(heap->numEntries > 0);

	result = heap->entries[0];
	Heap_removeByIndex(heap, 0);

	return result;
}

size_t
Heap_count(const Heap *heap) {
	return heap->numEntries;
}

bool
Heap_hasMore(const Heap *heap) {
	return heap->numEntries > 0;
}

void
Heap_remove(Heap *heap, HeapValue *value) {
	Heap_removeByIndex(heap, value->index);
}

// Adapted from "Hackers Delight"
// Returns the smallest power of two greater or equal to x.
static inline size_t
nextPower2(size_t x) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
#	if (SIZE_MAX > 0xffff)
		x |= x >> 16;
#		if (SIZE_MAX > 0xffffffff)
			x |= x >> 32;
#		endif
#	endif
	return x + 1;
}


