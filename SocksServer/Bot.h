#pragma once
#include "ClientWG.h"
#include "Game.h"

class Bot
{
public:
	void step() {
		if (Game::get().get_player() != bot_id) {
			ClientWG::get().send_data({Action::TURN});
			return;
		}
		
	};
private:
	int bot_id;
};

