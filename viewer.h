/*
 *  viewer.h
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  coder under GPLv3
 *
 */

#ifndef __PW__VIEWER_H__
#define __PW__VIEWER_H__

#include <iterator>
#include <list>
#include <cassert>
#include <memory>
#include "game.h"
#include "FixedPointNumber.h"
#include "gamedebug.h"
#include "gfx.h"

struct SDL_Surface;

// Renders the current state of the game to a graphics object
//
// The offset is a number between 0 and 1 that specifies how far we are
// past this game state, in units of time. As this parameter varies from
// 0 to 1, the fleets all move in the forward direction. This is used to
// fake smooth animation.
void DrawGame(const GameDesc& desc, const GameState& state, SDL_Surface* surf, double offset = 0.0, const GameDebugInfo* debugInfo = NULL);

Color GetDefaultPlayerPlanetColor(int playerID);

struct ViewerState {
	size_t index;
	GameState state;
	GameDebugInfo debugInfo;
	ViewerState(size_t _i, const GameState& _s) : index(_i), state(_s) {}
};

struct Viewer {
	GameDesc gameDesc;
	std::list<ViewerState> gameStates;
	std::list<ViewerState>::iterator currentState;
	bool withAnimation;
	typedef FixedPointNumber<1000> Offset;
	Offset offsetToGo;
	long dtForAnimation;
	Viewer() : withAnimation(true), dtForAnimation(0) {}
	
	void init() { assert(ready()); currentState = gameStates.begin(); }
	bool ready() const { return !gameStates.empty(); }
	bool isAtStart() const { return currentState == gameStates.begin(); }
	bool isAtEnd() const { std::list<ViewerState>::iterator n = currentState; ++n; return n == gameStates.end(); }
	bool _next() {
		if(!ready()) return false;
		if(isAtEnd()) return false;
		++currentState; return true;
	}
	bool _last() {
		if(!ready()) return false;
		if(isAtStart()) return false;
		--currentState; return true;
	}
	
	void move(int d);
	void next() { move(1); }
	void last() { move(-1); }
	
	bool isCurrentlyAnimating() { return ready() && withAnimation && offsetToGo != 0; }
	void frame(SDL_Surface* surf, long dt);	
};

// ------- use this stuff if you want some simple SDL handling ----

extern int screenw, screenh, screenbpp;

bool Viewer_initWindow(const std::string& windowTitle); // returns true on success
void Viewer_mainLoop(); // returns on exit

// The viewer mainloop will get this.
// Note that these functions takes ownership of the pointer!
// These function are multithreading safe.
void Viewer_pushInitialGame(Game* game);
void Viewer_pushGameState(GameState* state);
void Viewer_pushGameStateDebugInfo(GameDebugInfo* info);

#endif
