#pragma once
#include <string>
#include <vector>
#include <map>
#include "json.hpp"
#include "ClientWG.h"

using nlohmann::json;
using std::string;
using std::map;
using std::vector;

struct PlayerSend {
	string name;
	string password;
	string game;
	int num_turns;
	int num_players;
	bool is_observer;
	PlayerSend(string, string, string, int, int, bool);
	string to_json() const;
};

struct PlayerGet {
	int idx;
	string name;
	bool is_observer;
	PlayerGet(string);
};

struct Point {
	int x = 0;
	int y = 0;
	int z = 0;
	Point(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
	Point(string str = "") {
		if (str != "") {
			json j = json::parse(str);
			x = j["x"].get<int>();
			y = j["y"].get<int>();
			z = j["z"].get<int>();
		}
	}
	string to_json();
};

struct TankType {
	string name;
	int health;
	int speed;
	TankType() {}
	TankType(string str, int health_, int speed_): name(str), health(health_), speed(speed_) {}
};

struct Tank {
	int player_id;
	int tank_id;
	TankType vehicle_type;
	int health;
	Point spawn_position;
	Point position;
	int capture_points;
	Tank() {
		player_id = -1;
	}
	Tank(string, string);
};

struct WinPoints {
	int capture;
	int kill;
	WinPoints() {}
	WinPoints(string);
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
	void from_json(string str);
};

struct Map {
	int size;
	string name;
	vector<map<string, vector<Point>>> spawn_points;
	vector<Point> base;
	/*std::vector<Point> catapult;
	std::vector<Point> hard_repair;
	std::vector<Point> light_repair;
	std::vector<Point> obstacle;*/
	void from_json(string str);
};

struct Chat {
	string message;
	Chat(string str = "") : message(str) {}
	string to_json() {
		json j = {};
		j["message"] = message;
		return j.dump();
	}
};

struct DataAction {
	int vehicle_id;
	Point target;
	DataAction() {}
	DataAction(string str) {
		json j = json::parse(str);
		vehicle_id = j["vehicle_id"].get<int>();
		target = Point(j["target"].dump());
	}
	string to_json() {
		json j = {};
		j["vehicle_id"] = vehicle_id;
		j["target"] = json::parse(target.to_json());
		return j.dump();
	}
};

struct PlayerAction {
	int player_id;
	int action_type;
	DataAction data_action;
	Chat mes;
	PlayerAction(string str) {
		json j = json::parse(str);
		player_id = j["player_id"];
		action_type = j["action_type"];
		if (action_type == Action::CHAT) {
			mes = Chat(j["data"]["message"].get<std::string>());
		}
		else {
			data_action = DataAction(j["data"].dump());
		}
	}
};

struct PlayersActions {
	vector<PlayerAction> actions;
	void from_json(string str) {
		actions.clear();
		json j = json::parse(str);
		for (auto it = j["actions"].begin(); it != j["actions"].end(); ++it) {
			actions.push_back(PlayerAction(it->dump()));
		}
	}
};

