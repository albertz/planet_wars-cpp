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
#include "game.h"
#include "utils.h"

void Game::WriteLogMessage(const std::string& message) {
	if(logFile)
		*logFile << message << std::endl;
}

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
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		s
		<< "P " << p->x << " " << p->y << " "
		<< PovSwitch(pov, p->owner) << " "
		<< p->numShips << " " << p->growthRate << std::endl;
	}
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
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

// Returns the distance between two planets, rounded up to the next highest
// integer. This is the number of discrete time steps it takes to get
// between the two planets.
int Game::Distance(int sourcePlanet, int destinationPlanet) {
	const Planet& source = planets[sourcePlanet];
	const Planet& destination = planets[destinationPlanet];
	double dx = source.x - destination.x;
	double dy = source.y - destination.y;
	return (int)ceil(sqrt(dx * dx + dy * dy));
}

//Resolves the battle at planet p, if there is one.
//* Removes all fleets involved in the battle
//* Sets the number of ships and owner of the planet according the outcome
void Game::FightBattle(Planet& p) {
	std::map<int,int> participants;	
	participants[p.owner] = p.numShips;
	
	for (Fleets::iterator it = fleets.begin(); it != fleets.end(); ) {
		Fleet& f = *it;
		if (f.turnsRemaining <= 0 && &GetPlanet(f.destinationPlanet) == &p) {
			participants[f.owner] += f.numShips;
			it = fleets.erase(it);
		}
		else ++it;
	}
	
	Fleet winner(0, 0);
	Fleet second(0, 0);
	for (std::map<int,int>::iterator f = participants.begin(); f != participants.end(); ++f) {
		if (f->second > second.numShips) {
			if(f->second > winner.numShips) {
				second = winner;
				winner = Fleet(f->first, f->second);
			} else {
				second = Fleet(f->first, f->second);
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
void Game::DoTimeStep() {
	// Add ships to each non-neutral planet according to its growth rate.
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner > 0) {
			p->numShips += p->growthRate;
		}
	}
	// Advance all fleets by one time step.
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		f->TimeStep();
	}
	// Determine the result of any battles
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		FightBattle(*p);
	}
	
	if(gamePlayback) {
		bool needcomma = false;
		for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
			if(needcomma) *gamePlayback << ",";
			*gamePlayback
			<< p->owner << "."
			<< p->numShips;
			needcomma = true;
		}
		for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
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

// Issue an order. This function takes num_ships off the source_planet,
// puts them into a newly-created fleet, calculates the distance to the
// destination_planet, and sets the fleet's total trip time to that
// distance. Checks that the given player_id is allowed to give the given
// order. If not, the offending player is kicked from the game. If the
// order was carried out without any issue, and everything is peachy, then
// 0 is returned. Otherwise, -1 is returned.
int Game::IssueOrder(int playerID,
					 int sourcePlanet,
					 int destinationPlanet,
					 int numShips) {
	Planet& source = planets[sourcePlanet];
	if (source.owner != playerID ||
		numShips > source.numShips ||
		numShips < 0) {
		WriteLogMessage("Dropping player " + to_string(playerID) +
						". source.Owner() = " + to_string(source.owner) + ", playerID = " +
						to_string(playerID) + ", numShips = " + to_string(numShips) +
						", source.NumShips() = " + to_string(source.numShips));
		DropPlayer(playerID);
		return -1;
	}
	source.numShips -= numShips;
	int distance = Distance(sourcePlanet, destinationPlanet);
	Fleet f(source.owner,
			numShips,
			sourcePlanet,
			destinationPlanet,
			distance,
			distance);
	fleets.push_back(f);
	return 0;
}

void Game::AddFleet(Fleet f) {
	fleets.push_back(f);
}

// Behaves just like the longer form of IssueOrder, but takes a string
// of the form "source_planet destination_planet num_ships". That is, three
// integers separated by space characters.
int Game::IssueOrder(int playerID, const std::string& order) {
	std::vector<std::string> tokens = Tokenize(order, " ");
	if (tokens.size() != 3) return -1;
	
	int sourcePlanet = atoi(tokens[0].c_str());
	int destinationPlanet = atoi(tokens[1].c_str());
	int numShips = atoi(tokens[2].c_str());
	return IssueOrder(playerID, sourcePlanet, destinationPlanet, numShips);
}

// Kicks a player out of the game. This is used in cases where a player
// tries to give an illegal order or runs over the time limit.
void Game::DropPlayer(int playerID) {
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
bool Game::IsAlive(int playerID) {
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			return true;
	}
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner == playerID)
			return true;
	}
	return false;
}

