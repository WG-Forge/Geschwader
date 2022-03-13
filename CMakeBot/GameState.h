#pragma once

#include <json.hpp>
#include <vector>
#include <memory>
#include <map>
#include "JSONParser.h"

using nlohmann::json;
using std::vector;
using std::map;
using std::shared_ptr;

struct GameState {
private:
	using TankRegistry = map<int, vector<shared_ptr<Tank>>>;
public:
	static GameState& get() {
		static GameState instance;
		return instance;
	}

	int num_players;
	int num_turns;
	int current_turn;
	vector<Player> players;
	vector<Player> observers;
	int current_player_idx;
	bool finished;
	TankRegistry vehicles;
	map<int, vector<int>> attack_matrix;
	int winner;
	map<int, WinPoints> win_points;
	void update(json str);
private:
	GameState() = default;
};