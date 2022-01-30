#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "ClientWG.h"
using nlohmann::json;
struct PlayerSend {
	std::string name;
	std::string password;
	std::string game;
	int num_turns;
	int num_players;
	bool is_observer;
	PlayerSend(std::string _name, std::string _password = "",std::string _game = "", int _num_turns = 45, int _num_players = 1, bool _is_observer = false) :
	name(_name), game(_game), password(_password), num_turns(_num_turns), num_players(_num_players), is_observer(_is_observer) {}
	std::string to_json() {
		nlohmann::json j{};
		j["name"] = name;
		j["password"] = password;
		j["game"] = game;
		j["num_turns"] = num_turns;
		j["num_players"] = num_players;
		j["is_observer"] = is_observer;
		return j.dump();
	}
};
struct PlayerGet {
	int idx;
	std::string name;
	std::string password;
	bool is_observer;
	PlayerGet(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
		idx = j["idx"].get<int>();
		name = j["name"].get<std::string>();
		password = j["password"].get<std::string>();
		is_observer = j["is_observer"].get<bool>();
	}
};
struct Point {
	int x;
	int y;
	int z;
	Point(int x_, int y_, int z_) {
		x = x_;
		y = y_;
		z = z_;
	}
	Point(std::string str = "") {
		if (str != "") {
			nlohmann::json j = nlohmann::json::parse(str);
			x = j["x"].get<int>();
			y = j["y"].get<int>();
			z = j["z"].get<int>();
		}
	}
	std::string to_json() {
		nlohmann::json j{};
		j["x"] = x;
		j["y"] = y;
		j["z"] = z;
		return j.dump();
	}
};
struct TankType {
	std::string name;
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
	Tank() {
		player_id = -1;
	}
	Tank(std::string str, std::string _tank_id) {
		nlohmann::json j = nlohmann::json::parse(str);
		player_id = j["player_id"].get<int>();
		std::string type = j["vehicle_type"].get<std::string>();
		if (type == "medium_tank") {
			vehicle_type = TankType("medium_tank", 2, 2);
		}
		health = j["health"].get<int>();
		spawn_position = Point(j["spawn_position"].dump());
		position = Point(j["position"].dump());
		capture_points = j["capture_points"].get<int>();
		tank_id = stoi(_tank_id);
	}
};
struct WinPoints {
	int capture;
	int kill;
	WinPoints() {}
	WinPoints(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
		capture = j["capture"].get<int>();
		kill = j["kill"].get<int>();
	}
};
struct GameState {
	int num_players;
	int num_turns;
	int current_turn;
	std::vector<PlayerGet> players;
	std::vector<PlayerGet> observers;
	int current_player_idx;
	bool finished;
	std::map<int, std::vector<Tank>> vehicles;
	std::map<int, std::vector<int>> attack_matrix;
	int winner;
	std::map<int, WinPoints> win_points;
	void from_json(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
		num_players = j["num_players"].get<int>();
		num_turns = j["num_turns"].get<int>();
		current_turn = j["current_turn"].get<int>();
		std::vector<json> jj = j["players"].get<std::vector<json>>();
		players.clear();
		for (auto value : jj) {
			players.push_back(PlayerGet(value.dump()));
		}
		jj = j["observers"].get<std::vector<json>>();
		observers.clear();
		for (auto value : jj) {
			observers.push_back(PlayerGet(value.dump()));
		}
		current_player_idx = j["current_player_idx"].get<int>();
		finished = j["finished"].get<bool>();
		vehicles.clear();
		int counter = 1;
		std::map<json, json> jmap = j["vehicles"].get<std::map<json, json>>();
		for (auto it = jmap.begin(); it != jmap.end();++it) {
			Tank buf(it->second.dump(), it->first.get<std::string>());
			vehicles[buf.player_id].push_back(buf);
		}
		jmap = j["attack_matrix"].get<std::map<json, json>>();
		for (auto it = jmap.begin(); it != jmap.end(); ++it) {
			attack_matrix[stoi(it->first.get<std::string>())] = it->second.get<std::vector<int>>();
		}
		if (j["winner"].is_null()) {
			winner = -1;
		}
		else {
			winner = j["winner"].get<int>();
		}
		win_points.clear();
		jmap = j["win_points"].get<std::map<json, json>>();
		for (auto it = jmap.begin(); it != jmap.end(); ++it) {
			WinPoints buf(it->second.dump());
			win_points[stoi(it->first.get<std::string>())] = buf;
		}
	}
};
struct Map {
	int size;
	std::string name;
	std::vector<std::map<std::string, std::vector<Point>>> spawn_points;
	std::vector<Point> base;
	std::vector<Point> catapult;
	std::vector<Point> hard_repair;
	std::vector<Point> light_repair;
	std::vector<Point> obstacle;
	void from_json(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
		size = j["size"].get<int>();
		name = j["name"].get<std::string>();
		std::vector<json> jvec = j["spawn_points"].get<std::vector<json>>();
		for (int i = 0; i < jvec.size(); i++) {
			std::map<std::string, std::vector<Point>> buf;
			std::map<json, std::vector<json>> jmap = jvec[i].get<std::map<json, std::vector<json>>>();
			for (auto it = jmap.begin(); it != jmap.end(); ++it) {
				for (int k = 0; k < it->second.size(); ++k) {
					buf[it->first.get<std::string>()].push_back(Point(it->second[k].dump()));
				}
			}
			spawn_points.push_back(buf);
		}
		jvec = j["content"]["base"].get<std::vector<json>>();
		for (auto it : jvec) {
			base.push_back(Point(it.dump()));
		}
		jvec = j["content"]["catapult"].get<std::vector<json>>();
		for (auto it : jvec) {
			catapult.push_back(Point(it.dump()));
		}
		jvec = j["content"]["hard_repair"].get<std::vector<json>>();
		for (auto it : jvec) {
			hard_repair.push_back(Point(it.dump()));
		}
		jvec = j["content"]["light_repair"].get<std::vector<json>>();
		for (auto it : jvec) {
			light_repair.push_back(Point(it.dump()));
		}
		jvec = j["content"]["obstacle"].get<std::vector<json>>();
		for (auto it : jvec) {
			obstacle.push_back(Point(it.dump()));
		}
	}
};
struct Chat {
	std::string message;
	Chat(std::string str = "") : message(str) {}
	std::string to_json() {
		nlohmann::json j = {};
		j["message"] = message;
		return j.dump();
	}
};
struct DataAction {
	int vehicle_id;
	Point target;
	DataAction() {}
	DataAction(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
		vehicle_id = j["vehicle_id"].get<int>();
		target = Point(j["target"].dump());
	}
	std::string to_json() {
		nlohmann::json j = {};
		j["vehicle_id"] = vehicle_id;
		j["target"] = nlohmann::json::parse(target.to_json());
		return j.dump();
	}
};
struct PlayerAction {
	int player_id;
	int action_type;
	DataAction data_action;
	Chat mes;
	PlayerAction(std::string str) {
		nlohmann::json j = nlohmann::json::parse(str);
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
	std::vector<PlayerAction> actions;
	void from_json(std::string str) {
		actions.clear();
		nlohmann::json j = nlohmann::json::parse(str);
		for (auto it = j["actions"].begin(); it != j["actions"].end(); ++it) {
			actions.push_back(PlayerAction(it->dump()));
		}
	}
};

