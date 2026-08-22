TARGET = game
OBJS = src/main.o

CFLAGS = -O2 -G0 -Wall
LIBS = -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Nikitu

include $(PSPDEV)/lib/build.mak