// If the game is not yet over (ie: at least two players have planets or
// fleets remaining), returns -1. If the game is over (ie: only one player
// is left) then that player's number is returned. If there are no
// remaining players, then the game is a draw and 0 is returned.
int Game::Winner() {
	std::set<int> remainingPlayers;
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		remainingPlayers.insert(p->owner);
	}
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		remainingPlayers.insert(f->owner);
	}
	remainingPlayers.erase(0);
	if (numTurns > maxGameLength) {
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
int Game::NumShips(int playerID) {
	int numShips = 0;
	for (Planets::iterator p = planets.begin(); p != planets.end(); ++p) {
		if (p->owner == playerID)
			numShips += p->numShips;
	}
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		if (f->owner == playerID)
			numShips += f->numShips;
	}
	return numShips;
}


// Parses a game state from a string. On success, returns 1. On failure,
// returns 0.
int Game::ParseGameState(const std::string& s) {
	planets.clear();
	fleets.clear();
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
			int planet_id = planets.size();

			if(gamePlayback) {
				if (planets.size() > 0) *gamePlayback << ":";
				*gamePlayback << x << "," << y << "," << owner << "," << numShips << "," << growthRate;				
			}
			
			Planet p(planet_id, owner, numShips, growthRate, x, y);
			planets.push_back(p);

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
			fleets.push_back(f);

		} else
			return 0;
	}
	if(gamePlayback) *gamePlayback << "|" << std::flush;
	return 1;
}

// Loads a map from a test file. The text file contains a description of
// the starting state of a game. See the project wiki for a description of
// the file format. It should be called the Planet Wars Point-in-Time
// format. On success, return 1. On failure, returns 0.
int Game::LoadMapFromFile(const std::string& mapFilename) {
	std::ifstream f(mapFilename.c_str());
	std::string s = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	return ParseGameState(s);
}






// ------------ drawing stuff -------------------
// commented out for now, maybe we can reimplement that later for SDL or so

#if 0
// Gets a color for a player (clamped)
private Color GetColor(int player, ArrayList<Color> colors) {
	if (player > colors.size()) {
		return Color.PINK;
	} else {
		return colors.get(player);
	}
}

private Point getPlanetPos(Planet p, double top, double left,
						   double right, double bottom, int width,
						   int height) {
	int x = (int)((p.X() - left) / (right - left) * width);
	int y = height - (int)((p.Y() - top) / (bottom - top) * height);
	return new Point(x, y);
}

// A planet's inherent radius is its radius before being transformed for
// rendering. The final rendered radii of all the planets are proportional
// to their inherent radii. The radii are scaled for maximum aesthetic
// appeal.
private double inherentRadius(Planet p) {
	return Math.sqrt(p.GrowthRate());
	//return Math.log(p.GrowthRate() + 3.0);
	//return p.GrowthRate();
}

