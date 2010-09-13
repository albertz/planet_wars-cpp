/*
 *  viewer.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#include <SDL.h>
#include <cmath>
#include <limits>
#include "viewer.h"
#include "gfx.h"
#include "game.h"

struct Point { int x, y; Point(int _x = 0, int _y = 0) : x(_x), y(_y) {} };

// Gets a color for a player (clamped)
static Color GetColor(int player) {
	static const Color colors[] = {
		Color(255, 64, 64),
		Color(64, 255, 64),
		Color(64, 64, 255),
		Color(255, 255, 64)
	};
	
	if (player < 0 || (size_t)player >= sizeof(colors)/sizeof(colors[0]))
		return colors[sizeof(colors)/sizeof(colors[0])-1];
	else
		return colors[player];
}

static Point getPlanetPos(const Planet& p, double top, double left,
						  double right, double bottom, int width,
						  int height) {
	int x = (int)((p.x - left) / (right - left) * width);
	int y = height - (int)((p.y - top) / (bottom - top) * height);
	return Point(x, y);
}

// A planet's inherent radius is its radius before being transformed for
// rendering. The final rendered radii of all the planets are proportional
// to their inherent radii. The radii are scaled for maximum aesthetic
// appeal.
static double inherentRadius(const Planet& p) {
	return sqrt(p.growthRate);
	//return log(p.GrowthRate() + 3.0);
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
void Render(const Game& game,
			int width, // Desired image width
			int height, // Desired image height
			double offset, // Real number between 0 and 1
			SDL_Surface* surf) { // Rendering context
	static const Color bgColor(188, 189, 172);
	static const Color textColor(0, 0, 0);

	// Determine the dimensions of the viewport in game coordinates.
	double top = std::numeric_limits<double>::max();
	double left = std::numeric_limits<double>::max();
	double right = std::numeric_limits<double>::min();
	double bottom = std::numeric_limits<double>::min();
	for (Game::Planets::const_iterator p = game.planets.begin(); p != game.planets.end(); ++p) {
		if (p->x < left) left = p->x;
		if (p->x > right) right = p->x;
		if (p->y > bottom) bottom = p->y;
		if (p->y < top) top = p->y;
	}
	double xRange = right - left;
	double yRange = bottom - top;
	double paddingFactor = 0.1;
	left -= xRange * paddingFactor;
	right += xRange * paddingFactor;
	top -= yRange * paddingFactor;
	bottom += yRange * paddingFactor;
	std::vector<Point> planetPos(game.planets.size());
	
	// Determine the best scaling factor for the sizes of the planets.
	double minSizeFactor = std::numeric_limits<double>::max();
	for (size_t i = 0; i < game.planets.size(); ++i) {
		for (size_t j = i + 1; j < game.planets.size(); ++j) {
			const Planet& a = game.planets[i];
			const Planet& b = game.planets[j];
			double dx = b.x - a.x;
			double dy = b.y - a.y;
			double dist = sqrt(dx * dx + dy * dy);
			//double aSize = inherentRadius(a);
			//double bSize = inherentRadius(b);
			double sizeFactor = dist / sqrt(a.growthRate);
			minSizeFactor = std::min(sizeFactor, minSizeFactor);
		}
	}
	minSizeFactor *= 1.2;
	
	// Draw the planets.
	int i = 0;
	for (Game::Planets::const_iterator p = game.planets.begin(); p != game.planets.end(); ++p) {
		Point pos = getPlanetPos(*p, top, left, right, bottom, width,
								 height);
		planetPos[i++] = pos;
		int x = pos.x;
		int y = pos.y;
		double size = minSizeFactor * inherentRadius(*p);
		int r = (int)std::min(size / (right - left) * width,
							  size / (bottom - top) * height);
		Color c = GetColor(p->owner);
		int cx = x - r / 2;
		int cy = y - r / 2;
		DrawCircleFilled(surf, cx, cy, r, r, c);
		for (int step = 1; step >= 0; step--)
			DrawCircleFilled(surf, x - (r-step)/2, y - (r-step)/2, r-step, r-step, c * 1.2f);
		for (int step = 1; step < 3; step++)
			DrawCircleFilled(surf, x - (r+step)/2, y - (r+step)/2, r+step, r+step, c * 0.8f);
		
		DrawText(surf, to_string(p->numShips), c, x, y, true);
	}

	// Draw fleets
	for (Game::Fleets::const_iterator f = game.fleets.begin(); f != game.fleets.end(); ++f) {
		Point sPos = planetPos[f->sourcePlanet];
		Point dPos = planetPos[f->destinationPlanet];
		double tripProgress = 1.0 - (double)f->turnsRemaining / f->totalTripLength;
		if (tripProgress > 0.99 || tripProgress < 0.01) continue;
		double dx = dPos.x - sPos.x;
		double dy = dPos.y - sPos.y;
		double x = sPos.x + dx * tripProgress;
		double y = sPos.y + dy * tripProgress;
		
		Color c = GetColor(f->owner) * 0.8f;
		DrawText(surf, to_string(f->numShips), c, x, y, true);
	}
}

void DrawGame(const Game& game, SDL_Surface* surf) {
	Render(game, surf->w, surf->h, 0.0, surf);
}
