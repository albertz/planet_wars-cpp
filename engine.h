/*
 *  engine.h
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 25.09.10.
 *  code under GPLv3
 *
 */

#ifndef __PW__ENGINE_H__
#define __PW__ENGINE_H__

#include <ostream>

struct Game;

struct PWMainloopCallbacks {
	void (*OnInitialGame)(const Game& game);
	void (*OnNextGameState)(const Game& game);
};

bool PW__init(int argc, char** argv, std::ostream* replayStream);
bool PW__mainloop(PWMainloopCallbacks callbacks = PWMainloopCallbacks());

#endif
