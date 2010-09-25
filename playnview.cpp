/*
 *  playnview.cpp
 *  PlanetWars
 *
 *  Created by Albert Zeyer on 16.09.10.
 *  code under GPLv3
 *
 */

#include <signal.h> // for kill
#include <unistd.h> // for getpid
#include "engine.h"
#include "viewer.h"

static void OnInitialGame(const Game& game) {
	Viewer_pushInitialGame(new Game(game));
}

static void OnNextGameState(const Game& game) {
	Viewer_pushGameState(new GameState(game.state));
}

static int PlayGameThread(void*) {
	PWMainloopCallbacks callbacks;
	callbacks.OnInitialGame = &OnInitialGame;
	callbacks.OnNextGameState = &OnNextGameState;
	return PW__mainloop(callbacks) ? 0 : 1;
}

int main(int argc, char** argv) {
	if(!PW__init(argc, argv, NULL)) return 1;		
	if(!Viewer_initWindow("PlanetWars")) return 1;
	
	SDL_Thread* player = SDL_CreateThread(&PlayGameThread, NULL);

	Viewer_mainLoop();
	kill(getpid(), SIGQUIT); // shortcut hack. needed to avoid complicateness when you close the SDL window
	
	SDL_WaitThread(player, NULL);
	SDL_Quit();
	return 0;
}
