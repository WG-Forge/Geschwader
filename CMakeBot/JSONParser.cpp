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

Player::Player(json j) {
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
	shoot_range_bonus = j["shoot_range_bonus"].get<int>();
	tank_id = stoi(tank_id_);
}

WinPoints::WinPoints(json j) {
	capture = j["capture"].get<int>();
	kill = j["kill"].get<int>();
}

void Map::from_json(json j) {
	size = j["size"].get<int>();
	name = j["name"].get<string>();
	vector<json> tank_spawn_points = j["spawn_points"].get<vector<json>>();
	for (const auto& type : tank_spawn_points) {
		map<string, vector<Point>> buf;
		map<json, vector<json>> type_points = type.get<map<json, vector<json>>>();
		for (const auto& [type, points] : type_points) {
			for (const auto& point : points) {
				buf[type.get<string>()].emplace_back(point);
			}
		}
		spawn_points.push_back(buf);
	}
	vector<json> bases = j["content"]["base"].get<vector<json>>();
	for (const auto& it : bases) {
		base.emplace_back(it);
	}
	vector<json> catapults = j["content"]["catapult"].get<std::vector<json>>();
	for (const auto& it : catapults) {
		catapult.emplace_back(it);
	}
	vector<json> hard_repairs = j["content"]["hard_repair"].get<std::vector<json>>();
	for (const auto& it : hard_repairs) {
		hard_repair.emplace_back(it);
	}
	vector<json> light_repairs = j["content"]["light_repair"].get<std::vector<json>>();
	for (const auto& it : light_repairs) {
		light_repair.emplace_back(it);
	}
	vector<json> obstacles = j["content"]["obstacle"].get<vector<json>>();
	for (const auto& it : obstacles) {
		obstacle.emplace_back(it);
	}
	for (const auto& pl : spawn_points) {
		for (const auto& point : pl) {
			obstacle.insert(obstacle.end(), point.second.begin(), point.second.end());
		}
	}
}

void GameState::from_json(json j) {
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
		Tank buf(tank_data, tank_id.get<string>());
		string type = tank_data["vehicle_type"].get<string>();
		if (vehicles[buf.player_id].size() == 0) {
			vehicles[buf.player_id] = vector<Tank>(5);
		}
		if (type == "spg") {
			buf.vehicle_type = TankType(Tanks::SPG, 1, 1);
			vehicles[buf.player_id][0] = buf;
		}
		else if (type == "light_tank") {
			buf.vehicle_type = TankType(Tanks::LT, 1, 3);
			vehicles[buf.player_id][1] = buf;
		}
		else if (type == "heavy_tank") {
			buf.vehicle_type = TankType(Tanks::TT, 3, 1);
			vehicles[buf.player_id][2] = buf;
		}
		else if (type == "medium_tank") {
			buf.vehicle_type = TankType(Tanks::ST, 2, 2);
			vehicles[buf.player_id][3] = buf;
		}
		else if (type == "at_spg") {
			buf.vehicle_type = TankType(Tanks::AT_SPG, 2, 1);
			vehicles[buf.player_id][4] = buf;
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

