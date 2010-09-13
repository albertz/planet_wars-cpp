/*
 *  gfx.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#include <SDL.h>
#include "gfx.h"
#include "SDL_picofont.h"

void DrawText(SDL_Surface* surf, const std::string& txt, Color col, int x, int y, bool center) {
	SDL_Color sdlCol = {col.r, col.g, col.b};
	SDL_Surface* txtSurf = FNT_Render(txt.c_str(), sdlCol);
	SDL_Rect srcRect = {0, 0, txtSurf->w, txtSurf->h};
	SDL_Rect dstRect = {x, y, txtSurf->w, txtSurf->h};
	if(center) {
		dstRect.x -= txtSurf->w / 2;
		dstRect.y -= txtSurf->h / 2;
	}
	SDL_BlitSurface(txtSurf, &srcRect, surf, &dstRect);
	SDL_FreeSurface(txtSurf);
}

