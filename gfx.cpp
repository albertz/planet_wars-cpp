/*
 *  gfx.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#include <SDL.h>
#include <cmath>
#include "gfx.h"
#include "SDL_picofont.h"
#include "PixelFunctors.h"

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

Vec TextGetSize(const std::string& txt) {
	return FNT_GetSize(txt);
}

inline bool LockSurface(SDL_Surface * bmp)  {
	if (SDL_MUSTLOCK(bmp))
		return SDL_LockSurface(bmp) != -1;
	return true;
}

inline void UnlockSurface(SDL_Surface * bmp)  {
	if (SDL_MUSTLOCK(bmp))
		SDL_UnlockSurface(bmp);
}

#define LOCK_OR_QUIT(bmp)	{ if(!LockSurface(bmp)) return; }
#define LOCK_OR_FAIL(bmp)	{ if(!LockSurface(bmp)) return false; }

typedef char byte;
typedef unsigned char uchar;


//
// Clipping routines
//


class SDLRectBasic : public SDL_Rect {
public:
	typedef Sint16 Type;
	typedef Uint16 TypeS;
	
	SDLRectBasic() { this->SDL_Rect::x = this->SDL_Rect::y = this->SDL_Rect::w = this->SDL_Rect::h = 0; }
	SDLRectBasic(const SDL_Rect & r): SDL_Rect(r) {}
	Type& x() { return this->SDL_Rect::x; }
	Type& y() { return this->SDL_Rect::y; }
	TypeS& width() { return this->SDL_Rect::w; }
	TypeS& height() { return this->SDL_Rect::h; }
	
	Type x() const { return this->SDL_Rect::x; }
	Type y() const { return this->SDL_Rect::y; }
	TypeS width() const { return this->SDL_Rect::w; }
	TypeS height() const { return this->SDL_Rect::h; }
};

template<typename _Type, typename _TypeS>
class RefRectBasic {
public:
	typedef _Type Type;
	typedef _TypeS TypeS;
private:
	Type *m_x, *m_y;
	TypeS *m_w, *m_h;
public:
	RefRectBasic() : m_x(NULL), m_y(NULL), m_w(NULL), m_h(NULL) {
		// HINT: never use this constructor directly; it's only there to avoid some possible compiler-warnings
		assert(false);
	}
	RefRectBasic(Type& x_, Type& y_, TypeS& w_, TypeS& h_)
	: m_x(&x_), m_y(&y_), m_w(&w_), m_h(&h_) {}
	
	Type& x() { return *m_x; }
	Type& y() { return *m_y; }
	TypeS& width() { return *m_w; }
	TypeS& height() { return *m_h; }
	
	Type x() const { return *m_x; }
	Type y() const { return *m_y; }
	TypeS width() const { return *m_w; }
	TypeS height() const { return *m_h; }
};


// _RectBasic has to provide the following public members:
//		typedef ... Type; // type for x,y
//		typedef ... TypeS; // type for w,h
//		Type x();
//		Type y();
//		TypeS width();
//		TypeS height();
//		and the above as const
template<typename _RectBasic>
class OLXRect : public _RectBasic {
public:
	
	OLXRect(const _RectBasic & r): _RectBasic(r) {}
	
	class GetX2 {
	protected:
		const _RectBasic* base;
	public:
		GetX2(const _RectBasic* b) : base(b) {}
		operator typename _RectBasic::Type () const
		{ return base->x() + base->width(); }
	};
	class AssignX2 : public GetX2 {
	private:
		_RectBasic* base;
	public:
		AssignX2(_RectBasic* b) : GetX2(b), base(b) {}
		AssignX2& operator=(const typename _RectBasic::Type& v)
		{ base->width() = v - base->x(); return *this; }	
	};
	
	AssignX2 x2() { return AssignX2(this); }
	GetX2 x2() const { return GetX2(this); }
	
	class GetY2 {
	protected:
		const _RectBasic* base;
	public:
		GetY2(const _RectBasic* b) : base(b) {}
		operator typename _RectBasic::Type () const
		{ return base->y() + base->height(); }
	};
	class AssignY2 : public GetY2 {
	private:
		_RectBasic* base;
	public:
		AssignY2(_RectBasic* b) : GetY2(b), base(b) {}
		AssignY2& operator=(const typename _RectBasic::Type& v)
		{ base->height() = v - base->y(); return *this; }
	};
	AssignY2 y2() { return AssignY2(this); }
	GetY2 y2() const { return GetY2(this); }
	
	template<typename _ClipRect>
	bool clipWith(const _ClipRect& clip) {
		// Horizontal
		{
			typename OLXRect::Type orig_x2 = this->OLXRect::x2();
			this->OLXRect::x() = std::max( (typename OLXRect::Type)this->OLXRect::x(), (typename OLXRect::Type)clip.x() );
			this->OLXRect::x2() = std::min( orig_x2, (typename OLXRect::Type)clip.x2() );
			this->OLXRect::x2() = std::max( this->OLXRect::x(), (typename OLXRect::Type)this->OLXRect::x2() );
		}
		
		// Vertical
		{
			typename OLXRect::Type orig_y2 = this->OLXRect::y2();
			this->OLXRect::y() = std::max( (typename OLXRect::Type)this->OLXRect::y(), (typename OLXRect::Type)clip.y() );
			this->OLXRect::y2() = std::min( orig_y2, (typename OLXRect::Type)clip.y2() );
			this->OLXRect::y2() = std::max( this->OLXRect::y(), (typename OLXRect::Type)this->OLXRect::y2() );
		}
		
		return (this->OLXRect::width() && this->OLXRect::height());
	}
};


typedef OLXRect<SDLRectBasic> SDLRect;  // Use this for creating clipping rects from SDL

template<typename _Type, typename _TypeS, typename _ClipRect>
bool ClipRefRectWith(_Type& x, _Type& y, _TypeS& w, _TypeS& h, const _ClipRect& clip) {
	RefRectBasic<_Type, _TypeS> refrect = RefRectBasic<_Type, _TypeS>(x, y, w, h);
	return ((OLXRect<RefRectBasic<_Type, _TypeS> >&) refrect).clipWith(clip);
}

template<typename _ClipRect>
bool ClipRefRectWith(SDL_Rect& rect, const _ClipRect& clip) {
	RefRectBasic<Sint16,Uint16> refrect(rect.x, rect.y, rect.w, rect.h);
	return OLXRect< RefRectBasic<Sint16,Uint16> >(refrect).clipWith(clip);
}



void DrawHLine(SDL_Surface * bmpDest, int x, int x2, int y, Color colour) {
	
	if (bmpDest->flags & SDL_HWSURFACE)  {
		DrawRectFill(bmpDest, x, y, x2, y + 1, colour); // In hardware mode this is much faster, in software it is slower
		return;
	}
	
	// Swap the ends if necessary
	if (x2 < x)  {
		int tmp;
		tmp = x;
		x = x2;
		x2 = tmp;
	}
	
	const SDL_Rect& r = bmpDest->clip_rect;
	
	// Clipping
	if (y < r.y) return;
	if (y >= (r.y + r.h)) return;
	
	if (x < r.x)
		x = r.x;
	else if (x >= (r.x + r.w))
		return;
	
	if (x2 < r.x)
		return;
	else if (x2 >= (r.x + r.w))
		x2 = r.x + r.w - 1;
	
	// Lock
	LOCK_OR_QUIT(bmpDest);
	byte bpp = (byte)bmpDest->format->BytesPerPixel;
	Uint8 *px2 = (uchar *)bmpDest->pixels+bmpDest->pitch*y+bpp*x2;
	
	// Draw depending on the alpha
	switch (colour.a)  {
		case SDL_ALPHA_OPAQUE:  
		{
			// Solid (no alpha) drawing
			PixelPut& putter = getPixelPutFunc(bmpDest);
			Uint32 packed_cl = Pack(colour, bmpDest->format);
			for (Uint8* px = (Uint8*)bmpDest->pixels + bmpDest->pitch * y + bpp * x; px <= px2; px += bpp)
				putter.put(px, packed_cl);
		} break;
		case SDL_ALPHA_TRANSPARENT:
			break;
		default:
		{
			// Draw the line alpha-blended with the background
			PixelPutAlpha& putter = getPixelAlphaPutFunc(bmpDest);
			for (Uint8* px = (Uint8*)bmpDest->pixels + bmpDest->pitch * y + bpp * x; px <= px2; px += bpp)
				putter.put(px, bmpDest->format, colour);
		}
	}
	
	UnlockSurface(bmpDest);
	
}

// Draw vertical line
void DrawVLine(SDL_Surface * bmpDest, int y, int y2, int x, Color colour) {
	if (bmpDest->flags & SDL_HWSURFACE)  {
		DrawRectFill(bmpDest, x, y, x + 1, y2, colour); // In hardware mode this is much faster, in software it is slower
		return;
	}
	
	// Swap the ends if necessary
	if (y2 < y)  {
		int tmp;
		tmp = y;
		y = y2;
		y2 = tmp;
	}
	
	const SDL_Rect& r = bmpDest->clip_rect;
	
	// Clipping
	if (x < r.x) return;
	if (x >= (r.x + r.w)) return;
	
	if (y < r.y)
		y = r.y;
	else if (y >= (r.y + r.h))
		return;
	
	if (y2 < r.y)
		return;
	else if (y2 >= (r.y + r.h))
		y2 = r.y + r.h - 1;
	
	LOCK_OR_QUIT(bmpDest);
	ushort pitch = (ushort)bmpDest->pitch;
	byte bpp = (byte)bmpDest->format->BytesPerPixel;
	Uint8 *px2 = (Uint8 *)bmpDest->pixels+pitch*y2+bpp*x;
	
	// Draw depending on the alpha
	switch (colour.a)  {
		case SDL_ALPHA_OPAQUE:  
		{
			// Solid (no alpha) drawing
			PixelPut& putter = getPixelPutFunc(bmpDest);
			Uint32 packed_cl = Pack(colour, bmpDest->format);
			for (Uint8 *px= (Uint8 *)bmpDest->pixels+pitch*y + bpp*x; px <= px2; px+=pitch)
				putter.put(px, packed_cl);
		} break;
		case SDL_ALPHA_TRANSPARENT:
			break;
		default:
		{
			// Draw the line alpha-blended with the background
			PixelPutAlpha& putter = getPixelAlphaPutFunc(bmpDest);
			for (Uint8 *px= (Uint8 *)bmpDest->pixels+pitch*y + bpp*x; px <= px2; px+=pitch)
				putter.put(px, bmpDest->format, colour);
		}
	}
	
	UnlockSurface(bmpDest);
}


static void DrawRectFill_Overlay(SDL_Surface *bmpDest, const SDL_Rect& r, Color color)
{
	// Clipping
	if (!ClipRefRectWith((SDLRect&)r, (SDLRect&)bmpDest->clip_rect))
		return;
	
	const int bpp = bmpDest->format->BytesPerPixel;
	Uint8 *px = (Uint8 *)bmpDest->pixels + r.y * bmpDest->pitch + r.x * bpp;
	int step = bmpDest->pitch - r.w * bpp;
	
	// Draw the fill rect
	PixelPutAlpha& putter = getPixelAlphaPutFunc(bmpDest);
	for (int y = r.h; y; --y, px += step)
		for (int x = r.w; x; --x, px += bpp)  {
			putter.put(px, bmpDest->format, color);
		}
	
}


void PutPixelA(SDL_Surface * bmpDest, int x, int y, Uint32 colour, Uint8 a)  {
	Uint8* px = (Uint8*)bmpDest->pixels + y * bmpDest->pitch + x * bmpDest->format->BytesPerPixel;
	
	PixelPutAlpha& putter = getPixelAlphaPutFunc(bmpDest);
	Color c = Unpack_solid(colour, bmpDest->format); c.a = a;
	putter.put(px, bmpDest->format, c);
}

////////////////////
// Perform a line draw using a put pixel callback
// Grabbed from allegro
inline void perform_line(SDL_Surface * bmp, int x1, int y1, int x2, int y2, Color col, void (*proc)(SDL_Surface * , int, int, Uint32, Uint8))
{
	int dx = x2-x1;
	int dy = y2-y1;
	int i1, i2;
	int x, y;
	int dd;
	
	Uint32 d = col.get(bmp->format);
	
	LOCK_OR_QUIT(bmp);
	
	/* worker macro */
