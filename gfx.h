/*
 *  gfx.h
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#ifndef __PW__GFX_H__
#define __PW__GFX_H__

#include <SDL.h>
#include <cassert>
#include "utils.h"

inline SDL_PixelFormat* getMainPixelFormat() { return SDL_GetVideoSurface()->format; }

struct Color {
	Color() : r(0), g(0), b(0), a(SDL_ALPHA_OPAQUE) {}
	Color(Uint8 _r, Uint8 _g, Uint8 _b) : r(_r), g(_g), b(_b), a(SDL_ALPHA_OPAQUE) {}
	Color(Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a) : r(_r), g(_g), b(_b), a(_a) {}
	Color(SDL_PixelFormat *f, Uint32 cl) { SDL_GetRGBA(cl, f, &r, &g, &b, &a); }
	explicit Color(Uint32 cl)	{ set(getMainPixelFormat(), cl); }
	Color(const SDL_Color& cl) : r(cl.r), g(cl.g), b(cl.b), a(SDL_ALPHA_OPAQUE) {}
	static Color fromDefault(Uint32 cl) { return Color(Uint8(cl >> 24), Uint8(cl >> 16), Uint8(cl >> 8), Uint8(cl)); }
	
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
	
	Uint32 get() const { return get(getMainPixelFormat()); }
	Uint32 get(SDL_PixelFormat *f) const { return SDL_MapRGBA(f, r, g, b, a); }
	Uint32 getDefault() const { return (Uint32(r) << 24) | (Uint32(g) << 16) | (Uint32(b) << 8) | Uint32(a); }
	Color derived(Sint16 _r, Sint16 _g, Sint16 _b, Sint16 _a) const {
		return Color(
					 Uint8(CLAMP(_r + r, 0, 255)),
					 Uint8(CLAMP(_g + g, 0, 255)),
					 Uint8(CLAMP(_b + b, 0, 255)),
					 Uint8(CLAMP(_a + a, 0, 255)));
	}
	void set(SDL_PixelFormat *f, Uint32 cl) { SDL_GetRGBA(cl, f, &r, &g, &b, &a); }
	
	bool operator == ( const Color & c ) const { return r == c.r && g == c.g && b == c.b && a == c.a; };
	bool operator != ( const Color & c ) const { return ! ( *this == c ); };
	
	Color operator * ( float f ) const { return Color( Uint8(CLAMP(r*f,0.0f,255.0f)), Uint8(CLAMP(g*f,0.0f,255.0f)), Uint8(CLAMP(b*f,0.0f,255.0f)), a ); };
	Color operator + ( const Color & c ) const { return Color( (Uint8)CLAMP(Uint16(r)+c.r,0,255), (Uint8)CLAMP(Uint16(g)+c.g,0,255), (Uint8)CLAMP(Uint16(b)+c.b,0,255), (Uint8)CLAMP(Uint16(a)+c.a,0,255)); };
	bool operator<(const Color& c) const {
		if(r != c.r) return r < c.r;
		if(g != c.g) return g < c.g;
		if(b != c.b) return b < c.b;
		return a < c.a;
	}
	Uint8& operator[](int i) { switch(i) { case 0: return r; case 1: return g; case 2: return b; case 3: return a; default: assert(false); } return *((Uint8*)NULL); }
	Uint8 operator[](int i) const { switch(i) { case 0: return r; case 1: return g; case 2: return b; case 3: return a; default: assert(false); } return 0; }
};

inline void FillSurface(SDL_Surface* surf, Color col) {
	SDL_Rect r = {0, 0, surf->w, surf->h};
	SDL_FillRect(surf, &r, col.get(surf->format));
}

#endif
