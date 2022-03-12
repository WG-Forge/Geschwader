#include "Map.h"

void Map::update(json j) {
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