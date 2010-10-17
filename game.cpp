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
// Author:	Jeff Cameron (jeff@jpcameron.com)
// ported to C++ by Albert Zeyer
//
// Stores the game state.

#include <string>
#include <set>
#include <math.h>
#include <sstream>
#include <map>
#include <iterator>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include "game.h"
#include "utils.h"

// Writes a string which represents the current game state. This string
// conforms to the Point-in-Time format from the project Wiki.
//
// Optionally, you may specify the pov (Point of View) parameter. The pov
// parameter is a player number. If specified, the player numbers 1 and pov
// will be swapped in the game state output. This is used when sending the
// game state to individual players, so that they can always assume that
// they are player number 1.
std::string Game::PovRepresentation(int pov) {
	std::ostringstream s;
	for (size_t i = 0; i < desc.planets.size(); ++i) {
		s
		<< "P " << desc.planets[i].x << " " << desc.planets[i].y << " "
		<< PovSwitch(pov, state.planets[i].owner) << " "
		<< state.planets[i].numShips << " "
		<< desc.planets[i].growthRate << std::endl;
	}
	for (Fleets::iterator f = state.fleets.begin(); f != state.fleets.end(); ++f) {
		s
		<< "F " << PovSwitch(pov, f->owner) << " "
		<< f->numShips << " " << f->sourcePlanet << " "
		<< f->destinationPlanet << " " << f->totalTripLength << " "
		<< f->turnsRemaining << std::endl;
	}
	return s.str();
}

// Carries out the point-of-view switch operation, so that each player can
// always assume that he is player number 1. There are three cases.
// 1. If pov < 0 then no pov switching is being used. Return player_id.
// 2. If player_id == pov then return 1 so that each player thinks he is
//    player number 1.
// 3. If player_id == 1 then return pov so that the real player 1 looks
//    like he is player number "pov".
// 4. Otherwise return player_id, since players other than 1 and pov are
//    unaffected by the pov switch.
int Game::PovSwitch(int pov, int playerID) {
	if (pov < 0) return playerID;
	if (playerID == pov) return 1;
	if (playerID == 1) return pov;
	return playerID;
}

//Resolves the battle at planet p, if there is one.
//* Removes all fleets involved in the battle
//* Sets the number of ships and owner of the planet according the outcome
void PlanetState::FightBattle(int myPlanetIndex, const std::vector<Fleet>& fleets, int dt) {
	PlanetState& p = *this;
	std::vector<int> participants( std::max(3, p.owner + 1) ); // index = owner
	participants[p.owner] = p.numShips;
	
	for (std::vector<Fleet>::const_iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->turnsRemaining == dt && f->destinationPlanet == myPlanetIndex) {
			participants.resize( std::max(participants.size(), (size_t)f->owner + 1) );
			participants[f->owner] += f->numShips;
		}
	}
	
	Fleet winner(0, 0);
	Fleet second(0, 0);
	for (size_t i = 0; i < participants.size(); ++i) {
		if (participants[i] > second.numShips) {
			if(participants[i] > winner.numShips) {
				second = winner;
				winner = Fleet(i, participants[i]);
			} else {
				second = Fleet(i, participants[i]);
			}
		}
	}
	
	if (winner.numShips > second.numShips) {
		p.numShips = winner.numShips - second.numShips;
		p.owner = winner.owner;
	} else {
		p.numShips = 0;
	}
}

// Executes one time step.
//   * Planet bonuses are added to non-neutral planets.
//   * Fleets are advanced towards their destinations.
//   * Fleets that arrive at their destination are dealt with.
void GameState::DoTimeStep(const GameDesc& desc) {
	FleetsTimeStep(fleets);
	
	for (size_t p = 0; p < planets.size(); ++p)
		planets[p].DoTimeStep(p, desc.planets[p].growthRate, fleets);
	
	RemoveFinalFleets(fleets);
}

