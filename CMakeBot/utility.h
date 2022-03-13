#pragma once

#include <algorithm>
#include <vector>

#include "Point.h"

using std::max;
using std::vector;

enum MapCode {
    NOTHING = 0,
    ENEMY_TANK = 1,
    FRIENDLY_TANK = 2,
    OBSTACLE = 3,
    HARD_REPAIR = 4,
    LIGHT_REPAIR = 5,
    CATAPULT = 6
};

extern const vector<Point> near_gex;

//assigns unique index for point
int code(const Point& p, int rad);

//returns point from unique code
Point decode(int value, int rad);

bool can_exist(const Point& buf, const int& rad);

int distance(const Point& x1, const Point& x2);

int safe_index(Point ps, const vector<MapCode>& map_matrix, int idx);