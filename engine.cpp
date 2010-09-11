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
#include <stdlib.h>

namespace Engine {
    void KillClients(List<Process> clients) {
		for (Process p : clients) {
			if (p != null) {
				p.destroy();
			}
		}
    }
	
    bool AllTrue(boolean[] v) {
		for (int i = 0; i < v.length; ++i) {
			if (!v[i]) {
				return false;
			}
		}
		return true;
    }
}

int main(int argc, char** args) {
	using namespace std;
	
	// Check the command-line arguments.
	if (argc < 5) {
		cerr << "ERROR: wrong number of command-line arguments." << endl;
		cerr << "USAGE: engine map_file_name max_turn_time "
			 << "max_num_turns log_filename player_one "
			 << "player_two [more_players]" << endl;
		return 1;
	}

	// Initialize the game. Load the map.
	std::string mapFilename = args[0];
	long maxTurnTime = atol(args[1]);
	int maxNumTurns = atoi(args[2]);
	std::string logFilename = args[3];

	Game game(mapFilename, maxNumTurns, 0, logFilename);
	if (game.Init() == 0) {
		cerr << "ERROR: failed to start game. map: " << mapFilename << endl;
		return 1;
	}

	// Start the client programs (players).
	std::vector<Process> clients;
	for (int i = 4; i < argc; ++i) {
		std::string command = args[i];
		Process client = null;
		try {
			client = Runtime.getRuntime().exec(command);
		} catch (Exception e) {
			client = null;
		}
		if (client == null) {
			KillClients(clients);
			cerr << "ERROR: failed to start client: " << command << endl;
			return 1;
		}
		clients.push_back(client);
	}
	
	std::vector<bool> isAlive(clients.size());
	for (int i = 0; i < clients.size(); ++i) {
		isAlive[i] = clients[i] != null;
	}
	
	int numTurns = 0;
	// Enter the main game loop.
	while (game.Winner() < 0) {
		// Send the game state to the clients.
		//cout << "The game state:" << endl;
		//cout << game.toString() << endl;
		for (int i = 0; i < clients.size(); ++i) {
			if (clients[i] == null || !game.IsAlive(i + 1)) {
				continue;
			}
			std::string message = game.PovRepresentation(i + 1) + "go\n";
			try {
				OutputStream out = clients.get(i).getOutputStream();
				OutputStreamWriter writer = new OutputStreamWriter(out);
				writer.write(message, 0, message.length());
				writer.flush();
				game.WriteLogMessage("engine > player" + (i + 1) + ": " +
									 message);
			} catch (Exception e) {
				clients.set(i, null);
			}
		}
		// Get orders from the clients.
		StringBuilder[] buffers = new StringBuilder[clients.size()];
		boolean[] clientDone = new boolean[clients.size()];
		for (int i = 0; i < clients.size(); ++i) {
			buffers[i] = new StringBuilder();
			clientDone[i] = false;
		}
		long startTime = System.currentTimeMillis();
		while (!AllTrue(clientDone) &&
			   System.currentTimeMillis() - startTime < maxTurnTime) {
			for (int i = 0 ; i < clients.size(); ++i) {
				if (!isAlive[i] || !game.IsAlive(i + 1) || clientDone[i]) {
					clientDone[i] = true;
					continue;
				}
				try {
					InputStream inputStream =
					clients.get(i).getInputStream();
					while (inputStream.available() > 0) {
						char c = (char)inputStream.read();
						if (c == '\n') {
							std::string line = buffers[i].toString();
							//System.err.println("P" + (i+1) + ": " + line);
							line = line.toLowerCase().trim();
							game.WriteLogMessage("player" + (i + 1) + " > engine: " + line);
							if (line.equals("go")) {
								clientDone[i] = true;
							} else {
								game.IssueOrder(i + 1, line);
							}
							buffers[i] = new StringBuilder();
						} else {
							buffers[i].append(c);
						}
					}
				} catch (Exception e) {
					System.err.println("WARNING: player " + (i+1) +
									   " crashed.");
					clients.get(i).destroy();
					game.DropPlayer(i + 1);
					isAlive[i] = false;
				}
			}
		}
		for (int i = 0 ; i < clients.size(); ++i) {
			if (!isAlive[i] || !game.IsAlive(i + 1)) {
				continue;
			}
			if (clientDone[i]) {
				continue;
			}
			System.err.println("WARNING: player " + (i+1) +
							   " timed out.");
			clients.get(i).destroy();
			game.DropPlayer(i + 1);
			isAlive[i] = false;
		}
		++numTurns;
		System.err.println("Turn " + numTurns);
		game.DoTimeStep();
	}
	KillClients(clients);
	if (game.Winner() > 0) {
		System.err.println("Player " + game.Winner() + " Wins!");
	} else {
		System.err.println("Draw!");
	}
	System.out.println(game.GamePlaybackString());
}