#define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond)     \
{                                                                         \
if (d##pri_c == 0) {                                                   \
proc(bmp, x1, y1, d, col.a);                                               \
return;                                                             \
}                                                                      \
\
i1 = 2 * d##sec_c;                                                     \
dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
\
x = x1;                                                                \
y = y1;                                                                \
\
while (pri_c pri_cond pri_c##2) {                                      \
proc(bmp, x, y, d, col.a);                                                 \
\
if (dd sec_cond 0) {                                                \
sec_c sec_sign##= 1;                                             \
dd += i2;                                                        \
}                                                                   \
else                                                                \
dd += i1;                                                        \
\
pri_c pri_sign##= 1;                                                \
}                                                                      \
}
	
	if (dx >= 0) {
		if (dy >= 0) {
			if (dx >= dy) {
				/* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
				DO_LINE(+, x, <=, +, y, >=);
			}
			else {
				/* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
				DO_LINE(+, y, <=, +, x, >=);
			}
		}
		else {
			if (dx >= -dy) {
				/* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
				DO_LINE(+, x, <=, -, y, <=);
			}
			else {
				/* (x1 <= x2) && (y1 > y2) && (dx < dy) */
				DO_LINE(-, y, >=, +, x, >=);
			}
		}
	}
	else {
		if (dy >= 0) {
			if (-dx >= dy) {
				/* (x1 > x2) && (y1 <= y2) && (dx >= dy) */
				DO_LINE(-, x, >=, +, y, >=);
			}
			else {
				/* (x1 > x2) && (y1 <= y2) && (dx < dy) */
				DO_LINE(+, y, <=, -, x, <=);
			}
		}
		else {
			if (-dx >= -dy) {
				/* (x1 > x2) && (y1 > y2) && (dx >= dy) */
				DO_LINE(-, x, >=, -, y, <=);
			}
			else {
				/* (x1 > x2) && (y1 > y2) && (dx < dy) */
				DO_LINE(-, y, >=, -, x, <=);
			}
		}
	}
	