void Game::DoTimeStep() {
	state.DoTimeStep(desc);
	
	if(gamePlayback) {
		bool needcomma = false;
		for (GameState::Planets::iterator p = state.planets.begin(); p != state.planets.end(); ++p) {
			if(needcomma) *gamePlayback << ",";
			*gamePlayback
			<< p->owner << "."
			<< p->numShips;
			needcomma = true;
		}
		for (Fleets::iterator f = state.fleets.begin(); f != state.fleets.end(); ++f) {
			*gamePlayback
			<< ","
			<< f->owner << "."
			<< f->numShips << "."
			<< f->sourcePlanet << "."
			<< f->destinationPlanet << "."
			<< f->totalTripLength << "."
			<< f->turnsRemaining;
		}
		*gamePlayback << ":" << std::flush;
	}
	
	// Check to see if the maximum number of turns has been reached.
	++numTurns;	
}


Fleet* GameState::MatchingExistingFleet(const Fleet& f) {
	for (Fleets::iterator fi = fleets.begin(); fi != fleets.end(); ++fi) {
		if (fi->owner == f.owner &&
			fi->sourcePlanet == f.sourcePlanet &&
			fi->destinationPlanet == f.destinationPlanet &&
			fi->turnsRemaining == f.turnsRemaining)
			return &*fi;
	}
	return NULL;
}


// Execute an order. This function takes num_ships off the source_planet,
// puts them into a newly-created fleet, calculates the distance to the
// destination_planet, and sets the fleet's total trip time to that
// distance. Checks that the given player_id is allowed to give the given
// order. If the order was carried out without any issue, and everything is
// peachy, then true is returned. Otherwise, false is returned.
bool GameState::ExecuteOrder(const GameDesc& desc,
							 int playerID,
							 int sourcePlanet,
							 int destinationPlanet,
							 int numShips) {
	if (numShips <= 0 ||
		sourcePlanet < 0 || planets.size() <= (Planets::size_type)sourcePlanet ||
		destinationPlanet < 0 || planets.size() <= (Planets::size_type)destinationPlanet ||
		sourcePlanet == destinationPlanet) {
		return false;
	}
	PlanetState& source = planets[sourcePlanet];
	if (source.owner != playerID ||
		numShips > source.numShips) {
		return false;
	}

	source.numShips -= numShips;
	int distance = desc.Distance(sourcePlanet, destinationPlanet);
	Fleet f(source.owner,
			numShips,
			sourcePlanet,
			destinationPlanet,
			distance,
			distance);
	if(Fleet* existingFleet = MatchingExistingFleet(f))
		existingFleet->numShips += numShips;
	else 
		fleets.push_back(f);
	return true;
}

// Parses a string of the form "source_planet destination_planet num_ships"
// and calls state.ExecuteOrder. If that fails, the player is dropped.
bool Game::ExecuteOrder(int playerID, const std::string& order) {
	std::vector<std::string> tokens = Tokenize(order, " ");
	if (tokens.size() != 3) return -1;
	
	int sourcePlanet = atoi(tokens[0].c_str());
	int destinationPlanet = atoi(tokens[1].c_str());
	int numShips = atoi(tokens[2].c_str());

	if(!state.ExecuteOrder(desc, playerID, sourcePlanet, destinationPlanet, numShips)) {
		WriteLogMessage("Dropping player " + to_string(playerID) +
						". source.Owner() = " + to_string(state.planets[sourcePlanet].owner) + ", playerID = " +
						to_string(playerID) + ", numShips = " + to_string(numShips) +
						", source.NumShips() = " + to_string(state.planets[sourcePlanet].numShips));
		std::cerr << "Dropping player " << playerID << " because of invalid order: " << order << std::endl;
		state.DropPlayer(playerID);
		return false;
	}
	return true;
}

// Kicks a player out of the game. This is used in cases where a player
// tries to give an illegal order or runs over the time limit.
void GameState::DropPlayer(int playerID) {
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			p->owner = 0;
	}
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner == playerID)
			f->Kill();
	}
}

// Returns true if the named player owns at least one planet or fleet.
// Otherwise, the player is deemed to be dead and false is returned.
bool GameState::IsAlive(int playerID) const {
	for (Planets::const_iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			return true;
	}
	for (Fleets::const_iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner == playerID)
			return true;
	}
	return false;
}

