#include <nusys.h>

#include "malloc.h"

#include "stage00.h"

#include "ed64io.h"
#include "mem_heap.h"

NUContData contdata[1];  // storage for controller 1 inputs

#define EXTERN_SEGMENT_WITH_BSS(name)                         \
  extern char _##name##SegmentStart[], _##name##SegmentEnd[], \
      _##name##SegmentRomStart[], _##name##SegmentRomEnd[],   \
      _##name##SegmentBssStart[], _##name##SegmentBssEnd[]

void Rom2Ram(void*, void*, s32);

EXTERN_SEGMENT_WITH_BSS(duktape);
// EXTERN_SEGMENT_WITH_BSS(nextmb);
EXTERN_SEGMENT_WITH_BSS(memheap);

int systemHeapMemoryInit(void) {
  int initHeapResult;
  void* mallocRes;

  if (osGetMemSize() == 0x00800000) {
    ed64PrintfSync("have expansion pack\n");
  } else {
    ed64PrintfSync("expansion pack missing\n");
  }

  Rom2Ram(_memheapSegmentRomStart, _memheapSegmentStart,
          _memheapSegmentRomEnd - _memheapSegmentRomStart);
  bzero(_memheapSegmentBssStart,
        _memheapSegmentBssEnd - _memheapSegmentBssStart);

  ed64PrintfSync("init heap at %p\n", _memheapSegmentStart);
  /* Reserve system heap memory */
  initHeapResult = InitHeap(mem_heap, MEM_HEAP_SIZE);

  if (initHeapResult == -1) {
    ed64PrintfSync("failed to init heap\n");
  } else {
    ed64PrintfSync("init heap success, allocated=%d\n", MEM_HEAP_SIZE);
    mallocRes = malloc(sizeof(long));
    if (mallocRes) {
      ed64PrintfSync("malloc works=%p\n", mallocRes);
    } else {
      ed64PrintfSync("malloc failed\n");
    }
    free(mallocRes);
  }

  return 0;
}

#include "limits.h"
int intMax = INT_MAX;

void mainproc(void) {
  evd_init();

  // start thread which will catch and log errors
  ed64StartFaultHandlerThread(NU_GFX_TASKMGR_THREAD_PRI);

  // handler for libultra errors
  // ed64RegisterOSErrorHandler();

  // overwrite osSyncPrintf impl
  ed64ReplaceOSSyncPrintf();

  systemHeapMemoryInit();

  // load in the duktape segment into higher memory
  // Rom2Ram(_nextmbSegmentRomStart, _nextmbSegmentStart,
  //         _nextmbSegmentRomEnd - _nextmbSegmentRomStart);
  Rom2Ram(_duktapeSegmentRomStart, _duktapeSegmentStart,
          _duktapeSegmentRomEnd - _duktapeSegmentRomStart);
  bzero(_duktapeSegmentBssStart,
        _duktapeSegmentBssEnd - _duktapeSegmentBssStart);

  // initialize the graphics system
  nuGfxInit();

  // initialize the controller manager
  nuContInit();

  // initialize the level
  initStage00();

  // set the update+draw callback to be called every frame
  nuGfxFuncSet((NUGfxFunc)stage00);

  // enable video output
  nuGfxDisplayOn();

  // send this thread into an infinite loop
  while (1)
    ;
}

void Rom2Ram(void* from_addr, void* to_addr, s32 seq_size) {
  /* Cannot transfer if size is an odd number, so make it an even number. */
  if (seq_size & 0x00000001)
    seq_size++;

  nuPiReadRom((u32)from_addr, to_addr, seq_size);
}
