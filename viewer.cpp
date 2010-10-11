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
#include <iostream>
#include "viewer.h"
#include "gfx.h"
#include "game.h"
#include "vec.h"

using namespace std;

// Gets a color for a player (clamped)
Color GetDefaultPlayerPlanetColor(int player) {
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

static Point getPlanetPos(const PlanetDesc& p, double top, double left,
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
static double inherentRadius(const PlanetDesc& p) {
	return sqrt((double)p.growthRate) * 0.4 + 0.3;
	//return sqrt((double)p.growthRate) * 0.5;
	//return log(p.GrowthRate() + 3.0);
	//return p.GrowthRate();
}

static Color getPlanetColor(const GameDebugInfo* info, int planet, int player) {
	if(info == NULL) return GetDefaultPlayerPlanetColor(player);
	std::map<int, Color>::const_iterator f = info->planetColor.find(planet);
	if(f == info->planetColor.end()) return GetDefaultPlayerPlanetColor(player);
	return f->second;
}

static std::string getPlanetDebugText(const GameDebugInfo* info, int planet) {
	if(info == NULL) return "";
	std::map<int, std::string>::const_iterator f = info->planetInfo.find(planet);
	if(f == info->planetInfo.end()) return "";
	return "\n<" + f->second + ">";
}

// Renders the current state of the game to a graphics object
//
// The offset is a number between 0 and 1 that specifies how far we are
// past this game state, in units of time. As this parameter varies from
// 0 to 1, the fleets all move in the forward direction. This is used to
// fake smooth animation.
void DrawGame(const GameDesc& desc, const GameState& state, SDL_Surface* surf, double offset, const GameDebugInfo* debugInfo) {
	static const Color planetIdColor(255, 228, 0);
	static const Color textColor(255, 255, 255);
	static const Color dbgTextColor(200, 255, 200);
	const int width = surf->w;
	const int height = surf->h;
	
	// Determine the dimensions of the viewport in game coordinates.
	double top = std::numeric_limits<double>::max();
	double left = std::numeric_limits<double>::max();
	double right = std::numeric_limits<double>::min();
	double bottom = std::numeric_limits<double>::min();
	for (GameDesc::Planets::const_iterator p = desc.planets.begin(); p != desc.planets.end(); ++p) {
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
	std::vector<Point> planetPos(desc.planets.size());
	
	// Determine the best scaling factor for the sizes of the planets.
	double minSizeFactor = std::numeric_limits<double>::max();
	for (size_t i = 0; i < desc.planets.size(); ++i) {
		for (size_t j = i + 1; j < desc.planets.size(); ++j) {
			const PlanetDesc& a = desc.planets[i];
			const PlanetDesc& b = desc.planets[j];
			double dx = b.x - a.x;
			double dy = b.y - a.y;
			double dist = sqrt(dx * dx + dy * dy);
			//double aSize = inherentRadius(a);
			//double bSize = inherentRadius(b);
			double sizeFactor = dist / sqrt((double)a.growthRate);
			minSizeFactor = std::min(sizeFactor, minSizeFactor);
		}
	}
	minSizeFactor *= 1.2;
	
	// Draw the planets.
	size_t i = 0;
	for (size_t p = 0; p < desc.planets.size(); ++p) {
		Point pos = getPlanetPos(desc.planets[p], top, left, right, bottom, width, height);
		planetPos[i++] = pos;
		int x = pos.x;
		int y = pos.y;
		double size = minSizeFactor * inherentRadius(desc.planets[p]);
		int r = (int)std::min(size / (right - left) * width,
							  size / (bottom - top) * height);
		if(r > 0) {
			Color c = getPlanetColor(debugInfo, p, state.planets[p].owner);
			DrawCircleFilled(surf, x, y, r+1, r+1, c * 1.2f);
			DrawCircleFilled(surf, x, y, r, r, c);
		}
	}

	// Draw the planet texts.
	for (size_t p = 0; p < desc.planets.size(); ++p) {
		int x = planetPos[p].x;
		int y = planetPos[p].y;
		DrawText(surf, to_string(state.planets[p].numShips), textColor, x, y, true);
		DrawText(surf, to_string(p), planetIdColor, x, y-10, true);
		std::string debugTxt = getPlanetDebugText(debugInfo, p);
		if(debugTxt != "") {
			Vec s = TextGetSize(debugTxt);
			DrawText(surf, debugTxt, dbgTextColor, x - s.x / 2, y + 2);
		}
	}
	
	// Draw fleets
	for (Fleets::const_iterator f = state.fleets.begin(); f != state.fleets.end(); ++f) {
		Point sPos = planetPos[f->sourcePlanet];
		Point dPos = planetPos[f->destinationPlanet];
		double tripProgress = 1.0 - (double(f->turnsRemaining) - offset) / f->totalTripLength;
		if (tripProgress > 0.99 || tripProgress < 0.01) continue;
		VecD delta = dPos - sPos;
		VecD pos = sPos + delta * tripProgress;
		Color c = GetDefaultPlayerPlanetColor(f->owner) * 1.3;
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

void Viewer::move(int d) {
	if(!ready()) return;
	if(!withAnimation) { if(d >= 0) _next(); else _last(); return; }
	if(SIGN(offsetToGo) != SIGN(d)) offsetToGo = 0;
	
	offsetToGo += d;
	dtForAnimation = 100;	
}

void Viewer::frame(SDL_Surface* surf, long dt) {
	if(!ready()) return;

	if(offsetToGo > 0 && isAtEnd()) {
		offsetToGo = 0;
		dtForAnimation = 0;
	}

	if(offsetToGo < 0 && isAtStart()) {
		offsetToGo = 0;
		dtForAnimation = 0;
	}
	
	double aniFrac = (dtForAnimation > 0) ? CLAMP(double(dt) / dtForAnimation, 0.0, 1.0) : 1.0;
	dtForAnimation -= dt; if(dtForAnimation < 0) dtForAnimation = 0;	
	Offset dOffset = offsetToGo * aniFrac;
	if(dOffset == 0 && offsetToGo != 0) dOffset = Offset::minGreater0() * SIGN(offsetToGo);
	Offset oldOffsetToGo = offsetToGo;
	offsetToGo -= dOffset;
	if(SIGN(offsetToGo) != SIGN(oldOffsetToGo)) offsetToGo = 0;
	if(labs(offsetToGo.number) <= 1) offsetToGo = 0;

	int numAbsSteps = 
	(
	 (oldOffsetToGo - Offset::minGreater0()).floor() -
	 (offsetToGo - Offset::minGreater0()).floor()
	 ).asInt();
	
	//if(oldOffsetToGo != 0)
	//cout << "frame: dt=" << dt << ", oldoff=" << oldOffsetToGo.number << ", off=" << offsetToGo.number << ", steps=" << numAbsSteps;

	bool success = true;
	while(numAbsSteps > 0) { success &= _next(); --numAbsSteps; }
	while(numAbsSteps < 0) { success &= _last(); ++numAbsSteps; }
	
	if(offsetToGo == 0 || dtForAnimation == 0 || !success) {
		offsetToGo = 0;
		dtForAnimation = 0;
	}
	
	double offset = ((Offset(1) - offsetToGo % 1) % 1).asDouble();
	//if(oldOffsetToGo != 0) cout << ", doffset=" << offset << endl;
	DrawGame(gameDesc, currentState->state, surf, offset, &currentState->debugInfo);
	
	std::string txtTurn = to_string(currentState->index + 1) + "/" + to_string(gameStates.size()) + ":";
	int x = 2, y = 2;
	DrawText(surf, txtTurn, Color(255,255,255), x, y);
	x += 10 + TextGetSize(txtTurn).x;
	const int upperPlayer = currentState->state.HighestPlayerID();
	for(int p = 1; p <= upperPlayer; ++p) {
		std::string txtPlayer = to_string(currentState->state.NumShips(p)) + "/" + to_string(currentState->state.Production(p, gameDesc));
		DrawText(surf, txtPlayer, GetDefaultPlayerPlanetColor(p), x, y);
		x += 10 + TextGetSize(txtPlayer).x;
	}
}


int screenw = 500, screenh = 500, screenbpp = 0;

static void fixScreenWH() {
	screenw = (screenw + screenh) / 2;
	screenh = screenw;
}

static Viewer viewer;
static bool pressedAnyKey = false;

#define EVENT_STDIN_INITIAL 1
#define EVENT_STDIN_CHUNK 2
#define EVENT_STDIN_DEBUG 3

#define SETVIDEOMODE SDL_SetVideoMode(screenw, screenh, screenbpp, SDL_RESIZABLE)

// returns false for exit
static bool HandleEvent(const SDL_Event& event) {
	switch(event.type) {
		case SDL_QUIT: return false;
		case SDL_VIDEORESIZE:
			screenw = event.resize.w;
			screenh = event.resize.h;
			fixScreenWH();
			SETVIDEOMODE;
			break;
		case SDL_USEREVENT: {
			switch(event.user.code) {
				case EVENT_STDIN_INITIAL: {
					std::auto_ptr<Game> game( (Game*)event.user.data1 );
					viewer.gameDesc = game->desc;
					viewer.gameStates.push_back(ViewerState(0, game->state));
					viewer.init();
					break;
				}
				case EVENT_STDIN_CHUNK: {
					std::auto_ptr<GameState> gameState( (GameState*)event.user.data1 );
					viewer.gameStates.push_back(ViewerState(viewer.gameStates.size(), *gameState));
					if(!pressedAnyKey) {
						viewer.offsetToGo++;
						viewer.dtForAnimation += 200;
					}
					break;
				}
				case EVENT_STDIN_DEBUG: {
					std::auto_ptr<GameDebugInfo> debugInfo( (GameDebugInfo*)event.user.data1 );
					assert(viewer.gameStates.size() > 0);
					viewer.gameStates.back().debugInfo = *debugInfo;
					if(!pressedAnyKey) {
						viewer.offsetToGo++;
						viewer.dtForAnimation += 200;
					}
					break;
				}
				default: assert(false);
			}
			break;
		}
		case SDL_KEYDOWN:
			if(!pressedAnyKey) { viewer.offsetToGo %= 1; viewer.dtForAnimation = 100; }
			pressedAnyKey = true;
			switch(event.key.keysym.sym) {
				case SDLK_LEFT: viewer.last(); break;
				case SDLK_RIGHT: viewer.next(); break;
				case SDLK_q: return false;
				default: break; // ignore
			}
			break;
		default:
			break;
	}
	return true;
}

static const Color backgroundCol(0,0,0);

bool Viewer_initWindow(const std::string& windowTitle) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		cerr << "init SDL failed: " << SDL_GetError() << endl;
		return false;
	}
	
	// we want to redraw when these occur
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	SDL_EventState(SDL_VIDEOEXPOSE, SDL_ENABLE);
	
	// init window
	if(SETVIDEOMODE == NULL) {
		cerr << "setting video mode " << screenw << "x" << screenh << "x" << screenbpp << " failed: "
		<< SDL_GetError() << endl;
		return false;
	}
	FillSurface(SDL_GetVideoSurface(), backgroundCol);
	
	SDL_WM_SetCaption(windowTitle.c_str(), NULL);
	SDL_EnableKeyRepeat(100, 30);
	return true;
}

void Viewer_mainLoop() {
	long lastTime = currentTimeMillis();
	while(true) {
		bool haveEvent = false;
		SDL_Event event;
		if(viewer.isCurrentlyAnimating()) {
			SDL_Delay(10);
			haveEvent = SDL_PollEvent(&event) > 0;
		}
		else {
			haveEvent = SDL_WaitEvent(&event) > 0;
			lastTime = currentTimeMillis();
		}
		while(haveEvent) {
			if(!HandleEvent(event)) return;
			/* Read further events if there are any. It's important that we do this to keep
			 * the SDL queue as empty as possible. The ReadStdinThread will push a lot of
			 * events into it and we want to keep it responsible to other real input events.
			 */
			haveEvent = SDL_PollEvent(&event) > 0;			
		}
		
		long dt = currentTimeMillis() - lastTime;
		lastTime += dt;
		
		FillSurface(SDL_GetVideoSurface(), backgroundCol);
		viewer.frame(SDL_GetVideoSurface(), dt);
		SDL_Flip(SDL_GetVideoSurface());
	}
}

void Viewer_pushInitialGame(Game* game) {
	SDL_Event ev; memset(&ev, 0, sizeof(SDL_Event));
	ev.type = SDL_USEREVENT;
	ev.user.code = EVENT_STDIN_INITIAL;
	ev.user.data1 = game;
	while(SDL_PushEvent(&ev) < 0) SDL_Delay(1); // repeat until pushed
}

void Viewer_pushGameState(GameState* state) {
	SDL_Event ev; memset(&ev, 0, sizeof(SDL_Event));
	ev.type = SDL_USEREVENT;
	ev.user.code = EVENT_STDIN_CHUNK;
	ev.user.data1 = state;
	while(SDL_PushEvent(&ev) < 0) SDL_Delay(1); // repeat until pushed	
}

void Viewer_pushGameStateDebugInfo(GameDebugInfo* info) {
	SDL_Event ev; memset(&ev, 0, sizeof(SDL_Event));
	ev.type = SDL_USEREVENT;
	ev.user.code = EVENT_STDIN_DEBUG;
	ev.user.data1 = info;
	while(SDL_PushEvent(&ev) < 0) SDL_Delay(1); // repeat until pushed	
}