#undef DO_LINE
	
	UnlockSurface(bmp);
}

inline void perform_line(SDL_Surface * bmp, int x1, int y1, int x2, int y2, Color col, void (*proc)(SDL_Surface * , int, int, Uint32))
{
	int dx = x2-x1;
	int dy = y2-y1;
	int i1, i2;
	int x, y;
	int dd;
	
	Uint32 d = col.get(bmp->format);
	
	LOCK_OR_QUIT(bmp);
	
	/* worker macro */
#define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond)     \
{                                                                         \
if (d##pri_c == 0) {                                                   \
proc(bmp, x1, y1, d);                                               \
return;                                                             \
}                                                                      \
\
i1 = 2 * d##sec_c;                                                     \
dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
\
x = x1;                                                                \
y = y1;                                                                \
\
while (pri_c pri_cond pri_c##2) {                                      \
proc(bmp, x, y, d);                                                 \
\
if (dd sec_cond 0) {                                                \
sec_c sec_sign##= 1;                                             \
dd += i2;                                                        \
}                                                                   \
else                                                                \
dd += i1;                                                        \
\
pri_c pri_sign##= 1;                                                \
}                                                                      \
}
	
	if (dx >= 0) {
		if (dy >= 0) {
			if (dx >= dy) {
				/* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
				DO_LINE(+, x, <=, +, y, >=);
			}
			else {
				/* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
				DO_LINE(+, y, <=, +, x, >=);
			}
		}
		else {
			if (dx >= -dy) {
				/* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
				DO_LINE(+, x, <=, -, y, <=);
			}
			else {
				/* (x1 <= x2) && (y1 > y2) && (dx < dy) */
				DO_LINE(-, y, >=, +, x, >=);
			}
		}
	}
	else {
		if (dy >= 0) {
			if (-dx >= dy) {
				/* (x1 > x2) && (y1 <= y2) && (dx >= dy) */
				DO_LINE(-, x, >=, +, y, >=);
			}
			else {
				/* (x1 > x2) && (y1 <= y2) && (dx < dy) */
				DO_LINE(+, y, <=, -, x, <=);
			}
		}
		else {
			if (-dx >= -dy) {
				/* (x1 > x2) && (y1 > y2) && (dx >= dy) */
				DO_LINE(-, x, >=, -, y, <=);
			}
			else {
				/* (x1 > x2) && (y1 > y2) && (dx < dy) */
				DO_LINE(-, y, >=, -, x, <=);
			}
		}
	}
	
#undef DO_LINE
	
	UnlockSurface(bmp);
}


