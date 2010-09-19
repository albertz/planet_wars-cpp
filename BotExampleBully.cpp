#include <iostream>
#include "game.h"

static void DoTurn(Game& pw) {
	// (1) If we current have a fleet in flight, just do nothing.
	if (pw.state.fleets.size() >= 1) {
	    return;
	}
	// (2) Find my strongest planet.
	int source = -1;
	double sourceScore = -1;
	for (size_t p = 0; p < pw.NumPlanets(); ++p) {
		if(pw.state.planets[p].owner != 1) continue;
	    double score = (double)pw.state.planets[p].numShips;
	    if (score > sourceScore) {
			sourceScore = score;
			source = p;
	    }
	}
	// (3) Find the weakest enemy or neutral planet.
	int dest = -1;
	double destScore = -1;
	for (size_t p = 0; p < pw.NumPlanets(); ++p) {
		if(pw.state.planets[p].owner == 1) continue;
	    double score = 1.0 / (1 + pw.state.planets[p].numShips);
	    if (score > destScore) {
			destScore = score;
			dest = p;
	    }
	}
	// (4) Send half the ships from my strongest planet to the weakest
	// planet that I do not own.
	if (source >= 0 && dest >= 0) {
	    int numShips = pw.state.planets[source].numShips / 2;
	    pw.IssueOrder(source, dest, numShips);
	}
}

int main(int argc, char *argv[]) {
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
				map_data = "";
				DoTurn(game);
				game.FinishTurn();
			} else {
				map_data += current_line;
			}
			current_line = "";
		}
	}
	return 0;
}
