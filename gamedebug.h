/*
 *  gamedebug.h
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 18.09.10.
 *  code under GPLv3
 *
 */

#ifndef __PW__GAMEDEBUG_H__
#define __PW__GAMEDEBUG_H__

#include <map>
#include <string>
#include "gfx.h"

struct GameDebugInfo {
	std::map<int, std::string> planetInfo;
	std::map<int, Color> planetColor;
};

#endif