//
// Line clipping - grabbed from SDL_gfx
//

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

static inline int clipEncode(int x, int y, int left, int top, int right, int bottom)
{
    int code = 0;
	
    if (x < left) {
		code |= CLIP_LEFT_EDGE;
    } else if (x > right) {
		code |= CLIP_RIGHT_EDGE;
    }
	
    if (y < top) {
		code |= CLIP_TOP_EDGE;
    } else if (y > bottom) {
		code |= CLIP_BOTTOM_EDGE;
    }
	
    return code;
}

bool ClipLine(SDL_Surface * dst, int * x1, int * y1, int * x2, int * y2)
{
    int left, right, top, bottom;
    Uint32 code1, code2;
    bool draw = false;
    int swaptmp;
    float m;
	
	// No line
	if (*x1 == *x2 && *y1 == *y2)
		return false;
	
    // Get clipping boundary
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
	
    while (true) {
		code1 = clipEncode(*x1, *y1, left, top, right, bottom);
		code2 = clipEncode(*x2, *y2, left, top, right, bottom);
		if (CLIP_ACCEPT(code1, code2)) {
			draw = true;
			break;
		} else if (CLIP_REJECT(code1, code2))
			break;
		else {
			if (CLIP_INSIDE(code1)) {
				swaptmp = *x2;
				*x2 = *x1;
				*x1 = swaptmp;
				swaptmp = *y2;
				*y2 = *y1;
				*y1 = swaptmp;
				swaptmp = code2;
				code2 = code1;
				code1 = swaptmp;
			}
			
			if (*x2 != *x1) {
				m = (*y2 - *y1) / (float) (*x2 - *x1);
			} else {
				m = 1.0f;
			}
			
			if (code1 & CLIP_LEFT_EDGE) {
				*y1 += (int) ((left - *x1) * m);
				*x1 = left;
			} else if (code1 & CLIP_RIGHT_EDGE) {
				*y1 += (int) ((right - *x1) * m);
				*x1 = right;
			} else if (code1 & CLIP_BOTTOM_EDGE) {
				if (*x2 != *x1) {
					*x1 += (int) ((bottom - *y1) / m);
				}
				*y1 = bottom;
			} else if (code1 & CLIP_TOP_EDGE) {
				if (*x2 != *x1) {
					*x1 += (int) ((top - *y1) / m);
				}
				*y1 = top;
			}
		}
    }
	
    return draw && (*x1 != *x2 || *y1 != *y2);
}


