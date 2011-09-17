FONT_SRC := unifontfull-5.1.20080820.hex
CC := g++-4.5
HOSTCC = $(CC)
OBJECTS = controllerdata.o memorymanip.o misc.o movie.o moviefile.o render.o rom.o zip.o fonts/font.o keymapper.o window.o window-sdl.o settings.o framerate.o mainloop.o rrdata.o png.o lsnesrc.o memorywatch.o command.o avsnoop.o moviedata.o controller.o framebuffer.o

#AVI dumper
OBJECTS += avidump/avidump-control.o avidump/avidump.o avidump/sox.o

PROGRAMS = lsnes.exe

CFLAGS = -I. $(shell sdl-config --cflags) $(USER_CFLAGS)
HOSTCCFLAGS = $(USER_HOSTCCFLAGS)
LDFLAGS = $(shell sdl-config --libs) $(USER_LDFLAGS)

#Lua.
ifdef NO_LUA
OBJECTS += lua-dummy.o
else
OBJECTS += lua.o $(patsubst %.cpp,%.o,$(wildcard lua/*.cpp))
CFLAGS += $(shell pkg-config lua5.1 --cflags)
LDFLAGS += $(shell pkg-config lua5.1 --libs)
endif


ifdef NO_THREADS
CFLAGS += -DNO_THREADS
endif

ifdef BSNES_IS_COMPAT
CFLAGS += -DBSNES_IS_COMPAT
endif


all: $(PROGRAMS)

.PRECIOUS: %.exe %.o

%.exe: %.o $(OBJECTS)
	$(CC) -o $@ $^ $(BSNES_PATH)/out/libsnes.a -ldl -lboost_iostreams -lboost_filesystem -lboost_system -lz $(LDFLAGS)

%.o: %.cpp
	$(CC) -I. -g -std=c++0x -I$(BSNES_PATH) -c -o $@ $< $(CFLAGS)

fonts/font.o: fonts/$(FONT_SRC) fonts/parsehexfont.exe
	fonts/parsehexfont.exe <fonts/$(FONT_SRC) >fonts/font.cpp
	$(HOSTCC) -std=c++0x $(HOSTCCFLAGS) -c -o fonts/font.o fonts/font.cpp
	$(HOSTCC) -std=c++0x $(HOSTCCFLAGS) -o fonts/verifyhexfont.exe fonts/verifyhexfont.cpp fonts/font.o
	fonts/verifyhexfont.exe

fonts/parsehexfont.exe: fonts/parsehexfont.cpp
	$(HOSTCC) -std=c++0x $(HOSTCCFLAGS) -o $@ $^

clean:
	rm -f fonts/font.o fonts/font.cpp fonts/verifyhexfont.exe fonts/parsehexfont.exe $(OBJECTS) romtest1.o $(PROGRAMS) lsnes.o
