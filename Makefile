TARGET = game
OBJS = src/main.o

CFLAGS = -O2 -G0 -Wall
LIBS = -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Nikitu

PSPSDK := $(shell psp-config --pspsdk-path)

include $(PSPSDK)/lib/build.mak
