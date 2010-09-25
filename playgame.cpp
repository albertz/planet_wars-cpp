/*
 *  playgame.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 25.09.10.
 *  code under GPLv3
 *
 */

#include "engine.h"

int main(int argc, char** argv) {
	if(!PW__init(argc, argv)) return 1;

	return PW__mainloop() ? 0 : 1;
}
