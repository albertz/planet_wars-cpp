CC=gcc
CPP=g++

CFLAGS := -g -O2 -Wall
CFLAGS := $(CFLAGS) $(shell \
	[ "$$(uname -s)" = "Darwin" ] && echo "-arch" && uname -m; \
)

LFLAGS := $(CFLAGS)

SDL_CFLAGS := $(shell \
	[ "$$(uname -s)" = "Darwin" ] && echo "-I /Library/Frameworks/SDL.framework/Headers" && exit 0; \
	sdl-config --cflags; \
)

SDL_LFLAGS := $(shell \
	[ "$$(uname -s)" = "Darwin" ] && echo "-framework SDL -framework Cocoa SDLmain.m" && exit 0; \
	sdl-config --libs; \
)

SDL_LFLAGS := $(SDL_CFLAGS) $(SDL_LFLAGS)

all: playgame showgame

clean:
	rm -rf *.o playgame showgame

PlanetWars.o: PlanetWars.cpp PlanetWars.h
	$(CPP) $(CFLAGS) $< -c -o $@

engine.o: engine.cpp
	$(CPP) $(CFLAGS) $< -c -o $@

game.o: game.cpp game.h
	$(CPP) $(CFLAGS) $< -c -o $@

utils.o: utils.cpp utils.h
	$(CPP) $(CFLAGS) $< -c -o $@

showgame.o: showgame.cpp
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

viewer.o: viewer.cpp
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

font.o: font.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

gfx.o: gfx.cpp gfx.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@
	
SDL_picofont.o: SDL_picofont.cpp SDL_picofont.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@
	
#%.o: %.cpp

playgame: PlanetWars.o engine.o game.o utils.o
	$(CPP) $(LFLAGS) $^ -o $@

showgame: PlanetWars.o utils.o game.o showgame.o viewer.o font.o SDL_picofont.o gfx.o
	$(CPP) $(LFLAGS) $(SDL_LFLAGS) $^ -o $@
