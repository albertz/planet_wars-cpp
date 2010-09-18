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
static char** argv;

void PrintHelpAndExit() {
	cerr
	<< "usage: " << endl
	<< "  " << argv[0] << " <map_file_name> <max_turn_time> "
	<< "<max_num_turns> <log_filename> <player_one> "
	<< "<player_two> [more_players]" << endl
	<< "or" << endl
	<< "  " << argv[0] << " [-m <map>] [-t <turn_time>] "
	<< "[-n <num_turns>] [-l <logfile>] [-wait] [-quiet] [--] "
	<< "<player_one> <player_two> [more_players]" << endl
	<< "with default values:" << endl
	<< "  map = maps/map1.txt" << endl
	<< "  turn_time = -1 = no timeout" << endl
	<< "  num_turns = -1 = infinity" << endl
	<< "  logfile = \"\" = no logfile" << endl
	<< "-wait : wait for player1 to exit (useful for debugging)" << endl
	<< "-quiet : less output" << endl
	<< "-- : needed if you specify more than 5 players" << endl
	<< "or" << endl
	<< "  " << argv[0] << " -h : this help" << endl
	;
	_exit(0);
}

static std::string mapFilename = "maps/map1.txt";
static long maxTurnTime = -1;
static int maxNumTurns = -1;
static std::string logFilename;	
static std::ofstream logStream;
static std::ostream* replayStream = NULL;
static bool waitForBot1 = false;
static bool beQuiet = false;
static std::vector<std::string> playerCommands;

static bool looksLikeParamOption(const std::string& arg) {
	if(arg.size() == 0) return false;
	if(arg[0] != '-') return false;
	if(arg.size() <= 1) return false;
	return arg[1] >= 'a' && arg[1] <= 'z';
}

void ParseParams() {
	bool haveSeperatorStr = false;
	std::vector<std::string> unnamedParams; unnamedParams.reserve(6);
	for(int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if(arg == "--")
			haveSeperatorStr = true;
		else if(!looksLikeParamOption(arg))
			unnamedParams.push_back(arg);
		else if(arg == "-wait")
			waitForBot1 = true;
		else if(arg == "-quiet")
			beQuiet = true;
		else if(arg == "-h")
			PrintHelpAndExit();
		else {
			++i;
			if(i >= argc) {
				cerr << arg << " expecting option or invalid" << endl;
				PrintHelpAndExit();
			}
			if(arg == "-m")
				mapFilename = argv[i];
			else if(arg == "-t")
				maxTurnTime = atol(argv[i]);
			else if(arg == "-n")
				maxNumTurns = atoi(argv[i]);
			else if(arg == "-l")
				logFilename = argv[i];
			else {
				cerr << "don't understand option: " << arg << endl;
				PrintHelpAndExit();
			}
		}
	}
	
	if(unnamedParams.size() >= 6 && !haveSeperatorStr) { // old style parameters
		mapFilename = unnamedParams[0];
		maxTurnTime = atol(unnamedParams[1].c_str());
		maxNumTurns = atoi(unnamedParams[2].c_str());
		logFilename = unnamedParams[3];
		playerCommands = std::vector<std::string>( &unnamedParams[4], &unnamedParams[unnamedParams.size()] );		
	}
	else { // new style parameters
		playerCommands = unnamedParams;
	}	
	
	if(playerCommands.size() < 2) {
		cerr << "you need at least 2 players" << endl;
		PrintHelpAndExit();
	}
	
	if(logFilename != "")
		logStream.open(logFilename.c_str());
	
	if(maxTurnTime < 0)
		maxTurnTime = std::numeric_limits<long>::max();
}

static std::vector<Process*> clients;

void KillClients() {
	for (size_t i = 0; i < clients.size(); ++i) {
		delete clients[i];
		clients[i] = NULL;
	}
}

int PlayGameThread(void*) {	
	// Initialize the game. Load the map.
	Game game(maxNumTurns, replayStream, logStream ? &logStream : NULL);	
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
		if(!beQuiet) cerr << "Turn " << numTurns << endl;
		game.DoTimeStep();		
		Viewer_pushGameState(new GameState(game.state));
	}
	
	if(beQuiet) cerr << "after " << numTurns << " turns: "; // so we know at least the numturns
	if (game.Winner() > 0) {
		cerr << "Player " << game.Winner() << " Wins!" << endl;
	} else {
		cerr << "Draw!" << endl;
	}

	if(waitForBot1)
		clients[0]->waitForExit();
	
	KillClients();

	return 0;
}

void signalhandler(int) {
	for (size_t i = 0; i < clients.size(); ++i)
		clients[i]->destroy();
	_exit(0);
}

int main(int _argc, char** _argv) {
	argc = _argc;
	argv = _argv;
	ParseParams();
	
	signal(SIGHUP, &signalhandler);
	signal(SIGINT, &signalhandler);
	signal(SIGQUIT, &signalhandler);
	
	// Start the client programs (players).
	clients.reserve(playerCommands.size());
	for (size_t i = 0; i < playerCommands.size(); ++i) {
		std::string command = playerCommands[i];
		Process* client = new Process(command);
		clients.push_back(client);
		
		client->run();
		if (!*client) {
			cerr << "ERROR: failed to start client: " << command << endl;
			KillClients();
			return 1;
		}
	}
	
	if(!Viewer_initWindow("PlanetWars")) return 1;
	
	SDL_Thread* player = SDL_CreateThread(&PlayGameThread, NULL);

	Viewer_mainLoop();

	SDL_WaitThread(player, NULL);
	SDL_Quit();
	return 0;
}



