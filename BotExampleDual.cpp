#include <iostream>
#include "game.h"

static void DoTurn(const Game& pw) {
	unsigned int numFleets = 1;
	bool attackMode = false;
	if (pw.state.NumShips(1) > pw.state.NumShips(2)) {
	    if (pw.Production(1) > pw.Production(2)) {
			numFleets = 1;
			attackMode = true;
	    } else {
			numFleets = 3;
	    }
	} else {
	    if (pw.Production(1) > pw.Production(2)) {
			numFleets = 1;
	    } else {
			numFleets = 5;
	    }	    
	}
	// (1) If we current have more tha numFleets fleets in flight, just do
	// nothing until at least one of the fleets arrives.
	if (pw.MyFleets().size() >= numFleets) {
	    return;
	}
	// (2) Find my strongest planet.
	int source = -1;
	double sourceScore = -1;
	for (size_t p = 0; p < pw.NumPlanets(); ++p) {
		if(pw.state.planets[p].owner != 1) continue;
	    double score = (double)pw.state.planets[p].numShips / (1 + pw.desc.planets[p].growthRate);
	    if (score > sourceScore) {
			sourceScore = score;
			source = p;
	    }
	}
	// (3) Find the weakest enemy or neutral planet.
	int dest = -1;
	double destScore = -1;
	std::vector<Planet> candidates = pw.NotMyPlanets();
	if (attackMode) {
	    candidates = pw.EnemyPlanets();
	}
	for (std::vector<Planet>::iterator p = candidates.begin(); p != candidates.end(); ++p) {
	    double score = (double)(1 + p->growthRate) / p->numShips;
	    if (score > destScore) {
			destScore = score;
			dest = p->planetId;
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