// If the game is not yet over (ie: at least two players have planets or
// fleets remaining), returns -1. If the game is over (ie: only one player
// is left) then that player's number is returned. If there are no
// remaining players, then the game is a draw and 0 is returned.
int GameState::Winner(bool maxTurnsReached) const {
	std::set<int> remainingPlayers;
	for (Planets::const_iterator p = planets.begin(); p != planets.end(); ++p) {
		remainingPlayers.insert(p->owner);
	}
	for (Fleets::const_iterator f = fleets.begin(); f != fleets.end(); ++f) {
		remainingPlayers.insert(f->owner);
	}
	remainingPlayers.erase(0);
	if (maxTurnsReached) {
		int leadingPlayer = -1;
		int mostShips = -1;
		for (std::set<int>::iterator p = remainingPlayers.begin(); p != remainingPlayers.end(); ++p) {
			int playerID = *p;
			int numShips = NumShips(playerID);
			if (numShips == mostShips) {
				leadingPlayer = 0;
			} else if (numShips > mostShips) {
				leadingPlayer = playerID;
				mostShips = numShips;
			}
		}
		return leadingPlayer;
	}
	switch (remainingPlayers.size()) {
		case 0:
			return 0;
		case 1:
			return *remainingPlayers.begin();
	}
	return -1;
}

// Returns the number of ships that the current player has, either located
// on planets or in flight.
int GameState::NumShips(int playerID) const {
	int numShips = 0;
	for (Planets::const_iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			numShips += p->numShips;
	}
	for (Fleets::const_iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner == playerID)
			numShips += f->numShips;
	}
	return numShips;
}

int GameState::NumShipsOnPlanets(int playerID) const {
	int numShips = 0;
	for (Planets::const_iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			numShips += p->numShips;
	}
	return numShips;
}


int GameState::HighestPlayerID() const {
	int highestP = 0;
	for (Planets::const_iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner > highestP)
			highestP = p->owner;
	}
	for (Fleets::const_iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner > highestP)
			highestP = f->owner;
	}
	return highestP;	
}

// Parses a game state from a string. On success, returns true. On failure, returns false.
bool Game::ParseGameState(const std::string& s) {
	clear();
	std::vector<std::string> lines = Tokenize(s, "\n");
	for (size_t i = 0; i < lines.size(); ++i) {
		std::string& line = lines[i];
		size_t commentBegin = line.find('#');
		if (commentBegin != std::string::npos)
			line = line.substr(0, commentBegin);
		if (TrimSpaces(line).size() == 0) continue;
		
		std::vector<std::string> tokens = Tokenize(line, " ");
		if (tokens.size() == 0) continue;
		
		if (tokens[0] == "P") {
			if (tokens.size() != 6) return 0;
			
			double x = atof(tokens[1].c_str());
			double y = atof(tokens[2].c_str());
			int owner = atoi(tokens[3].c_str());
			int numShips = atoi(tokens[4].c_str());
			int growthRate = atoi(tokens[5].c_str());

			if(gamePlayback) {
				if (desc.planets.size() > 0) *gamePlayback << ":";
				*gamePlayback << x << "," << y << "," << owner << "," << numShips << "," << growthRate;				
			}
			
			PlanetDesc planetDesc(growthRate, x, y);
			PlanetState planetState(owner, numShips);
			desc.planets.push_back(planetDesc);
			state.planets.push_back(planetState);

		} else if (tokens[0] == "F") {
			if (tokens.size() != 7) return 0;

			int owner = atoi(tokens[1].c_str());
			int numShips = atoi(tokens[2].c_str());
			int source = atoi(tokens[3].c_str());
			int destination = atoi(tokens[4].c_str());
			int totalTripLength = atoi(tokens[5].c_str());
			int turnsRemaining = atoi(tokens[6].c_str());
			
			Fleet f(owner,
					numShips,
					source,
					destination,
					totalTripLength,
					turnsRemaining);
			state.fleets.push_back(f);

		} else
			return false;
	}
	if(gamePlayback) *gamePlayback << "|" << std::flush;
	return true;
}

bool Game::ParseGamePlaybackInitial(const std::string& s) {
	clear();
	std::vector<std::string> toks = Tokenize(s, ":");
	for (size_t i = 0; i < toks.size(); ++i) {
		std::vector<std::string> planetData = Tokenize(toks[i], ",");		
		if (planetData.size() != 5) return false;
			
		double x = atof(planetData[0].c_str());
		double y = atof(planetData[1].c_str());
		int owner = atoi(planetData[2].c_str());
		int numShips = atoi(planetData[3].c_str());
		int growthRate = atoi(planetData[4].c_str());

		PlanetDesc planetDesc(growthRate, x, y);
		PlanetState planetState(owner, numShips);
		desc.planets.push_back(planetDesc);
		state.planets.push_back(planetState);
	}
	return true;	
}

