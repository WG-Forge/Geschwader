#include "JSONParser.h"

PlayerSend::operator json() const{
	json j{};
	j["name"] = name;
	j["password"] = password;
	j["game"] = game;
	j["num_turns"] = num_turns;
	j["num_players"] = num_players;
	j["is_observer"] = is_observer;
	return j;
}

PlayerGet::PlayerGet(json j) {
	idx = j["idx"].get<int>();
	name = j["name"].get<string>();
	is_observer = j["is_observer"].get<bool>();
}

PlayerSend::PlayerSend(string _name, string _password = "", string _game = "",
	int _num_turns = 45, int _num_players = 1, bool _is_observer = false):
	name(_name), game(_game), password(_password), num_turns(_num_turns),
	num_players(_num_players), is_observer(_is_observer) {}

Point::operator json() const {
	json j{};
	j["x"] = x;
	j["y"] = y;
	j["z"] = z;
	return j;
}

Tank::Tank(json j, string tank_id_) {
	player_id = j["player_id"].get<int>();
	health = j["health"].get<int>();
	spawn_position = Point(j["spawn_position"]);
	position = Point(j["position"]);
	capture_points = j["capture_points"].get<int>();
	tank_id = stoi(tank_id_);
}

WinPoints::WinPoints(json j) {
	capture = j["capture"].get<int>();
	kill = j["kill"].get<int>();
}

void Map::from_json(json j) {
	size = j["size"].get<int>();
	name = j["name"].get<string>();
	vector<json> jvec = j["spawn_points"].get<vector<json>>();
	for (int i = 0; i < jvec.size(); i++) {
		map<string, vector<Point>> buf;
		map<json, vector<json>> jmap = jvec[i].get<map<json, vector<json>>>();
		for (const auto& it : jmap) {
			for (int k = 0; k < it.second.size(); ++k) {
				buf[it.first.get<string>()].emplace_back(it.second[k]);
			}
		}
		spawn_points.push_back(buf);
	}
	jvec = j["content"]["base"].get<vector<json>>();
	for (const auto& it : jvec) {
		base.emplace_back(it);
	}
	/*jvec = j["content"]["catapult"].get<std::vector<json>>();
	for (const auto& it : jvec) {
		catapult.emplace_back(it);
	}
	jvec = j["content"]["hard_repair"].get<std::vector<json>>();
	for (const auto& it : jvec) {
		hard_repair.emplace_back(it);
	}
	jvec = j["content"]["light_repair"].get<std::vector<json>>();
	for (const auto& it : jvec) {
		light_repair.emplace_back(it);
	}
	jvec = j["content"]["obstacle"].get<std::vector<json>>();
	for (const auto& it : jvec) {
		obstacle.emplace_back(it);
	}*/
}

void GameState::from_json(json j) {
	players.clear();
	observers.clear();
	vehicles.clear();
	win_points.clear();

	num_players = j["num_players"].get<int>();
	num_turns = j["num_turns"].get<int>();
	current_turn = j["current_turn"].get<int>();
	vector<json> jj = j["players"].get<vector<json>>();
	for (auto value : jj) {
		players.push_back(PlayerGet(value.dump()));
	}
	jj = j["observers"].get<vector<json>>();
	for (auto value : jj) {
		observers.push_back(PlayerGet(value.dump()));
	}
	current_player_idx = j["current_player_idx"].get<int>();
	finished = j["finished"].get<bool>();
	map<json, json> jmap = j["vehicles"].get<map<json, json>>();
	for (const auto& it : jmap) {
		Tank buf(it.second, it.first.get<string>());
		string type = it.second["vehicle_type"].get<string>();
		if (vehicles[buf.player_id].size() == 0) {
			vehicles[buf.player_id] = vector<Tank>(5);
		}
		if (type == "medium_tank") {
			buf.vehicle_type = TankType("medium_tank", 2, 2);
			vehicles[buf.player_id][3] = buf;
		}
		else if (type == "light_tank") {
			buf.vehicle_type = TankType("light_tank", 1, 3);
			vehicles[buf.player_id][1] = buf;
		}
		else if (type == "heavy_tank") {
			buf.vehicle_type = TankType("heavy_tank", 3, 1);
			vehicles[buf.player_id][2] = buf;
		}
		else if (type == "at_spg") {
			buf.vehicle_type = TankType("at_spg", 2, 1);
			vehicles[buf.player_id][4] = buf;
		}
		else if (type == "spg") {
			buf.vehicle_type = TankType("spg", 1, 1);
			vehicles[buf.player_id][0] = buf;
		}
	}
	jmap = j["attack_matrix"].get<map<json, json>>();
	for (const auto& it : jmap) {
		attack_matrix[stoi(it.first.get<string>())] = it.second.get<vector<int>>();
	}

	winner = j["winner"].is_null() ? -1 : j["winner"].get<int>();
	
	jmap = j["win_points"].get<map<json, json>>();
	for (const auto& it : jmap) {
		WinPoints buf(it.second);
		win_points[stoi(it.first.get<string>())] = buf;
	}
}

