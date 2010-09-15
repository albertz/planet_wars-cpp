#include <iostream>
#include <cstdlib>
#include "utils.h"
#include "game.h"

static double NextRandD() {
	return double(rand()) / RAND_MAX;
}

static size_t NextRand(size_t upperLimit) { // limit excluded
	return CLAMP( size_t(NextRandD() * upperLimit), size_t(0), upperLimit - 1 );
}

static void DoTurn(const Game& pw) {
	typedef std::vector<Planet> Planets;
	// (1) If we current have a fleet in flight, then do nothing until it
	// arrives.
	if (pw.MyFleets().size() >= 1) {
	    return;
	}
	// (2) Pick one of my planets at random.
	int source = -1;
	Planets p = pw.MyPlanets();
	if (p.size() > 0) {
	    source = NextRand(p.size());
	}
	// (3) Pick a target planet at random.
	int dest = -1;
	p = pw.Planets();
	if (p.size() > 0) {
	    dest = NextRand(p.size());
	}
	// (4) Send half the ships from source to dest.
	if (source >= 0 && dest >= 0 && source != dest) {
	    int numShips = pw.state.planets[source].numShips / 2;
	    pw.IssueOrder(source, dest, numShips);
	}
}

int main(int argc, char *argv[]) {
	srand(currentTimeMillis());
	std::string current_line;
	std::string map_data;
	while (true) {
		int c = std::cin.get();
		current_line += (char)c;
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
