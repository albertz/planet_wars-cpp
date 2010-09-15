#include <iostream>
#include "game.h"

static void DoTurn(const Game& pw) {
	typedef std::vector<Planet> Planets;
	Planets myPlanets = pw.MyPlanets();
	Planets enemyPlanets = pw.EnemyPlanets();
	for (Planets::iterator source = myPlanets.begin(); source != myPlanets.end(); ++source) {
	    if (source->numShips < 10 * source->growthRate) {
			continue;
	    }
	    int dest = -1;
	    int bestDistance = 999999;
	    for (Planets::iterator p = enemyPlanets.begin(); p != enemyPlanets.end(); ++p) {
			int dist = pw.desc.Distance(source->planetId, p->planetId);
			if (dist < bestDistance) {
				bestDistance = dist;
				dest = p->planetId;
			}
	    }
	    if (dest >= 0) {
			pw.IssueOrder(source->planetId, dest, source->numShips);
	    }
	}
}


int main(int argc, char *argv[]) {
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
