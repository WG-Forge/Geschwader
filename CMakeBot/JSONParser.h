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

enum Tanks {
	SPG = 0,
	LT = 1,
	TT = 2,
	ST = 3,
	AT_SPG = 4,
};

struct PlayerSend {
	string name;
	string password;
	string game;
	int num_turns;
	int num_players;
	bool is_observer;
	PlayerSend(string, string, string, int, int, bool);
	operator json() const;
};

struct Player {
	int idx;
	string name;
	bool is_observer;
	Player(json);
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
	operator json() const;

	Point operator+(const Point& other) {
		return Point(x + other.x, y + other.y, z + other.z);
	}

	bool operator==(const Point& other) {
		return x == other.x && y == other.y;
	}
};

struct TankType {
	Tanks name;
	int health;
	int speed;
	TankType() = default;
	TankType(Tanks str, int health_, int speed_): name(str), health(health_), speed(speed_) {}
};

struct Tank {
	int player_id;
	int tank_id;
	TankType vehicle_type;
	int health;
	Point spawn_position;
	Point position;
	int capture_points;
	int shoot_range_bonus;
	Tank() : player_id(-1) {}
	Tank(json, string);
};

struct WinPoints {
	int capture;
	int kill;
	WinPoints() = default;
	WinPoints(json);
};

struct GameState {
	int num_players;
	int num_turns;
	int current_turn;
	vector<Player> players;
	vector<Player> observers;
	int current_player_idx;
	bool finished;
	map<int, vector<Tank>> vehicles;
	map<int, vector<int>> attack_matrix;
	int winner;
	map<int, WinPoints> win_points;
	void from_json(json str);
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
	void from_json(json str);
};

struct Chat {
	string message;
	Chat(string str = "") : message(str) {}
	operator json() {
		json j = {};
		j["message"] = message;
		return j;
	}
};

struct DataAction {
	int vehicle_id;
	Point target;
	DataAction() = default;
	DataAction(string str) {
		json j = json::parse(str);
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

struct PlayerAction {
	int player_id;
	int action_type;
	DataAction data_action;
	Chat mes;
	PlayerAction(json j) {
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
	void from_json(json j) {
		actions.clear();
		for (const auto& action : j["actions"]) {
			actions.emplace_back(action);
		}
	}
};

