#  squaresdemo Makefile
#  this is just the standard nusystem makefile

include $(ROOT)/usr/include/make/PRdefs

N64KITDIR    = c:\nintendo\n64kit
NUSYSINCDIR  = $(N64KITDIR)/nusys/include
NUSYSLIBDIR  = $(N64KITDIR)/nusys/lib
NUSTDINCDIR = $(N64KITDIR)/nustd/include
NUSTDLIBDIR = $(N64KITDIR)/nustd/lib

LIB = $(ROOT)/usr/lib
LPR = $(LIB)/PR
INC = $(ROOT)/usr/include
CC  = gcc
LD  = ld
MAKEROM = mild

LCDEFS =  -DNU_DEBUG -DEXEGCC -DF3DEX_GBI_2
LCINCS =  -I. -nostdinc -I- -I$(NUSTDINCDIR) -I$(NUSYSINCDIR) -I$(ROOT)/usr/include/PR
LCOPTS =  -G 0
LDFLAGS = $(MKDEPOPT) -L$(LIB) -L$(NUSYSLIBDIR) -L$(NUSTDLIBDIR) -L$(GCCDIR)/mipse/lib -lnusys_d -lnustd_d -lgultra_d -lkmc

OPTIMIZER = -g

APP =   squaresdemo.out

TARGETS = squaresdemo.n64

HFILES =  graphic.h

CODEFILES = main.c graphic.c gfxinit.c stage00.c  ed64io_everdrive.c  ed64io_fault.c ed64io_sys.c   ed64io_usb.c libc_shims.c

CODEOBJECTS = $(CODEFILES:.c=.o)  $(NUSYSLIBDIR)/nusys.o


DATAFILES   =     mem_heap.c   duktape.c duk_console.c

EXTRA_SYMBOLS =  -u acos -u asin -u atan -u atan2 -u ceil -u cos -u exp -u floor -u fmod -u log -u longjmp -u memcmp -u memmove -u pow -u realloc -u setjmp -u sin -u sprintf -u sqrt -u strcmp -u strncmp -u tan

DATAOBJECTS = $(DATAFILES:.c=.o)

CODESEGMENT = codesegment.o

OBJECTS = $(CODESEGMENT) $(DATAOBJECTS)


default:        $(TARGETS)

include $(COMMONRULES)

$(CODESEGMENT): $(CODEOBJECTS)
    $(LD) -o $(CODESEGMENT) -r $(CODEOBJECTS) $(LDFLAGS) $(EXTRA_SYMBOLS)

$(TARGETS): $(OBJECTS)
    $(MAKEROM) spec -I$(NUSYSINCDIR) -r $(TARGETS) -e $(APP) 
    makemask $(TARGETS)

version:
    gcc -v

