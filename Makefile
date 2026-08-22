TARGET = game
OBJS = src/main_v007.o src/graphics.o

CFLAGS = -O2 -G0 -Wall
LIBS = -lpspdebug -lpspgu -lpspdisplay -lpspge -lpspctrl -lpspsdk -lpng -lz -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Nikitu

PSPSDK := $(shell psp-config --pspsdk-path)

include $(PSPSDK)/lib/build.mak