bool GameState::ParseGamePlaybackChunk(const std::string& s) {
	fleets.clear();
	std::vector<std::string> items = Tokenize(s, ",");
	
	size_t numPlanets = 0;
	for (size_t j = 0; j < items.size(); ++j) {
		std::vector<std::string> fields = Tokenize(items[j], ".");
		switch(fields.size()) {
			case 2: // planet
				if(numPlanets + 1 > planets.size()) return false;
				planets[numPlanets].owner = atoi(fields[0].c_str());
				planets[numPlanets].numShips = atoi(fields[1].c_str());
				numPlanets++;
				break;
			case 6: { // fleet
				Fleet f(atoi(fields[0].c_str()),
						atoi(fields[1].c_str()),
						atoi(fields[2].c_str()),
						atoi(fields[3].c_str()),
						atoi(fields[4].c_str()),
						atoi(fields[5].c_str()));
				fleets.push_back(f);
				break;
			}
			default:
				return false;
		}
	}
	
	if(numPlanets < planets.size()) return false;
	return true;
}


// Loads a map from a test file. The text file contains a description of
// the starting state of a game. See the project wiki for a description of
// the file format. It should be called the Planet Wars Point-in-Time
// format. On success, return true. On failure, returns false.
bool Game::LoadMapFromFile(const std::string& mapFilename) {
	std::ifstream f(mapFilename.c_str());
	if(!f) return false;
	std::string s = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	return ParseGameState(s);
}



std::vector<Planet> Game::Planets() const {
	std::vector<Planet> r;
	for (size_t i = 0; i < NumPlanets(); ++i)
		r.push_back(GetPlanet(i));
	return r;
}

std::vector<Planet> Game::MyPlanets() const {
	std::vector<Planet> r;
	for (size_t i = 0; i < NumPlanets(); ++i) {
		if (state.planets[i].owner == 1)
			r.push_back(GetPlanet(i));
	}
	return r;
}

std::vector<Planet> Game::NeutralPlanets() const {
	std::vector<Planet> r;
	for (size_t i = 0; i < NumPlanets(); ++i) {
		if (state.planets[i].owner == 0)
			r.push_back(GetPlanet(i));
	}
	return r;
}

std::vector<Planet> Game::EnemyPlanets() const {
	std::vector<Planet> r;
	for (size_t i = 0; i < NumPlanets(); ++i) {
		if (state.planets[i].owner > 1)
			r.push_back(GetPlanet(i));
	}
	return r;
}

std::vector<Planet> Game::NotMyPlanets() const {
	std::vector<Planet> r;
	for (size_t i = 0; i < NumPlanets(); ++i) {
		if (state.planets[i].owner != 1)
			r.push_back(GetPlanet(i));
	}
	return r;
}

std::vector<Fleet> Game::Fleets() const {
	std::vector<Fleet> r;
	for (size_t i = 0; i < NumFleets(); ++i) {
		r.push_back(GetFleet(i));
	}
	return r;
}

std::vector<Fleet> Game::MyFleets() const {
	std::vector<Fleet> r;
	for (size_t i = 0; i < NumFleets(); ++i) {
		if (state.fleets[i].owner == 1)
			r.push_back(GetFleet(i));
	}
	return r;
}

std::vector<Fleet> Game::EnemyFleets() const {
	std::vector<Fleet> r;
	for (size_t i = 0; i < NumFleets(); ++i) {
		if (state.fleets[i].owner > 1)
			r.push_back(GetFleet(i));
	}
	return r;
}

int GameState::Production(int playerID, const GameDesc& desc) const {
	int prod = 0;
	for (size_t i = 0; i < desc.planets.size(); ++i) {
		if (planets[i].owner == playerID)
			prod += desc.planets[i].growthRate;
	}
	return prod;
}

void Game::IssueOrder(int source_planet,
                            int destination_planet,
                            int num_ships) const {
	std::cout << source_planet << " "
	<< destination_planet << " "
	<< num_ships << std::endl;
	std::cout.flush();
}

void Game::FinishTurn() const {
	std::cout << "go" << std::endl;
	std::cout.flush();
}
