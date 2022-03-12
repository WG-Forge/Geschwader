#pragma once

#include <vector>
#include <string>
#include <map>
#include <json.hpp>

#include "Point.h"

using std::map;
using std::string;
using std::vector;
using nlohmann::json;

struct Map {
	static Map& get() {
		static Map instance;
		return instance;
	}

	int size;
	string name;
	vector<map<string, vector<Point>>> spawn_points;
	vector<Point> base;
	std::vector<Point> catapult;
	std::vector<Point> hard_repair;
	std::vector<Point> light_repair;
	vector<Point> obstacle;
	void update(json str);
private:
	Map() = default;
};