/*
 *  viewgame.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 13.09.10.
 *  code under GPLv3
 *
 */

#include <SDL.h>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <string>
#include <list>
#include "utils.h"
#include "game.h"
#include "gfx.h"
#include "viewer.h"

using namespace std;

static int screenw = 640, screenh = 480, screenbpp = 0;
static char* argv0 = NULL;

void PrintHelpAndExit() {
	cerr << "usage: " << argv0 << " [-s WxH[xBPP]] [-h]" << endl;
	_exit(0);
}

void ParseParams(int argc, char** argv) {
	argv0 = argv[0];
	for(int i = 1; i < argc; ++i) {
		std::string arg = ToLower(argv[i]);
		if(arg == "-s") {
			if(i == argc - 1) {
				cerr << "-s expecting option" << endl;
				PrintHelpAndExit();
			}
			std::vector<std::string> toks = Tokenize(argv[i+1], "x");
			if(toks.size() < 2 || toks.size() > 3) {
				cerr << "-s expecting WxH[xBPP]" << endl;
				PrintHelpAndExit();
			}
			screenw = atoi(toks[0].c_str());
			screenh = atoi(toks[1].c_str());
			if(toks.size() > 2) screenbpp = atoi(toks[2].c_str());			
		}
		else if(arg == "-h")
			PrintHelpAndExit();
		else {
			cerr << "don't understand: " << arg << endl;
			PrintHelpAndExit();
		}
	}
}

#define EVENT_STDIN_INITIAL 1
#define EVENT_STDIN_CHUNK 2

// read stdin and push to SDL queue
int ReadStdinThread(void*) {
	int state = EVENT_STDIN_INITIAL;
	char c;
	std::string buf;
	while(read(STDIN_FILENO, &c, 1) > 0) {		
		SDL_Event ev; memset(&ev, 0, sizeof(SDL_Event));
		ev.type = SDL_USEREVENT;
		ev.user.type = state;
		
		switch (state) {
			case EVENT_STDIN_INITIAL:
				if(c == '|') {
					ev.user.data1 = strdup(buf.c_str());
					buf = "";
					state = EVENT_STDIN_CHUNK;
				}
				else buf += c;
				break;
			case EVENT_STDIN_CHUNK:
				if(c == ':') {
					ev.user.data1 = strdup(buf.c_str());
					buf = "";
				}
				else buf += c;
				break;
			default:
				assert(0);
		}
		
		if(ev.user.data1)
			while(SDL_PushEvent(&ev) < 0) SDL_Delay(1); // repeat until pushed
	}
	return 0;
}

int main(int argc, char** argv) {
	ParseParams(argc, argv);
	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);
	
	// we want to redraw when these occur
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	SDL_EventState(SDL_VIDEOEXPOSE, SDL_ENABLE);

	SDL_Thread* stdinReader = SDL_CreateThread(&ReadStdinThread, NULL);
	
	// init window
	if(SDL_SetVideoMode(screenw, screenh, screenbpp, 0) == NULL) {
		cerr << "setting video mode " << screenw << "x" << screenh << "x" << screenbpp << " failed: "
			 << SDL_GetError() << endl;
		PrintHelpAndExit();
	}
	SDL_WM_SetCaption("PlanetWars visualizer", NULL);
	FillSurface(SDL_GetVideoSurface(), Color(0, 0, 0));

	Viewer viewer;
	
	// main loop
	SDL_Event event;
	while ( SDL_WaitEvent(&event) >= 0 ) {
		switch(event.type) {
			case SDL_QUIT: goto exit;
			case SDL_USEREVENT: {
				char* str = (char*)event.user.data1;
				switch(event.user.type) {
					case EVENT_STDIN_INITIAL:
						viewer.gameStates.push_back(Game());
						viewer.init();
						assert(viewer.gameStates.back().ParseGamePlaybackInitial(str));
						break;
					case EVENT_STDIN_CHUNK:
						viewer.gameStates.push_back(Game());
						assert(viewer.gameStates.back().ParseGamePlaybackChunk(str, viewer.gameStates.front()));
						break;
					default: assert(0);
				}
				free(str);
				break;
			}
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_LEFT: viewer.last(); break;
					case SDLK_RIGHT: viewer.next(); break;
				}
				break;
		}
		
		viewer.draw();
	}

exit:
	_exit(0); // for now. seems that the reader even keeps busy with the close() below
	close(STDIN_FILENO);
	SDL_WaitThread(stdinReader, NULL);
	SDL_Quit();
	return 0;
}
