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

#ifndef __PW__GAME_H__
#define __PW__GAME_H__

#include <string>
#include <ostream>
#include <vector>
#include <list>
#include <cmath>

// This class stores details about one fleet. There is one of these classes
// for each fleet that is in flight at any given time.
struct Fleet {
	int owner;
	int numShips;
	int sourcePlanet;
	int destinationPlanet;
	int totalTripLength;
	int turnsRemaining;
	
	// Initializes a fleet.
	Fleet(int _owner,
		  int _num_ships,
		  int _source_planet = -1,
		  int _destination_planet = -1,
		  int _total_trip_length = -1,
		  int _turns_remaining = -1)
	: owner(_owner), numShips(_num_ships), sourcePlanet(_source_planet),
	destinationPlanet(_destination_planet), totalTripLength(_total_trip_length),
	turnsRemaining(_turns_remaining) {}
	
	void TimeStep() {
		if (turnsRemaining > 0)
            --turnsRemaining;
        else
            turnsRemaining = 0;
	}
	
	void Kill() { owner = numShips = turnsRemaining = 0; }
};

// all gamestate relevant information about a planet
struct PlanetState {
	int owner;
	int numShips;
	
	PlanetState(int _owner = 0, int _num_ships = 0)
	: owner(_owner), numShips(_num_ships) {}
};

// planet description. all the game global constants
struct PlanetDesc {
	int growthRate;
	double x, y;
	
	PlanetDesc(int _growthRate, double _x, double _y)
	: growthRate(_growthRate), x(_x), y(_y) {}
};

// all together. this can be handy
struct Planet : PlanetDesc, PlanetState {
	int planetId;
	
	Planet(int _planet_id, const PlanetDesc& desc, const PlanetState& state)
	: PlanetDesc(desc), PlanetState(state), planetId(_planet_id) {}
};

struct GameDesc;

struct GameState {
	typedef std::vector<PlanetState> Planets;
	typedef std::vector<Fleet> Fleets;
	Planets planets;
	Fleets fleets;

	// Parses a chunk from a game playback.
	// NOTE: the planets.size must fit!
	bool ParseGamePlaybackChunk(const std::string& s);

	// Executes one time step.
	//   * Planet bonuses are added to non-neutral planets.
	//   * Fleets are advanced towards their destinations.
	//   * Fleets that arrive at their destination are dealt with.
	void DoTimeStep(const GameDesc& desc);	

	//Resolves the battle at planet p, if there is one.
    //* Removes all fleets involved in the battle
    //* Sets the number of ships and owner of the planet according the outcome
    void __FightBattle(PlanetState& p);

	// Execute an order. This function takes num_ships off the source_planet,
	// puts them into a newly-created fleet, calculates the distance to the
	// destination_planet, and sets the fleet's total trip time to that
	// distance. Checks that the given player_id is allowed to give the given
	// order. If the order was carried out without any issue, and everything
	// is peachy, then true is returned. Otherwise, false is returned.
	bool ExecuteOrder(const GameDesc& desc,
					  int playerID,
					  int sourcePlanet,
					  int destinationPlanet,
					  int numShips);		
	
	// Kicks a player out of the game. This is used in cases where a player
	// tries to give an illegal order or runs over the time limit.
	void DropPlayer(int playerID);		
	
	// Returns true if the named player owns at least one planet or fleet.
	// Otherwise, the player is deemed to be dead and false is returned.
	bool IsAlive(int playerID) const;

	// If the game is not yet over (ie: at least two players have planets or
	// fleets remaining), returns -1. If the game is over (ie: only one player
	// is left) then that player's number is returned. If there are no
	// remaining players, then the game is a draw and 0 is returned.
	int Winner(bool maxTurnsReached) const;
	
	// Returns the number of ships that the current player has, either located
	// on planets or in flight.
	int NumShips(int playerID) const;
	
	// Returns the production of the given player.
    int Production(int playerID, const GameDesc& desc) const;

	int HighestPlayerID() const;
};

struct GameDesc {
	typedef std::vector<PlanetDesc> Planets;
	Planets planets;

    // Returns the distance between two planets, rounded up to the next highest
    // integer. This is the number of discrete time steps it takes to get
    // between the two planets.
    int Distance(int sourcePlanet, int destinationPlanet) const {
		const PlanetDesc& source = planets[sourcePlanet];
		const PlanetDesc& destination = planets[destinationPlanet];
		double dx = source.x - destination.x;
		double dy = source.y - destination.y;
		return (int)ceil(sqrt(dx * dx + dy * dy));		
	}
};

struct Game {
	GameDesc desc;
	GameState state;
	
	// The maximum length of the game in turns. After this many turns, the game
	// will end, with whoever has the most ships as the winner. If there is no
	// player with the most ships, then the game is a draw.
	int maxGameLength;
	int numTurns;
	
	// This is the game playback string. It's a complete description of the
	// game. It can be read by a visualization program to visualize the game.
	std::ostream* gamePlayback;
	