inline void secure_perform_line(SDL_Surface * bmpDest, int x1, int y1, int x2, int y2, Color color, void (*proc)(SDL_Surface *, int, int, Uint32, Uint8)) {
	if (!ClipLine(bmpDest, &x1, &y1, &x2, &y2)) // Clipping
		return;
	
	perform_line(bmpDest, x1, y1, x2, y2, color, proc);
}

inline void secure_perform_line(SDL_Surface * bmpDest, int x1, int y1, int x2, int y2, Color color, void (*proc)(SDL_Surface *, int, int, Uint32)) {
	if (!ClipLine(bmpDest, &x1, &y1, &x2, &y2)) // Clipping
		return;
	
	perform_line(bmpDest, x1, y1, x2, y2, color, proc);
}

///////////////////
// Line drawing
void DrawLine(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Color color) {
	secure_perform_line(dst, x1, y1, x2, y2, color, PutPixelA);
}


//////////////////////
// Draws a filled rectangle
void DrawRectFill(SDL_Surface *bmpDest, int x, int y, int x2, int y2, Color color)
{
	SDL_Rect r = { x, y, x2 - x, y2 - y };
	
	switch (color.a)  {
		case SDL_ALPHA_OPAQUE:
			SDL_FillRect(bmpDest,&r,color.get(bmpDest->format));
			break;
		case SDL_ALPHA_TRANSPARENT:
			break;
		default:
			DrawRectFill_Overlay(bmpDest, r, color);
	}
}


