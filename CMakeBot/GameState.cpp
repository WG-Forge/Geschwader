#include "GameState.h"

using std::make_shared;

void GameState::update(json j) {
	players.clear();
	observers.clear();
	vehicles.clear();
	win_points.clear();

	num_players = j["num_players"].get<int>();
	num_turns = j["num_turns"].get<int>();
	current_turn = j["current_turn"].get<int>();
	current_player_idx = j["current_player_idx"].get<int>();
	finished = j["finished"].get<bool>();

	vector<json> players_data = j["players"].get<vector<json>>();
	for (const auto& player : players_data) {
		players.emplace_back(player);
	}
	vector<json> observers_data = j["observers"].get<vector<json>>();
	for (const auto& observer : observers_data) {
		observers.emplace_back(observer);
	}

	map<json, json> vehicle_data = j["vehicles"].get<map<json, json>>();
	for (const auto& [tank_id, tank_data] : vehicle_data) {
		int player_id = tank_data["player_id"].get<int>();
		//Tank buf(tank_data, tank_id.get<string>());
		string type = tank_data["vehicle_type"].get<string>();
		if (vehicles[player_id].size() == 0) {
			vehicles[player_id] = vector<shared_ptr<Tank>>(5);
		}
		if (type == "spg") {
			vehicles[player_id][0] = make_shared<SPG>(tank_data, tank_id.get<string>());
			//buf.vehicle_type = TankType("spg", 1, 1);
			//vehicles[player_id][0] = buf;
		}
		else if (type == "light_tank") {
			vehicles[player_id][1] = make_shared<Light_tank>(tank_data, tank_id.get<string>());
			//buf.vehicle_type = TankType("light_tank", 1, 3);
			//vehicles[player_id][1] = buf;
		}
		else if (type == "heavy_tank") {
			vehicles[player_id][2] = make_shared<Heavy_tank>(tank_data, tank_id.get<string>());
			//buf.vehicle_type = TankType("heavy_tank", 3, 1);
			//vehicles[player_id][2] = buf;
		}
		else if (type == "medium_tank") {
			vehicles[player_id][3] = make_shared<Medium_tank>(tank_data, tank_id.get<string>());
			//buf.vehicle_type = TankType("medium_tank", 2, 2);
			//vehicles[player_id][3] = buf;
		}
		else if (type == "at_spg") {
			vehicles[player_id][4] = make_shared<AT_SPG>(tank_data, tank_id.get<string>());
			//buf.vehicle_type = TankType("at_spg", 2, 1);
			//vehicles[player_id][4] = buf;
		}
	}
	map<json, json> attack_data = j["attack_matrix"].get<map<json, json>>();
	for (const auto& [attacker, victim] : attack_data) {
		attack_matrix[stoi(attacker.get<string>())] = victim.get<vector<int>>();
	}

	winner = j["winner"].is_null() ? -1 : j["winner"].get<int>();

	map<json, json> points_data = j["win_points"].get<map<json, json>>();
	for (const auto& [player, points] : points_data) {
		win_points[stoi(player.get<string>())] = WinPoints(points);
	}
}