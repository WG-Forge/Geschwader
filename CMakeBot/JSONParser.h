#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <json.hpp>
#include "ClientWG.h"
#include "Tank.h"
#include "Point.h"

using nlohmann::json;

using std::string;
using std::map;
using std::vector;
using std::shared_ptr;

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

struct WinPoints {
	int capture;
	int kill;
	WinPoints() = default;
	WinPoints(json);
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

