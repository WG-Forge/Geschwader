#pragma once
#include <string>
#include <vector>
#include <map>
#include <json.hpp>
#include "ClientWG.h"

using nlohmann::json;

using std::vector;
using std::string;
using std::map;

struct PlayerSend {
	string name;
	string password;
	string game;
	int num_turns;
	int num_players;
	bool is_observer;
	PlayerSend(string _name, string _password = "",string _game = "", int _num_turns = 45, int _num_players = 1, bool _is_observer = false) :
	name(_name), game(_game), password(_password), num_turns(_num_turns), num_players(_num_players), is_observer(_is_observer) {}
	operator json() {
		json j{};
		j["name"] = name;
		j["password"] = password;
		j["game"] = game;
		j["num_turns"] = num_turns;
		j["num_players"] = num_players;
		j["is_observer"] = is_observer;
		return j;
	}
};

struct PlayerGet {
	int idx;
	string name;
	string password;
	bool is_observer;
	PlayerGet(json j) {
		idx = j["idx"].get<int>();
		name = j["name"].get<string>();
		is_observer = j["is_observer"].get<bool>();
		//password = j["password"].get<string>();
	}
};

struct Point {
	int x, y, z;

	Point() = default;
	Point(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
	Point(json j) {
		x = j["x"].get<int>();
		y = j["y"].get<int>();
		z = j["z"].get<int>();
	}
	operator json() {
		json j{};
		j["x"] = x;
		j["y"] = y;
		j["z"] = z;
		return j;
	}

	Point operator+(const Point& other) {
		return { x + other.x, y + other.y, z + other.z };
	}
	Point& operator+=(const Point& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}
};

struct TankType {
	string name;
	int health;
	int speed;
};

struct Tank {
	int player_id;
	int tank_id;
	TankType vehicle_type;
	int health;
	Point spawn_position;
	Point position;
	int capture_points;
	Tank() : player_id(-1) {}
	Tank(json j, string _tank_id) {
		tank_id = stoi(_tank_id);
		player_id = j["player_id"].get<int>();

		health = j["health"].get<int>();
		capture_points = j["capture_points"].get<int>();
		position = Point(j["position"]);
		spawn_position = Point(j["spawn_position"]);

		string type = j["vehicle_type"].get<string>();
		if (type == "medium_tank") {
			vehicle_type = { type, 2, 2 };
		}
	}
};

struct WinPoints {
	int capture;
	int kill;
	WinPoints() = default;
	WinPoints(json j) {
		capture = j["capture"].get<int>();
		kill = j["kill"].get<int>();
	}
};

struct GameState {
	int num_players;
	int num_turns;
	int current_turn;
	vector<PlayerGet> players;
	vector<PlayerGet> observers;
	int current_player_idx;
	bool finished;
	map<int, vector<Tank>> vehicles;
	map<int, vector<int>> attack_matrix;
	int winner;
	map<int, WinPoints> win_points;
	void from_json(json j) {
		players.clear();
		observers.clear();
		vehicles.clear();
		win_points.clear();

		num_players = j["num_players"].get<int>();
		num_turns = j["num_turns"].get<int>();
		current_turn = j["current_turn"].get<int>();

		for (auto it : j["players"].get<vector<json>>()) {
			players.emplace_back(it);
		}
		for (auto it : j["observers"].get<vector<json>>()) {
			observers.emplace_back(it);
		}

		current_player_idx = j["current_player_idx"].get<int>();
		finished = j["finished"].get<bool>();

		for (auto it : j["vehicles"].get<map<json, json>>()) {
			Tank buf(it.second, it.first.get<string>());
			vehicles[buf.player_id].push_back(buf);
		}

		for (auto it : j["attack_matrix"].get<map<json, json>>()) {
			attack_matrix[stoi(it.first.get<string>())] = it.second.get<vector<int>>();
		}

		winner = j["winner"].is_null() ? -1 : j["winner"].get<int>();

		for (auto it : j["win_points"].get<map<json, json>>()) {
			win_points[stoi(it.first.get<string>())] = { it.second };
		}
	}
};

struct Map {
	int size;
	string name;
	vector<map<string, vector<Point>>> spawn_points;
	vector<Point> base;
	vector<Point> catapult;
	vector<Point> hard_repair;
	vector<Point> light_repair;
	vector<Point> obstacle;
	void from_json(json j) {
		size = j["size"].get<int>();
		name = j["name"].get<string>();
		vector<json> jvec = j["spawn_points"].get<vector<json>>();
		for (auto point : jvec) {
			map<string, vector<Point>> buf;
			map<json, vector<json>> jmap = point.get<map<json, vector<json>>>();
			for (auto it : jmap) {
				for (int k = 0; k < it.second.size(); ++k) {
					buf[it.first.get<string>()].emplace_back(it.second[k]);
				}
			}
			spawn_points.push_back(buf);
		}
		jvec = j["content"]["base"].get<vector<json>>();
		for (auto it : jvec) {
			base.emplace_back(it);
		}
		/*jvec = j["content"]["catapult"].get<vector<json>>();
		for (auto it : jvec) {
			catapult.emplace_back(it);
		}
		jvec = j["content"]["hard_repair"].get<vector<json>>();
		for (auto it : jvec) {
			hard_repair.emplace_back(it);
		}
		jvec = j["content"]["light_repair"].get<vector<json>>();
		for (auto it : jvec) {
			light_repair.emplace_back(it);
		}*/
		jvec = j["content"]["obstacle"].get<vector<json>>();
		for (auto it : jvec) {
			obstacle.emplace_back(it);
		}
	}
};

//struct Chat {
//	string message;
//	Chat(string str = "") : message(str) {}
//	operator json() {
//		json j = {};
//		j["message"] = message;
//		return j;
//	}
//};

struct DataAction {
	int vehicle_id;
	Point target;
	DataAction() = default;
	DataAction(json j) {
		vehicle_id = j["vehicle_id"].get<int>();
		target = Point(j["target"]);
	}
	operator json() {
		json j = {};
		j["vehicle_id"] = vehicle_id;
		j["target"] = target;
		return j;
	}
};

//struct PlayerAction {
//	int player_id;
//	int action_type;
//	DataAction data_action;
//	Chat mes;
//	PlayerAction(json j) {
//		player_id = j["player_id"];
//		action_type = j["action_type"];
//		if (action_type == Action::CHAT) {
//			mes = Chat(j["data"]["message"].get<string>());
//		}
//		else {
//			data_action = DataAction(j["data"].dump());
//		}
//	}
//};

//struct PlayersActions {
//	vector<PlayerAction> actions;
//	void from_json(json j) {
//		actions.clear();
//		for (auto it : j["actions"]) {
//			actions.push_back(PlayerAction(it));
//		}
//	}
//};
