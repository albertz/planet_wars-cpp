#include <iostream>
#include "game.h"

#ifdef DEBUGGAME
#include <SDL.h>
#include "viewer.h"
#endif

// The DoTurn function is where your code goes. The PlanetWars object contains
// the state of the game, including information about all planets and fleets
// that currently exist. Inside this function, you issue orders using the
// pw.IssueOrder() function. For example, to send 10 ships from planet 3 to
// planet 8, you would say pw.IssueOrder(3, 8, 10).
//
// There is already a basic strategy in place here. You can use it as a
// starting point, or you can throw it out entirely and replace it with your
// own. Check out the tutorials and articles on the contest website at
// http://www.ai-contest.com/resources.
void DoTurn(const Game& pw) {
	// (1) If we currently have a fleet in flight, just do nothing.
	if (pw.MyFleets().size() >= 1) {
		return;
	}
	// (2) Find my strongest planet.
	int source = -1;
	double source_score = -999999.0;
	int source_num_ships = 0;
	std::vector<Planet> my_planets = pw.MyPlanets();
	for (size_t i = 0; i < my_planets.size(); ++i) {
		const Planet& p = my_planets[i];
		double score = (double)p.numShips;
		if (score > source_score) {
			source_score = score;
			source = p.planetId;
			source_num_ships = p.numShips;
		}
	}
	// (3) Find the weakest enemy or neutral planet.
	int dest = -1;
	double dest_score = -999999.0;
	std::vector<Planet> not_my_planets = pw.NotMyPlanets();
	for (size_t i = 0; i < not_my_planets.size(); ++i) {
		const Planet& p = not_my_planets[i];
		double score = 1.0 / (1 + p.numShips);
		if (score > dest_score) {
			dest_score = score;
			dest = p.planetId;
		}
	}
	// (4) Send half the ships from my strongest planet to the weakest
	// planet that I do not own.
	if (source >= 0 && dest >= 0) {
		int num_ships = source_num_ships / 2;
		pw.IssueOrder(source, dest, num_ships);
	}
}

int PlayGame(void* p = NULL) {
	bool isFirstTurn = true;
	std::string current_line;
	std::string map_data;
	while (true) {
		int c = std::cin.get();
		if(c < 0) break;
		current_line += (char)(unsigned char)c;
		if (c == '\n') {
			if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
				Game game;
				game.ParseGameState(map_data);
#ifdef DEBUGGAME
				if(isFirstTurn)
					Viewer_pushInitialGame(new Game(game));
				else
					Viewer_pushGameState(new GameState(game.state));
#endif
				map_data = "";
				DoTurn(game);
				game.FinishTurn();
				isFirstTurn = false;
			} else {
				map_data += current_line;
			}
			current_line = "";
		}
	}
	return 0;
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
#ifdef DEBUGGAME
	if(!Viewer_initWindow("My Bot")) return 1;
	
	/*SDL_Thread* player =*/ SDL_CreateThread(&PlayGame, NULL);
	
	Viewer_mainLoop();
	
	// the hard and ugly way. but the read is blocking in the player thread,
	// so this is just the easiest way. and also doesn't really matter anyway
	_exit(0);
#else
	PlayGame();
#endif
	return 0;
}
