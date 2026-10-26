#pragma once

#include "prelude.h"

namespace game_state_machine {
	enum class UIState {
		MainMenu,
		Settings,
		LevelSelect,
		// In game specific UIs
		Game,
		GamePauseMenu,
		// Hide all UI
		Hidden,
	};

	enum class GameState {
		Inactive,
		Playing,
		Paused,
		BetweenRounds,
	};

	UIState ui = UIState::MainMenu;
	GameState game = GameState::Inactive;

	/* GameState transitions */

	Str begin_game(u32 level) {
		if (game != GameState::Inactive) { return "Game must be Inactive to begin game"s; }
		game = GameState::BetweenRounds;
	}
	Str end_game() {}
	Str pause_game() {}
	Str unpause_game() {}
}