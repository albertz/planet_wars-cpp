// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>

// This is a utility class that parses strings.
class StringUtil {
 public:
  // Tokenizes a string s into tokens. Tokens are delimited by any of the
  // characters in delimiters. Blank tokens are omitted.
  static void Tokenize(const std::string& s,
                       const std::string& delimiters,
                       std::vector<std::string>& tokens);

  // A more convenient way of calling the Tokenize() method.
  static std::vector<std::string> Tokenize(
                       const std::string& s,
                       const std::string& delimiters = std::string(" "));
};

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
		if (turnsRemaining > 0) {
            --turnsRemaining;
        } else {
            turnsRemaining = 0;
        }
	}
};

// Stores information about one planet. There is one instance of this class
// for each planet on the map.
struct Planet {
	int planetId;
	int owner;
	int numShips;
	int growthRate;
	double x, y;

	// Initializes a planet.
	Planet(int _planet_id,
		   int _owner,
		   int _num_ships,
		   int _growth_rate,
		   double _x,
		   double _y)
	: planetId(_planet_id), owner(_owner), numShips(_num_ships),
	growthRate(_growth_rate), x(_x), y(_y) {}
};

class PlanetWars {
 public:
	PlanetWars() {} // dummy
  // Initializes the game state given a string containing game state data.
  PlanetWars(const std::string& game_state);

  // Returns the number of planets on the map. Planets are numbered starting
  // with 0.
  int NumPlanets() const;

  // Returns the planet with the given planet_id. There are NumPlanets()
  // planets. They are numbered starting at 0.
  const Planet& GetPlanet(int planet_id) const;

  // Returns the number of fleets.
  int NumFleets() const;

  // Returns the fleet with the given fleet_id. Fleets are numbered starting
  // with 0. There are NumFleets() fleets. fleet_id's are not consistent from
  // one turn to the next.
  const Fleet& GetFleet(int fleet_id) const;

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

  // Writes a string which represents the current game state. This string
  // conforms to the Point-in-Time format from the project Wiki.
  std::string ToString() const;

  // Returns the distance between two planets, rounded up to the next highest
  // integer. This is the number of discrete time steps it takes to get between
  // the two planets.
  int Distance(int source_planet, int destination_planet) const;

  // Sends an order to the game engine. The order is to send num_ships ships
  // from source_planet to destination_planet. The order must be valid, or
  // else your bot will get kicked and lose the game. For example, you must own
  // source_planet, and you can't send more ships than you actually have on
  // that planet.
  void IssueOrder(int source_planet,
		  int destination_planet,
		  int num_ships) const;

  // Returns true if the named player owns at least one planet or fleet.
  // Otherwise, the player is deemed to be dead and false is returned.
  bool IsAlive(int player_id) const;

  // Returns the number of ships that the given player has, either located
  // on planets or in flight.
  int NumShips(int player_id) const;

  // Sends a message to the game engine letting it know that you're done
  // issuing orders for now.
  void FinishTurn() const;

 private:
  // Parses a game state from a string. On success, returns 1. On failure,
  // returns 0.
  int ParseGameState(const std::string& s);

  // Store all the planets and fleets. OMG we wouldn't wanna lose all the
  // planets and fleets, would we!?
  std::vector<Planet> planets_;
  std::vector<Fleet> fleets_;
};

#endif
