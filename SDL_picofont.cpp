/* sdl_picofont

   http://nurd.se/~noname/sdl_picofont

   File authors:
      Fredrik Hultin

   License: GPLv2
*/

#include <string>
#include "SDL_picofont.h"

#define FNT_FONTHEIGHT 8
#define FNT_FONTWIDTH 8

static Vec FNT_Generate(const std::string& text, size_t len, Uint16 w, unsigned char* pixels)
{
	unsigned int col = 0, row = 0, stop = 0;
	Vec xy;

	unsigned char *fnt = FNT_GetFont();

	for(size_t i = 0; i < len; i++){
		unsigned char chr;
		switch(text[i]) {
			case '\n':
				row++;
				col = 0;
				chr = 0;
				break;

			case '\r':
				chr = 0;
				break;

			case '\t':
				chr = 0;
				col += 4 - col % 4;
				break;
		
			case '\0':
				stop = 1;
				chr = 0;
				break;
	
			default:
				col++;
				chr = text[i];
				break;
		}

		if(stop)
			break;

		if((col + 1) * FNT_FONTWIDTH > (unsigned int)xy.x)
			xy.x = col * FNT_FONTWIDTH;
		
		if((row + 1) * FNT_FONTHEIGHT > (unsigned int)xy.y)
			xy.y = (row + 1) * FNT_FONTHEIGHT;

		if(chr != 0 && w != 0){
			for(short y = 0; y < FNT_FONTHEIGHT; y++){
				for(short x = 0; x < FNT_FONTWIDTH; x++){
					if(fnt[text[i] * FNT_FONTHEIGHT + y] >> (7 - x) & 1){
						pixels[((col - 1) * FNT_FONTWIDTH) + x + (y + row * FNT_FONTHEIGHT) * w] = 1;
					}
				}
			}
		}
	}

	return xy;	
}

Vec FNT_GetSize(const std::string& text, size_t len) {
	return FNT_Generate(text, len, 0, NULL);	
}
	
SDL_Surface* FNT_Render(const std::string& text, size_t len, Color color)
{
	SDL_Color colors[2] = {
		{	(color.r + 0x7e) % 0xff,
			(color.g + 0x7e) % 0xff,
			(color.b + 0x7e) % 0xff
		},
		{
			color.r,
			color.g,
			color.b
		}
	};

	Vec xy = FNT_Generate(text, len, 0, NULL);

	SDL_Surface* surface
	= SDL_CreateRGBSurface(
			SDL_SWSURFACE, 
			xy.x,
			xy.y,
			8,
			0,
			0,
			0,
			0
	);

	if(!surface) return NULL;

	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, SDL_MapRGB(surface->format, (color.r + 0x7e) % 0xff, (color.g + 0x7e) % 0xff, (color.b + 0x7e) % 0xff));
	/*SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x0d, 0xea, 0xd0));*/

	SDL_SetColors(surface, colors, 0, 2);

	if(SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);

	FNT_Generate(text, len, surface->w, (unsigned char*)surface->pixels);
	
	if(SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);

	return surface;
}

