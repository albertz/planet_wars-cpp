// Copyright 2010 owners of the AI Challenge project
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at http://www.apache.org/licenses/LICENSE-2.0 . Unless
// required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// Author: Jeff Cameron (jeff@jpcameron.com)
// ported to C++ by Albert Zeyer
//
// Plays a game of Planet Wars between two computer programs.

#include <string>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <limits>
#include <signal.h>
#include "utils.h"
#include "game.h"
#include "process.h"
#include "engine.h"

using namespace std;

static std::vector<Process*> clients;

void KillClients() {
	for (size_t i = 0; i < clients.size(); ++i) {
		delete clients[i];
		clients[i] = NULL;
	}
}

static int argc;
static char** argv;

static std::string mapFilename = "maps/map1.txt";
static long maxTurnTime = 5000;
static long maxFirstTurnTime = 10000;
static int maxNumTurns = 200;
static std::string logFilename;	
static std::ofstream logStream;
static std::ostream* replayStream = &cout;
static bool waitForBot1 = false;
static bool beQuiet = false;
static std::vector<std::string> playerCommands;

void PrintHelpAndExit() {
	cerr
	<< "usage: " << endl
	<< "  " << argv[0] << " <map_file_name> <max_turn_time> "
	<< "<max_num_turns> <log_filename> <player_one> "
	<< "<player_two> [more_players]" << endl
	<< "or" << endl
	<< "  " << argv[0] << " [-m <map>] [-t <turn_time>] "
	<< "[-ft <first_turn_time>] "
	<< "[-n <num_turns>] [-l <logfile>] [-wait] "
	<< (replayStream ? "[-noout] " : "") << "[-quiet] [--] "
	<< "<player_one> <player_two> [more_players]" << endl
	<< "with default values:" << endl
	<< "  map = maps/map1.txt" << endl
	<< "  turn_time = 5 = timeout in seconds" << endl
	<< "  first_turn_time = -1 = no timeout" << endl
	<< "  num_turns = 200" << endl
	<< "  logfile = \"\" = no logfile" << endl
	<< "-wait : wait for player1 to exit (useful for debugging)" << endl;
	if(replayStream) cerr << "-noout : no replay output" << endl;
	cerr
	<< "-quiet : less output" << endl
	<< "-- : needed if you specify more than 5 players" << endl
	<< "or" << endl
	<< "  " << argv[0] << " -h : this help" << endl
	;
	_exit(0);
}

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
		else if(arg == "-noout")
			replayStream = NULL;
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
				maxTurnTime = atol(argv[i])*1000;
			else if(arg == "-ft")
				maxFirstTurnTime = atol(argv[i])*1000;
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
		playerCommands = std::vector<std::string>( unnamedParams.begin() + 4, unnamedParams.end() );
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
		maxTurnTime = (std::numeric_limits<int>::max)();

	if(maxFirstTurnTime < 0)
		maxFirstTurnTime = (std::numeric_limits<int>::max)();
}

void signalhandler(int) {
	for (size_t i = 0; i < clients.size(); ++i)
		if(clients[i]) clients[i]->destroy();
	exit(0);
}

bool PW__init(int _argc, char** _argv, std::ostream* _replayStream) {
	argc = _argc;
	argv = _argv;
	replayStream = _replayStream;
	ParseParams();
	
	signal(SIGINT, &signalhandler);
#ifndef _WIN32
	signal(SIGHUP, &signalhandler);
	signal(SIGQUIT, &signalhandler);
#endif

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
			return false;
		}
	}
	
	return true;
}

bool PW__mainloop(PWMainloopCallbacks callbacks) {
	// Initialize the game. Load the map.
	Game game(maxNumTurns, replayStream, logStream ? &logStream : NULL);	
	game.WriteLogMessage("initializing");
	if(!game.LoadMapFromFile(mapFilename)) {
		cerr << "ERROR: failed to load map: " << mapFilename << endl;
		return false;
	}
	
	if(callbacks.OnInitialGame)
		(*callbacks.OnInitialGame)(game);
	
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
					if(dt > ((numTurns == 0) ? maxFirstTurnTime : maxTurnTime)) break;
					std::string line;
					const size_t timeOut = ((numTurns == 0) ? maxFirstTurnTime : maxTurnTime) - dt;
					if(!clients[i]->readLine(line, timeOut)) break;
					
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
		if(callbacks.OnNextGameState)
			(*callbacks.OnNextGameState)(game);
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
	return true;
}
