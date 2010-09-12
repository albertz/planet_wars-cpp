#include "PlanetWars.h"
#include "utils.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

PlanetWars::PlanetWars(const std::string& gameState) {
  ParseGameState(gameState);
}

int PlanetWars::NumPlanets() const {
  return planets_.size();
}

const Planet& PlanetWars::GetPlanet(int planet_id) const {
  return planets_[planet_id];
}

int PlanetWars::NumFleets() const {
  return fleets_.size();
}

const Fleet& PlanetWars::GetFleet(int fleet_id) const {
  return fleets_[fleet_id];
}

std::vector<Planet> PlanetWars::Planets() const {
  std::vector<Planet> r;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    r.push_back(p);
  }
  return r;
}

std::vector<Planet> PlanetWars::MyPlanets() const {
  std::vector<Planet> r;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.owner == 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::NeutralPlanets() const {
  std::vector<Planet> r;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.owner == 0) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::EnemyPlanets() const {
  std::vector<Planet> r;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.owner > 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Planet> PlanetWars::NotMyPlanets() const {
  std::vector<Planet> r;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    if (p.owner != 1) {
      r.push_back(p);
    }
  }
  return r;
}

std::vector<Fleet> PlanetWars::Fleets() const {
  std::vector<Fleet> r;
  for (size_t i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    r.push_back(f);
  }
  return r;
}

std::vector<Fleet> PlanetWars::MyFleets() const {
  std::vector<Fleet> r;
  for (size_t i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    if (f.owner == 1) {
      r.push_back(f);
    }
  }
  return r;
}

std::vector<Fleet> PlanetWars::EnemyFleets() const {
  std::vector<Fleet> r;
  for (size_t i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    if (f.owner > 1) {
      r.push_back(f);
    }
  }
  return r;
}

std::string PlanetWars::ToString() const {
  std::stringstream s;
  for (size_t i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    s << "P " << p.x << " " << p.y << " " << p.owner
      << " " << p.numShips << " " << p.growthRate << std::endl;
  }
  for (size_t i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    s << "F " << f.owner << " " << f.numShips << " "
      << f.sourcePlanet << " " << f.destinationPlanet << " "
      << f.totalTripLength << " " << f.turnsRemaining << std::endl;
  }
  return s.str();
}

int PlanetWars::Distance(int source_planet, int destination_planet) const {
  const Planet& source = planets_[source_planet];
  const Planet& destination = planets_[destination_planet];
  double dx = source.x - destination.x;
  double dy = source.y - destination.y;
  return (int)ceil(sqrt(dx * dx + dy * dy));
}

void PlanetWars::IssueOrder(int source_planet,
                            int destination_planet,
                            int num_ships) const {
  std::cout << source_planet << " "
            << destination_planet << " "
            << num_ships << std::endl;
  std::cout.flush();
}

bool PlanetWars::IsAlive(int player_id) const {
  for (size_t i = 0; i < planets_.size(); ++i) {
    if (planets_[i].owner == player_id) {
      return true;
    }
  }
  for (size_t i = 0; i < fleets_.size(); ++i) {
    if (fleets_[i].owner == player_id) {
      return true;
    }
  }
  return false;
}

int PlanetWars::ParseGameState(const std::string& s) {
  planets_.clear();
  fleets_.clear();
  std::vector<std::string> lines = Tokenize(s, "\n");
  int planet_id = 0;
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i];
    size_t comment_begin = line.find_first_of('#');
    if (comment_begin != std::string::npos) {
      line = line.substr(0, comment_begin);
    }
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.size() == 0) {
      continue;
    }
    if (tokens[0] == "P") {
      if (tokens.size() != 6) {
        return 0;
      }
      Planet p(planet_id++,              // The ID of this planet
	       atoi(tokens[3].c_str()),  // Owner
               atoi(tokens[4].c_str()),  // Num ships
               atoi(tokens[5].c_str()),  // Growth rate
               atof(tokens[1].c_str()),  // X
               atof(tokens[2].c_str())); // Y
      planets_.push_back(p);
    } else if (tokens[0] == "F") {
      if (tokens.size() != 7) {
        return 0;
      }
      Fleet f(atoi(tokens[1].c_str()),  // Owner
              atoi(tokens[2].c_str()),  // Num ships
              atoi(tokens[3].c_str()),  // Source
              atoi(tokens[4].c_str()),  // Destination
              atoi(tokens[5].c_str()),  // Total trip length
              atoi(tokens[6].c_str())); // Turns remaining
      fleets_.push_back(f);
    } else {
      return 0;
    }
  }
  return 1;
}

void PlanetWars::FinishTurn() const {
  std::cout << "go" << std::endl;
  std::cout.flush();
}
