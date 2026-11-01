#include <SDL.h>

#include "game_manager.h"

// SDL.h is included so SDL can provide the platform entry point (WinMain on Windows)
int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	game_manager_init();
    game_manager_run();
	game_manager_shutdown();
	return 0;
}
