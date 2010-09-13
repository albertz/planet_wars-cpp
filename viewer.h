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
#include "game.h"

struct SDL_Surface;

void DrawGame(const Game& game, SDL_Surface* surf);

struct Viewer {
	std::list<Game> gameStates;
	std::list<Game>::iterator currentState;
	
	void init() { assert(ready()); currentState = gameStates.begin(); }
	bool ready() { return !gameStates.empty(); }
	bool next() {
		if(!ready()) return false;
		std::list<Game>::iterator n = currentState; ++n;
		if(n == gameStates.end()) return false;
		++currentState; return true;
	}
	bool last() {
		if(!ready()) return false;
		if(currentState == gameStates.begin()) return false;
		--currentState; return true;
	}
	void draw(SDL_Surface* surf = SDL_GetVideoSurface()) {
		if(ready())
			DrawGame(*currentState, surf);
	}
};

#endif
