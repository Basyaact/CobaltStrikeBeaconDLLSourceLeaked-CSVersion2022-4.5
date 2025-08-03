#include <Windows.h>
#include <stdlib.h>
#include "track_memory.h"

typedef struct {
	void * ptr;
	size_t size;
	int type;
	BOOL mask;
	void(*cleanup)(void *);
} MEMORY_RECORD;

#define TRACKER_SIZE_INCREASE 25

static MEMORY_RECORD * memoryTracker = NULL;
static size_t memoryTrackerCapacity = 0;
static size_t memoryTrackerLength = 0;

static HEAP_RECORD * heapRecords = NULL;
static BOOL   addedNewHeapRecord = TRUE;

void track_memory_add(void * ptr, size_t size, int type, BOOL mask, void(*cleanup)(void *)) {
	
	/* check if there is enough capacity for this new memory item */
	if (memoryTrackerLength + 1 >= memoryTrackerCapacity) {
		if (memoryTracker == NULL) {
			// dlog("track_memory_add -- malloc initial size");
			memoryTracker = (MEMORY_RECORD *)malloc(sizeof(MEMORY_RECORD) * TRACKER_SIZE_INCREASE);
		}
		else {
			// dlog("track_memory_add -- realloc current length: %lu capacity: %lu ", memoryTrackerLength, memoryTrackerCapacity);
			memoryTracker = (MEMORY_RECORD *)realloc(memoryTracker, sizeof(MEMORY_RECORD) * (memoryTrackerCapacity + TRACKER_SIZE_INCREASE));
		}
		memset(&memoryTracker[memoryTrackerCapacity], 0, sizeof(MEMORY_RECORD) * TRACKER_SIZE_INCREASE);
		memoryTrackerCapacity += TRACKER_SIZE_INCREASE;
		// dlog("track_memory_add -- realloc new capacity: %lu ", memoryTrackerCapacity);
	}

	/* Add the new memory item to track */
	// dlog("track_memory_add -- new item at %p of size %lu mask: %d", ptr, size, mask);
	memoryTracker[memoryTrackerLength].ptr = ptr;
	memoryTracker[memoryTrackerLength].size = size;
	memoryTracker[memoryTrackerLength].type = type;
	memoryTracker[memoryTrackerLength].mask = mask;
	memoryTracker[memoryTrackerLength].cleanup = cleanup;
	++memoryTrackerLength;

	if (mask) {
		addedNewHeapRecord = TRUE;
	}
}

void track_memory_cleanup() {
	size_t i;
	// dlog("track_memory_cleanup -- items being tracked: %lu ", memoryTrackerLength);
	for (i = 0; i < memoryTrackerLength; ++i) {
		if (memoryTracker[i].cleanup != NULL) {
			// dlog("track_memory_cleanup -- call cleanup function: %p ", memoryTracker[i].cleanup);
			memoryTracker[i].cleanup(memoryTracker[i].ptr);
		}
		else if (memoryTracker[i].type == TRACK_MEMORY_MALLOC) {
			// dlog("track_memory_cleanup -- call memset & free function: %p  size: %lu", memoryTracker[i].ptr, memoryTracker[i].size);
			memset(memoryTracker[i].ptr, 0, memoryTracker[i].size);
			free(memoryTracker[i].ptr);
		}
		else if (memoryTracker[i].type == TRACK_MEMORY_VALLOC) {
			// dlog("track_memory_cleanup -- call memset & VirtualFree function: %p", memoryTracker[i].ptr);
			memset(memoryTracker[i].ptr, 0, memoryTracker[i].size);
			VirtualFree(memoryTracker[i].ptr, 0, MEM_RELEASE);
		}
	}

	/* Cleanup the tracker */
	// dlog("track_memory_cleanup -- Cleanup the tracker -- call free function: %p", memoryTracker);
	if (memoryTracker) {
		free(memoryTracker);
	}
	if (heapRecords) {
		free(heapRecords);
	}
	memoryTrackerCapacity = 0;
	memoryTrackerLength = 0;
	addedNewHeapRecord = TRUE;
}

HEAP_RECORD * track_memory_get_heap_records_to_mask() {
	size_t i, count;

	if (addedNewHeapRecord == FALSE && heapRecords != NULL) {
		// dlog("track_memory_get_heap_records_to_mask -- heapRecords to mask has not changed");
		return heapRecords;
	}

	/* Determine how many sections need to be masked. */
	for (i = 0, count = 0; i < memoryTrackerLength; ++i) {
		if (memoryTracker[i].mask) {
			++count;
		}
	}
	// dlog("track_memory_get_heap_records_to_mask -- masking %lu heap sections", count);

	/* sections is a list of DWORDs containing a begin and end DWORD for each memory section. The list ends with 0, 0 */
	if (heapRecords != NULL) {
		free(heapRecords);
	}
	heapRecords = (HEAP_RECORD *)malloc((count + 1) * sizeof(HEAP_RECORD));
	for (i = 0, count = 0; i < memoryTrackerLength; ++i) {
		if (memoryTracker[i].mask) {
			heapRecords[count].ptr = (char *)memoryTracker[i].ptr;
			heapRecords[count].size = memoryTracker[i].size;
			// dlog("track_memory_get_heap_records_to_mask -- start: %p  size: %lu", heapRecords[count].ptr, heapRecords[count].size);
			count++;
		}
	}

	/* End the list of heap memory sections to mask */
	heapRecords[count].ptr = NULL;
	heapRecords[count].size = 0;

	/* Set flag to use this list of heap sections until a new section is added. */
	addedNewHeapRecord = FALSE;

	return heapRecords;
}
