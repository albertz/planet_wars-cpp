/*
 *  viewer.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#include <SDL.h>
#include "viewer.h"
#include "gfx.h"
#include "game.h"


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

void DrawGame(const Game& game, SDL_Surface* surf) {
	FillSurface(surf, Color(0, 0, 0));

	
}
