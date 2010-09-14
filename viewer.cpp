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
#include "vec.h"

// Gets a color for a player (clamped)
static Color GetColor(int player) {
	static const Color colors[] = {
		Color(68, 85, 85),
		Color(119, 170, 204),
		Color(204, 0, 0),
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
	return sqrt(p.growthRate) * 0.8;
	//return log(p.GrowthRate() + 3.0);
	//return p.GrowthRate();
}

// Renders the current state of the game to a graphics object
//
// The offset is a number between 0 and 1 that specifies how far we are
// past this game state, in units of time. As this parameter varies from
// 0 to 1, the fleets all move in the forward direction. This is used to
// fake smooth animation.
void DrawGame(const Game& game, SDL_Surface* surf, double offset) {
	static const Color textColor(255, 255, 255);
	const int width = surf->w;
	const int height = surf->h;
	
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
	size_t i = 0;
	for (Game::Planets::const_iterator p = game.planets.begin(); p != game.planets.end(); ++p) {
		Point pos = getPlanetPos(*p, top, left, right, bottom, width,
								 height);
		planetPos[i++] = pos;
		int x = pos.x;
		int y = pos.y;
		double size = minSizeFactor * inherentRadius(*p);
		int r = (int)std::min(size / (right - left) * width,
							  size / (bottom - top) * height);
		if(r > 0) {
			Color c = GetColor(p->owner);
			DrawCircleFilled(surf, x, y, r+1, r+1, c * 1.2f);
			DrawCircleFilled(surf, x, y, r, r, c);
		}
		DrawText(surf, to_string(p->numShips), textColor, x, y, true);
	}

	// Draw fleets
	for (Game::Fleets::const_iterator f = game.fleets.begin(); f != game.fleets.end(); ++f) {
		Point sPos = planetPos[f->sourcePlanet];
		Point dPos = planetPos[f->destinationPlanet];
		double tripProgress = 1.0 - (double)f->turnsRemaining / f->totalTripLength;
		if (tripProgress > 0.99 || tripProgress < 0.01) continue;
		VecD delta = dPos - sPos;
		VecD pos = sPos + delta * tripProgress;
		Color c = GetColor(f->owner) * 1.3;
		std::string txt = to_string(f->numShips);
		{
			VecD ndelta = delta.Normalize();
			VecD txtSize = VecD(TextGetSize(txt)) * 0.5;
			VecD txtBorderPt;
			if(ndelta.x == 0 || fabs(ndelta.y/ndelta.x) >= txtSize.y/txtSize.x) {
				txtBorderPt.x = fabs(txtSize.y * ndelta.x / ndelta.y) * SIGN(ndelta.x);
				txtBorderPt.y = txtSize.y * SIGN(ndelta.y);
			}
			else {
				txtBorderPt.x = txtSize.x * SIGN(ndelta.x);
				txtBorderPt.y = fabs(txtSize.x * ndelta.y / ndelta.x) * SIGN(ndelta.y);
			}
			txtBorderPt += pos + ndelta * 2;
			static const MatD ROT90 = MatD::Rotation(0,1);
			static const double LEN = 5;
			DrawLine(surf, txtBorderPt, txtBorderPt + ndelta * LEN, c);
			DrawLine(surf, txtBorderPt + (MatD(1)+ROT90) * ndelta * LEN*0.5, txtBorderPt + ndelta * LEN, c);
			DrawLine(surf, txtBorderPt + (MatD(1)-ROT90) * ndelta * LEN*0.5, txtBorderPt + ndelta * LEN, c);
		}
		DrawText(surf, txt, c, pos.x, pos.y, true);
	}
}
