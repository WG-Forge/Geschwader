#pragma once

class Game
{
public:
	inline static Game& get() {
		static Game instance;
		return instance;
	}

	inline bool is_finished() { return isFinished; }
	inline int get_player() { return player_id; }
private:
	Game() : isFinished(false),	player_id(0) {}

	bool isFinished;
	int player_id;
};
