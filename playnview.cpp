/*
 *  playnview.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 16.09.10.
 *  code under GPLv3
 *
 */

// merged the player and the viewer
// code mostly based on engine.cpp and showgame.cpp

#include <string>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include "utils.h"
#include "game.h"
#include "process.h"
#include "viewer.h"

using namespace std;

static int argc;
static char** args;
static std::vector<Process*> clients;

void KillClients() {
	for (size_t i = 0; i < clients.size(); ++i) {
		delete clients[i];
		clients[i] = NULL;
	}
}

int PlayGameThread(void* param) {	
	// Initialize the game. Load the map.
	std::string mapFilename = args[1];
	long maxTurnTime = atol(args[2]);
	int maxNumTurns = atoi(args[3]);
	std::string logFilename = args[4];	
	std::ofstream logStream(logFilename.c_str());
	
	Game game(maxNumTurns, NULL, logStream ? &logStream : NULL);	
	game.WriteLogMessage("initializing");
	if (game.LoadMapFromFile(mapFilename) == 0) {
		cerr << "ERROR: failed to start game. map: " << mapFilename << endl;
		return 1;
	}
	
	Viewer_pushInitialGame(new Game(game));
		
	std::vector<bool> isAlive(clients.size());
	for (size_t i = 0; i < clients.size(); ++i) {
		isAlive[i] = (bool)clients[i];
	}
	
	int numTurns = 0;
	// Enter the main game loop.
	while (game.Winner() < 0) {
		// Send the game state to the clients.
		//cout << "The game state:" << endl;
		//cout << game.toString() << endl;
		for (size_t i = 0; i < clients.size(); ++i) {
			if (!*clients[i] || !game.state.IsAlive(i + 1)) continue;
			
			std::string message = game.PovRepresentation(i + 1) + "go\n";
			try {
				*clients[i] << message << flush;
				game.WriteLogMessage("engine > player" + to_string(i + 1) + ": " +
									 message);
			} catch (...) {
				cerr << "ERROR while writing to client " << (i+1) << endl;
				clients[i]->destroy();
			}
		}
		
		// Get orders from the clients.
		std::vector<bool> clientDone(clients.size(), false);
		long startTime = currentTimeMillis();
		for (size_t i = 0; i < clients.size(); ++i) {
			if (!isAlive[i] || !game.state.IsAlive(i + 1) || clientDone[i]) {
				clientDone[i] = true;
				continue;
			}
			try {
				while(true) {
					long dt = currentTimeMillis() - startTime;
					if(dt > maxTurnTime) break;
					std::string line;
					if(!clients[i]->readLine(line, maxTurnTime - dt)) break;
					
					line = ToLower(TrimSpaces(line));
					//cerr << "P" << (i+1) << ": " << line << endl;
					game.WriteLogMessage("player" + to_string(i + 1) + " > engine: " + line);
					if (line == "go") {						
						clientDone[i] = true;
						break;
					}
					else
						game.ExecuteOrder(i + 1, line);
				}
			} catch (...) {
				cerr << "WARNING: player " << (i+1) << " crashed." << endl;
				clients[i]->destroy();
				game.state.DropPlayer(i + 1);
				isAlive[i] = false;
			}
		}
		for (size_t i = 0; i < clients.size(); ++i) {
			if (!isAlive[i] || !game.state.IsAlive(i + 1)) continue;
			if (clientDone[i]) continue;
			
			cerr << "WARNING: player " << (i+1) << " timed out." << endl;
			clients[i]->destroy();
			game.state.DropPlayer(i + 1);
			isAlive[i] = false;
		}
		++numTurns;
		cerr << "Turn " << numTurns << endl;
		game.DoTimeStep();		
		Viewer_pushGameState(new GameState(game.state));
	}
	
	if (game.Winner() > 0) {
		cerr << "Player " << game.Winner() << " Wins!" << endl;
	} else {
		cerr << "Draw!" << endl;
	}
	
	KillClients();

	return 0;
}

void signalhandler(int) {
	for (size_t i = 0; i < clients.size(); ++i)
		clients[i]->destroy();
}

int main(int _argc, char** _args) {
	argc = _argc;
	args = _args;
	
	// Check the command-line arguments.
	if (argc < 6) {
		cerr << "ERROR: wrong number of command-line arguments." << endl;
		cerr << "USAGE: engine map_file_name max_turn_time "
		<< "max_num_turns log_filename player_one "
		<< "player_two [more_players]" << endl;
		return 1;
	}

	signal(SIGHUP, &signalhandler);
	signal(SIGINT, &signalhandler);
	signal(SIGQUIT, &signalhandler);
	
	// Start the client programs (players).
	for (int i = 5; i < argc; ++i) {
		std::string command = args[i];
		Process* client = new Process(command);
		clients.push_back(client);
		
		client->run();
		if (!*client) {
			KillClients();
			cerr << "ERROR: failed to start client: " << command << endl;
			return 1;
		}
	}
	
	if(!Viewer_initWindow("PlanetWars")) return 1;
	
	SDL_Thread* player = SDL_CreateThread(&PlayGameThread, args);

	Viewer_mainLoop();

	SDL_WaitThread(player, NULL);
	SDL_Quit();
	return 0;
}