	// This is the name of the file in which to write log messages.
	std::ostream* logFile;

    // This constructor does not actually initialize the game object. You must
    // always call Init() before the game object will be in any kind of
    // coherent state.
	Game(int _maxGameLength = 0, std::ostream* _gamePlayback = NULL, std::ostream* _logFile = NULL)
	: maxGameLength(_maxGameLength), numTurns(0),
	gamePlayback(_gamePlayback), logFile(_logFile) {}

	void clear() { desc = GameDesc(); state = GameState(); }
	
	// Parses a game state from a string. On success, returns true. On failure, returns false.
	bool ParseGameState(const std::string& s);
	bool ParseGamePlaybackInitial(const std::string& s);
	
	// Loads a map from a text file. The text file contains a description of
	// the starting state of a game. See the project wiki for a description of
	// the file format. It should be called the Planet Wars Point-in-Time
	// format. On success, return true. On failure, returns false.
	bool LoadMapFromFile(const std::string& mapFilename);
		
	// Returns the number of planets. Planets are numbered starting with 0.
	size_t NumPlanets() const { return desc.planets.size(); }
		
    // Returns the number of fleets.
	size_t NumFleets() const { return state.fleets.size(); }
		
	// Writes a string which represents the current game state. No point-of-
    // view switching is performed.
    std::string toString() { return PovRepresentation(-1); }
	
    // Writes a string which represents the current game state. This string
    // conforms to the Point-in-Time format from the project Wiki.
    //
    // Optionally, you may specify the pov (Point of View) parameter. The pov
    // parameter is a player number. If specified, the player numbers 1 and pov
    // will be swapped in the game state output. This is used when sending the
    // game state to individual players, so that they can always assume that
    // they are player number 1.
    std::string PovRepresentation(int pov);
	
    // Carries out the point-of-view switch operation, so that each player can
    // always assume that he is player number 1. There are three cases.
    // 1. If pov < 0 then no pov switching is being used. Return player_id.
    // 2. If player_id == pov then return 1 so that each player thinks he is
    //    player number 1.
    // 3. If player_id == 1 then return pov so that the real player 1 looks
    //    like he is player number "pov".
    // 4. Otherwise return player_id, since players other than 1 and pov are
    //    unaffected by the pov switch.
    static int PovSwitch(int pov, int playerID);
	
	void DoTimeStep();
	
	// Parses a string of the form "source_planet destination_planet num_ships"
	// and calls state.ExecuteOrder. If that fails, the player is dropped.
	bool ExecuteOrder(int playerID, const std::string& order);
	
	void WriteLogMessage(const std::string& message) {
		if(logFile) *logFile << message << std::endl;
	}
	
	
	// --------------- functions to provide original-kind-of interface ------------
	
	// Returns the planet with the given planet_id. There are NumPlanets()
	// planets. They are numbered starting at 0.
	Planet GetPlanet(int planet_id) const {
		return Planet(planet_id, desc.planets[planet_id], state.planets[planet_id]);
	}
	
	// Returns the fleet with the given fleet_id. Fleets are numbered starting
	// with 0. There are NumFleets() fleets. fleet_id's are not consistent from
	// one turn to the next.
	const Fleet& GetFleet(int fleet_id) const {
		return state.fleets[fleet_id];
	}
	
	// Returns a list of all the planets.
	std::vector<Planet> Planets() const;
	
	// Return a list of all the planets owned by the current player. By
	// convention, the current player is always player number 1.
	std::vector<Planet> MyPlanets() const;
	
	// Return a list of all neutral planets.
	std::vector<Planet> NeutralPlanets() const;
	
	// Return a list of all the planets owned by rival players. This excludes
	// planets owned by the current player, as well as neutral planets.
	std::vector<Planet> EnemyPlanets() const;
	
	// Return a list of all the planets that are not owned by the current
	// player. This includes all enemy planets and neutral planets.
	std::vector<Planet> NotMyPlanets() const;
	
	// Return a list of all the fleets.
	std::vector<Fleet> Fleets() const;
	
	// Return a list of all the fleets owned by the current player.
	std::vector<Fleet> MyFleets() const;
	
	// Return a list of all the fleets owned by enemy players.
	std::vector<Fleet> EnemyFleets() const;
	
	int Winner() const { return state.Winner((maxGameLength >= 0) && (numTurns > maxGameLength)); }

	int Production(int playerID) const { return state.Production(playerID, desc); }
	
	
	// --------------- bot side control ----------------------
	
	// Sends an order to the game engine. The order is to send num_ships ships
	// from source_planet to destination_planet. The order must be valid, or
	// else your bot will get kicked and lose the game. For example, you must own
	// source_planet, and you can't send more ships than you actually have on
	// that planet.
	void IssueOrder(int source_planet,
					int destination_planet,
					int num_ships) const;
	
	// Sends a message to the game engine letting it know that you're done
	// issuing orders for now.
	void FinishTurn() const;
	
	
};

#endif