// Renders the current state of the game to a graphics object
//
// The offset is a number between 0 and 1 that specifies how far we are
// past this game state, in units of time. As this parameter varies from
// 0 to 1, the fleets all move in the forward direction. This is used to
// fake smooth animation.
//
// On success, return an image. If something goes wrong, returns null.
void Render(int width, // Desired image width
			int height, // Desired image height
			double offset, // Real number between 0 and 1
			BufferedImage bgImage, // Background image
			ArrayList<Color> colors, // Player colors
			Graphics2D g) { // Rendering context
	Font planetFont = new Font("Sans Serif", Font.BOLD, 12);
	Font fleetFont = new Font("Sans serif", Font.BOLD, 18);
	Color bgColor = new Color(188, 189, 172);
	Color textColor = Color.BLACK;
	if (bgImage != null) {
		g.drawImage(bgImage, 0, 0, null);
	}
	// Determine the dimensions of the viewport in game coordinates.
	double top = Double.MAX_VALUE;
	double left = Double.MAX_VALUE;
	double right = Double.MIN_VALUE;
	double bottom = Double.MIN_VALUE;
	for (Planet p : planets) {
		if (p.X() < left) left = p.X();
		if (p.X() > right) right = p.X();
		if (p.Y() > bottom) bottom = p.Y();
		if (p.Y() < top) top = p.Y();
	}
	double xRange = right - left;
	double yRange = bottom - top;
	double paddingFactor = 0.1;
	left -= xRange * paddingFactor;
	right += xRange * paddingFactor;
	top -= yRange * paddingFactor;
	bottom += yRange * paddingFactor;
	Point[] planetPos = new Point[planets.size()];
	g.setFont(planetFont);
	FontMetrics fm = g.getFontMetrics(planetFont);
	// Determine the best scaling factor for the sizes of the planets.
	double minSizeFactor = Double.MAX_VALUE;
	for (int i = 0; i < planets.size(); ++i) {
		for (int j = i + 1; j < planets.size(); ++j) {
			Planet a = planets.get(i);
			Planet b = planets.get(j);
			double dx = b.X() - a.X();
			double dy = b.Y() - a.Y();
			double dist = Math.sqrt(dx * dx + dy * dy);
			double aSize = inherentRadius(a);
			double bSize = inherentRadius(b);
			double sizeFactor = dist / (Math.sqrt(a.GrowthRate()));
			minSizeFactor = Math.min(sizeFactor, minSizeFactor);
		}
	}
	minSizeFactor *= 1.2;
	// Draw the planets.
	int i = 0;
	for (Planet p : planets) {
		Point pos = getPlanetPos(p, top, left, right, bottom, width,
								 height);
		planetPos[i++] = pos;
		int x = pos.x;
		int y = pos.y;
		double size = minSizeFactor * inherentRadius(p);
		int r = (int)Math.min(size / (right - left) * width,
							  size / (bottom - top) * height);
		g.setColor(GetColor(p.Owner(), colors));
		int cx = x - r / 2;
		int cy = y - r / 2;
		g.fillOval(cx, cy, r, r);
		Color c = g.getColor();
		for (int step = 1; step >= 0; step--) {
			g.setColor(g.getColor().brighter());
			g.drawOval(x - (r-step)/2, y - (r-step)/2, r-step, r-step);
		}
		g.setColor(c);
		for (int step = 0; step < 3; step++) {
			g.drawOval(x - (r+step)/2, y - (r+step)/2, r+step, r+step);
			g.setColor(g.getColor().darker());
		}
		
		java.awt.geom.Rectangle2D bounds =
		fm.getStringBounds(Integer.toString(p.NumShips()), g);
		x -= bounds.getWidth()/2;
		y += fm.getAscent()/2;
		
		g.setColor(textColor);
		g.drawString(Integer.toString(p.NumShips()), x, y);
	}
	// Draw fleets
	g.setFont(fleetFont);
	fm = g.getFontMetrics(fleetFont);
	for (Fleets::iterator f = fleets.begin(); f != fleets.end(); ++f) {
		Point sPos = planetPos[f.SourcePlanet()];
		Point dPos = planetPos[f.DestinationPlanet()];
		double tripProgress =
		1.0 - (double)f.TurnsRemaining() / f.TotalTripLength();
		if (tripProgress > 0.99 || tripProgress < 0.01) {
			continue;
		}
		double dx = dPos.x - sPos.x;
		double dy = dPos.y - sPos.y;
		double x = sPos.x + dx * tripProgress;
		double y = sPos.y + dy * tripProgress;
		java.awt.geom.Rectangle2D textBounds =
		fm.getStringBounds(Integer.toString(f.NumShips()), g);
		g.setColor(GetColor(f.Owner(), colors).darker());
		g.drawString(Integer.toString(f.NumShips()),
					 (int)(x-textBounds.getWidth()/2),
					 (int)(y+textBounds.getHeight()/2));
	}
}
#endif
