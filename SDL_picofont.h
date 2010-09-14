/* sdl_picofont

   http://nurd.se/~noname/sdl_picofont

   File authors:
      Fredrik Hultin

   License: GPLv2
*/

#ifndef SDL_PICOFONT_H
#define SDL_PICOFONT_H

#ifdef __cplusplus

#include <SDL.h>
#include <string>
#include "vec.h"
#include "gfx.h"

SDL_Surface* FNT_Render(const std::string& txt, size_t len, Color color);
inline SDL_Surface* FNT_Render(const std::string& txt, Color color) { return FNT_Render(txt, txt.size(), color); }

Vec FNT_GetSize(const std::string& txt, size_t len);
inline Vec FNT_GetSize(const std::string& txt) { return FNT_GetSize(txt, txt.size()); }

extern "C" {
#endif /* __cplusplus */

unsigned char* FNT_GetFont();
	
#ifdef __cplusplus
};
#endif /* __cplusplus */
	
#endif /* SDL_PICOFONT_H */
