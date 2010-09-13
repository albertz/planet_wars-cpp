CC=g++

all: playgame showgame

clean:
	rm -rf *.o 

%.o: %.cpp
	$(CC) -Wall -O2 -g $< -c -o $@

playgame: PlanetWars.o engine.o game.o utils.o
	$(CC) $^ -g -o $@

showgame: PlanetWars.o utils.o viewgame.o SDL_draw.o
	$(CC) $^ -g -o $@
