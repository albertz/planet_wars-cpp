CC=gcc
CPP=g++

TARGETS=playgame showgame playnview \
	BotCppStarterpack \
	BotExampleDual \
	BotExampleRage \
	BotExampleBully \
	BotExampleProspector \
	BotExampleRandom

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

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS)

engine.o: engine.cpp utils.h process.h
	$(CPP) $(CFLAGS) $< -c -o $@

game.o: game.cpp game.h utils.h
	$(CPP) $(CFLAGS) $< -c -o $@

utils.o: utils.cpp utils.h
	$(CPP) $(CFLAGS) $< -c -o $@

process.o: process.cpp process.h utils.h
	$(CPP) $(CFLAGS) $< -c -o $@

showgame.o: showgame.cpp viewer.h utils.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

playnview.o: playnview.cpp viewer.h utils.h process.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

viewer.o: viewer.cpp viewer.h utils.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

font.o: font.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@

gfx.o: gfx.cpp gfx.h utils.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@
	
SDL_picofont.o: SDL_picofont.cpp SDL_picofont.h
	$(CPP) $(CFLAGS) $(SDL_CFLAGS) $< -c -o $@
	
#%.o: %.cpp

playgame: engine.o game.o utils.o process.o
	$(CPP) $(LFLAGS) $^ -o $@

showgame: utils.o game.o showgame.o viewer.o font.o SDL_picofont.o gfx.o
	$(CPP) $(LFLAGS) $(SDL_LFLAGS) $^ -o $@

playnview: utils.o game.o playnview.o viewer.o font.o SDL_picofont.o gfx.o process.o
	$(CPP) $(LFLAGS) $(SDL_LFLAGS) $^ -o $@

Bot%: Bot%.cpp game.o utils.o
	$(CPP) $(LFLAGS) $^ -o $@
