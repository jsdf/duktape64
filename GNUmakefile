#  squaresdemo Makefile
#  this is just the standard nusystem makefile

include $(ROOT)/usr/include/make/PRdefs

NUSYSINCDIR  = $(ROOT)/usr/include/nusys
NUSYSLIBDIR  = $(ROOT)/usr/lib/nusys
NUSTDINCDIR  = $(ROOT)/usr/include/nustd
NUSTDLIBDIR  = $(ROOT)/usr/lib/nustd
KMCINCDIR  = $(ROOT)/usr/include/kmc
GCCLIBDIR = $(N64_TOOLCHAIN)/lib/gcc/mips64-elf/9.1.0
GCCINCDIR = $(N64_TOOLCHAIN)/lib/gcc/mips64-elf/9.1.0/include

LIB = $(ROOT)/usr/lib
LPR = $(LIB)/PR
INC = $(ROOT)/usr/include

LCDEFS =	-DNU_DEBUG -DDEBUG -DF3DEX_GBI_2
# LCINCS =	-I. -I$(NUSYSINCDIR) -I$(NUSTDINCDIR) -I$(ROOT)/usr/include/PR 
LCINCS =	-I.  -I$(GCCINCDIR) -I$(NUSYSINCDIR) -I$(NUSTDINCDIR) -I$(ROOT)/usr/include/PR -I$(INC)

LCOPTS =	-G 0 -include "ultra64.h" -std=gnu90  -Wno-comment -Werror-implicit-function-declaration 
LDFLAGS = $(MKDEPOPT) -L$(LIB) -L$(NUSYSLIBDIR) -L$(NUSTDLIBDIR) -L$(GCCLIBDIR) -lnusys_d -lnustd_d  -lultra_d  -lgcc

OPTIMIZER =	-g

APP =		squaresdemo.out

TARGETS =	squaresdemo.n64

HFILES =	graphic.h

CODEFILES = main.c graphic.c gfxinit.c stage00.c  ed64io_everdrive.c  ed64io_fault.c ed64io_sys.c   ed64io_usb.c libc_shims.c

CODEOBJECTS =	$(CODEFILES:.c=.o)  $(NUSYSLIBDIR)/nusys.o

DATAFILES   =     mem_heap.c   duktape.c duk_console.c

EXTRA_SYMBOLS =  -u acos -u asin -u atan -u atan2 -u ceil -u cos -u exp -u floor -u fmod -u log -u longjmp -u memcmp -u memmove -u pow -u realloc -u setjmp -u sin -u sprintf -u sqrt -u strcmp -u strncmp -u tan

DATAOBJECTS =	$(DATAFILES:.c=.o)

CODESEGMENT =	codesegment.o

OBJECTS =	$(CODESEGMENT) $(DATAOBJECTS)


default:        $(TARGETS)

include $(COMMONRULES)

$(CODESEGMENT):	$(CODEOBJECTS)
		$(LD) -o $(CODESEGMENT) -r $(CODEOBJECTS) $(LDFLAGS) $(EXTRA_SYMBOLS)

$(TARGETS):	$(OBJECTS)
		$(MAKEROM) spec -I$(NUSYSINCDIR) -r $(TARGETS) -s 9 -e $(APP)  #-d  -m 
		makemask $(TARGETS)

gcc-predefines:
		mips64-elf-gcc -dM -E - < /dev/null

