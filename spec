/*
	ROM spec file

	Main memory map

  	0x80000000  exception vectors, ...
  	0x80000400  zbuffer (size 320*240*2)
  	0x80025c00  codesegment
	      :  
  	0x8038F800  cfb 16b 3buffer (size 320*240*2*3)

*/

#include <nusys.h>

/* Use all graphic microcodes */
beginseg
	name	"code"
	flags	BOOT OBJECT
	entry 	nuBoot
	address NU_SPEC_BOOT_ADDR
        stack   NU_SPEC_BOOT_STACK
	include "codesegment.o"
	include "$(ROOT)/usr/lib/PR/rspboot.o"
	include "$(ROOT)/usr/lib/PR/gspF3DEX2.fifo.o"
	include "$(ROOT)/usr/lib/PR/gspL3DEX2.fifo.o"
	include "$(ROOT)/usr/lib/PR/gspF3DEX2.Rej.fifo.o"
        include "$(ROOT)/usr/lib/PR/gspF3DEX2.NoN.fifo.o"
        include "$(ROOT)/usr/lib/PR/gspF3DLX2.Rej.fifo.o"
	include "$(ROOT)/usr/lib/PR/gspS2DEX2.fifo.o"
endseg

beginseg
  name  "duktape"
  flags OBJECT
  after "code"
  include "duktape.o"
  include "duk_console.o"
endseg

beginseg
  name  "memheap"
  flags OBJECT
  address 0x80400000
  include "mem_heap.o"
endseg

/*
beginseg
  name  "nextmb"
  flags OBJECT
  size 0x100000
  address 0x80400000
  include "mem_heap.o"
endseg
*/

beginwave
  name  "squaresdemo"
  include "code" 
  include "duktape"
  // include  "nextmb"
  include  "memheap"
endwave

/*
beginwave
  name  "squaresdemoduktape"
  include "code" 
  include "duktape"
  include  "memheap"
endwave
*/