// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>

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

#endif
