#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <queue>
#include <set>

#include "Point.h"
#include "utility.h"
#include "ClientWG.h"

using std::string;
using std::queue;
using std::set;
using std::vector;
using nlohmann::json;

struct Tank {
    string type;
    int max_health;
    int speed;
    int player_id;
    int tank_id;
    int health;
    Point spawn_position;
    Point position;
    int capture_points;

    static bool cmp(const Tank* a, const Tank* b) {
        if (a->capture_points == b->capture_points) return a->health < b->health;
        return a->capture_points > b->capture_points;
    };

    virtual Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) = 0;
    virtual bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) = 0;

    Tank() = default;
    Tank(json j, string tank_id_, string type, int max_health, int speed);
protected:
    Query try_move(vector<MapCode> map_matrix, int rad);
};

struct SPG : public Tank {
	SPG(json j, string id) : Tank(j, id, "spg", 1, 1) {}

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) override;
    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix);
};

struct AT_SPG : public Tank {
	AT_SPG(json j, string id) : Tank(j, id, "at_spg", 2, 1) {}

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) override;
    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
};

struct Light_tank : public Tank {
	Light_tank(json j, string id) : Tank(j, id, "light_tank", 1, 3) {}

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) override;
    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
};

struct Medium_tank : public Tank {
	Medium_tank(json j, string id) : Tank(j, id, "medium_tank", 1, 3) {}

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) override;
    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
};

struct Heavy_tank : public Tank {
	Heavy_tank(json j, string id) : Tank(j, id, "heavy_tank", 1, 3) {}

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked) override;
    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
};