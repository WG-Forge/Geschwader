#pragma once

#include <json.hpp>

using nlohmann::json;

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

	Point operator-(const Point& other) {
		return Point(x - other.x, y - other.y, z - other.z);
	}

	Point operator/(const int& scale) {
		return Point(x / scale, y / scale, z / scale);
	}

	bool operator==(const Point& other) {
		return x == other.x && y == other.y;
	}
};