void DrawCircleFilled(SDL_Surface* bmpDest, int x, int y, int rx, int ry, Color color) {
	if(color.a == SDL_ALPHA_TRANSPARENT) return;
	if(rx <= 0 || ry <= 0) return;
	if(rx == 1) { DrawVLine(bmpDest, y - ry, y + ry, x, color); return; }
	if(ry == 1) { DrawHLine(bmpDest, x - rx, x + rx, y, color); return; }
	
	int innerRectW = int(rx / sqrt(2.0));
	int innerRectH = int(ry / sqrt(2.0));
	DrawRectFill(bmpDest, x - innerRectW, y - innerRectH, x + innerRectW + 1, y + innerRectH + 1, color);
	
	float f = float(rx) / float(ry);
	for(int _y = innerRectH + 1; _y < ry; _y++) {
		int w = int(f * sqrt(float(ry*ry - _y*_y))) - 1;
		
		DrawHLine(bmpDest, x - w, x + w, y - _y, color);
		DrawHLine(bmpDest, x - w, x + w, y + _y, color);
	}
	
	f = 1.0f / f;
	for(int _x = innerRectW + 1; _x < rx; _x++) {
		int h = int(f * sqrt(float(rx*rx - _x*_x))) - 1;
		
		DrawVLine(bmpDest, y - h, y + h, x - _x, color);
		DrawVLine(bmpDest, y - h, y + h, x + _x, color);
	}
}

static void PutPixel(SDL_Surface* s, int x, int y, Color c) {
	if(x < 0 || x >= s->w) return;
	if(y < 0 || y >= s->h) return;
	PutPixelA(s, x, y, c.get(s->format), c.a);
}

// TODO: not ready ...
void DrawCircle(SDL_Surface* bmpDest, int x, int y, int rx, int ry, Color color) {
	if(color.a == SDL_ALPHA_TRANSPARENT) return;
	if(rx <= 0 || ry <= 0) return;
	if(rx == 1) { DrawVLine(bmpDest, y - ry, y + ry, x, color); return; }
	if(ry == 1) { DrawHLine(bmpDest, x - rx, x + rx, y, color); return; }
	
	int innerRectW = int(rx / sqrt(2.0));
	int innerRectH = int(ry / sqrt(2.0));
	
	float f = float(rx) / float(ry);
	for(int _y = innerRectH + 1; _y < ry; _y++) {
		int w = int(f * sqrt(float(ry*ry - _y*_y))) - 1;

		PutPixel(bmpDest, x - w, y - _y, color);
		PutPixel(bmpDest, x + w, y - _y, color);
		PutPixel(bmpDest, x - w, y + _y, color);
		PutPixel(bmpDest, x + w, y + _y, color);
	}
	
	f = 1.0f / f;
	for(int _x = innerRectW + 1; _x < rx; _x++) {
		int h = int(f * sqrt(float(rx*rx - _x*_x))) - 1;
		
		PutPixel(bmpDest, x - _x, y - h, color);
		PutPixel(bmpDest, x + _x, y - h, color);
		PutPixel(bmpDest, x - _x, y + h, color);
		PutPixel(bmpDest, x + _x, y + h, color);
	}
}
